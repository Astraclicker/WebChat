#include "web.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <log.h>
#include <memory>
#include "../MySQL/MySql.h"
#include <chrono>

namespace {
    //登录比对不一致时,最多补发多少条
    constexpr int kSyncRecentLimit = 200;
}

//构造函数
session::session(
    tcp::socket socket,
    astra_sql::MySQLPool &mysqlPoolRef,
    astra_sql::Redispp &redisAPIRef,
    std::set<std::shared_ptr<session> > &sessions
) : mysqlPool(mysqlPoolRef),
    redisAPI(redisAPIRef),
    sessionSocker(std::move(socket)),
    sessionSet(sessions) {
    //数据库相关的检查环节丢给server了
}

//启动接口
void session::start() {
    sessionSet.insert(shared_from_this());
    doRead();
}

//广播给会话集合中匹配用户名的客户端
void session::broadCast(const std::string &msg, const std::string &receiver, const std::string &sendTime) const {
    nlohmann::json out;
    out["type"] = "text";
    out["data"]["receiver"] = receiver;
    out["data"]["sender"] = this->myUserName;
    out["data"]["text"] = msg;
    //带上发送方的时间戳: 接收端拿它落库,两端存的 sendTime 才一致(登录时才比得出来)
    out["data"]["sendTime"] = sendTime;
    const std::string frame = out.dump() + "\n";

    for (auto &session: sessionSet) {
        //好友和群组功能作为拓展功能，目前不校验receiver，只排排除自己
        if (session != this->shared_from_this()) {
            session->deliver(frame);
            //广播明细属于调试信息
            LOG(astra_log::Level::Debug, this->sessionSocker.remote_endpoint(), " send to ",
                session->sessionSocker.remote_endpoint());
        }
    }
}

//消息在这里排队发送
void session::deliver(const std::string &msg) {
    //将消息写入接收队列
    const bool sendProgress = !sendMsgs.empty();
    sendMsgs.push_back(msg);
    if (!sendProgress) {
        doWrite();
    }
}

//把一条消息存进聊天记录表
void session::saveMyChatHistory(const std::string &sender, const std::string &text, const std::string &sendTime) const {
    //检测登录状态是否异常(没拿到 uid 说明还没登录成功)
    if (this->myUid <= 0) {
        return;
    }
    //存库失败单独抛出异常日志，不影响其他功能
    try {
        if (!saveChatHistory(*mysqlPool.borrow(), sender, text, sendTime)) {
            LOG(astra_log::Level::Error, "save chat history failed: ", this->myUserName);
        }
    } catch (const std::exception &error) {
        LOG(astra_log::Level::Error, "save chat history error: ", error.what());
    }
}

//处理客户端发送的同步请求
void session::handle_sync(const nlohmann::json &data) {
    if (this->myUid <= 0) {
        nlohmann::json out;
        out["type"] = "error";
        out["data"] = "请先登录";
        deliver(out.dump() + "\n");
        return;
    }

    //解释客户端发送的同步请求
    const std::string lastSender = data.value("lastSender", std::string{});
    const std::string lastText = data.value("lastText", std::string{});
    const std::string lastTime = data.value("lastTime", std::string{});

    nlohmann::json response;
    response["type"] = "syncResponse";

    try {
        auto connection = mysqlPool.borrow();

        //服务端这一侧的最新一条
        const auto latest = fetchLatestChatRows(*connection, 1);
        const auto latestSenders = latest.value("sender", nlohmann::json::array());
        const auto latestTexts = latest.value("text", nlohmann::json::array());
        const auto latestTimes = latest.value("sendTime", nlohmann::json::array());

        std::string serverSender;
        std::string serverText;
        std::string serverTime;
        if (!latestSenders.empty() && !latestTexts.empty() && !latestTimes.empty()) {
            serverSender = latestSenders.back().get<std::string>();
            serverText = latestTexts.back().get<std::string>();
            serverTime = latestTimes.back().get<std::string>();
        }

        const bool matched = (serverSender == lastSender)
                             && (serverText == lastText)
                             && (serverTime == lastTime);
        response["data"]["matched"] = matched;

        if (!matched) {
            //对不上就把最近的一批发回去,客户端按 (sender,text,sendTime) 去重后补进本地
            const auto recent = fetchLatestChatRows(*connection, kSyncRecentLimit);
            const auto senders = recent.value("sender", nlohmann::json::array());
            const auto texts = recent.value("text", nlohmann::json::array());
            const auto times = recent.value("sendTime", nlohmann::json::array());
            const std::size_t count = std::min({senders.size(), texts.size(), times.size()});

            nlohmann::json messages = nlohmann::json::array();
            for (std::size_t i = 0; i < count; ++i) {
                nlohmann::json one;
                one["sender"] = senders[i];
                one["text"] = texts[i];
                one["sendTime"] = times[i];
                messages.push_back(std::move(one));
            }
            response["data"]["messages"] = messages;

            LOG(astra_log::Level::Info, "sync for uid ", this->myUid, ": send ", messages.size(),
                " recent records");
        }

        deliver(response.dump() + "\n");
    } catch (const std::exception &error) {
        LOG(astra_log::Level::Error, "sync failed: ", error.what());
        nlohmann::json out;
        out["type"] = "error";
        out["data"] = "同步失败";
        deliver(out.dump() + "\n");
    }
}

//将要发送的消息队列异步写入socket
void session::doWrite() {
    auto self(shared_from_this());
    boost::asio::async_write(
        sessionSocker,
        boost::asio::buffer(sendMsgs.front().data(), sendMsgs.front().size()),
        [this,self](const boost::system::error_code &errorCode, size_t) {
            if (!errorCode) {
                sendMsgs.pop_front();
                if (!sendMsgs.empty()) {
                    doWrite();
                }
            } else {
                sessionSet.erase(self);
                sendMsgs.clear();
            }
        });
}

//处理客户端登录请求
void session::handle_login(const std::string &login_user_name, const std::string &password) {
    const std::string cache_key = "login_cache:" + login_user_name;
    try {
        const auto cached_login = redisAPI.get_string(cache_key);
        if (cached_login) {
            const auto cached_data = nlohmann::json::parse(*cached_login);
            if (cached_data.value("password", std::string{}) == password) {
                bool ttl_refreshed = this->redisAPI.expire_key(cache_key, std::chrono::seconds(300));
                if (!ttl_refreshed) {
                    LOG(astra_log::Level::Error, "login cache expired before TTL refresh: ",
                        login_user_name);
                }
                this->myUserName = login_user_name;
                //缓存里带着 uid,命中时不必再查库
                this->myUid = cached_data.value("uid", 0LL);

                nlohmann::json response;
                //原本这里由数据库相关逻辑执行,现在交给redis
                response["type"] = "mysqlLoginFeedBack";
                response["data"] = "success";
                //客户端要用 uid 命名本地 SQLite 库
                response["uid"] = this->myUid;
                deliver(response.dump() + "\n");

                LOG(astra_log::Level::Info, "Login cache hit: ", login_user_name);

                //缓存命中登录成功
                LOG(astra_log::Level::Info, "login success");

                if (this->myUid > 0) {
                    createChatHistoryTable(*mysqlPool.borrow());
                }
                return;
            }
        }
    } catch (const std::exception &error) {
        LOG(astra_log::Level::Error, "failed to read login cache, fallback to MySQL: ", error.what());
    }
    std::string auth_user_name;
    long long auth_uid = 0;
    std::string frame;
    {
        login(*mysqlPool.borrow(),
              login_user_name, password,
              auth_user_name,
              auth_uid,
              frame);
    }

    //登录回执统一走本会话的发送队列:
    //避免和 broadCast 触发的 doWrite() 并发写同一个 socket
    if (!frame.empty()) {
        deliver(frame);
    }

    if (auth_user_name.empty()) //缓存未命中,查数据库查不到,登录失败
    {
        return;
    }
    //缓存未命中,查数据库回填
    this->myUserName = auth_user_name;
    this->myUid = auth_uid;

    //缓存未命中,如果查数据库查到了,就写入缓存,登录成功
    try {
        if (this->myUid > 0) {
            createChatHistoryTable(*mysqlPool.borrow());
        }
        //把 uid 和密码一起缓存
        nlohmann::json cache_value;
        cache_value["password"] = password;
        cache_value["uid"] = this->myUid;
        redisAPI.set_string(cache_key,
                            cache_value.dump(),
                            std::chrono::seconds(300));
        LOG(astra_log::Level::Info, "login cache stored: ", auth_user_name);
    } catch (const std::exception &error) {
        LOG(astra_log::Level::Error, "fail to store login cache: ", error.what());
    }
}

//处理客户端的创建用户请求
void session::handle_create_user(const std::string &login_user_name, const std::string &password) {
    long long new_uid = 0;
    {
        std::string frame;
        auto connection = mysqlPool.borrow();
        const bool ok = createUser(*connection, login_user_name, password, new_uid, frame);
        //成功/失败都要给客户端回执,同样走发送队列,不直接写 socket
        if (!frame.empty()) {
            deliver(frame);
        }
        if (!ok) {
            return;
        }
        //注册成功时确保聊天记录表存在
        if (new_uid > 0) {
            createChatHistoryTable(*connection);
        }
    }

    const std::string cache_key = "login_cache:" + login_user_name;
    try {
        nlohmann::json cache_value;
        cache_value["password"] = password;
        cache_value["uid"] = new_uid;
        const bool cache_stored = redisAPI.set_string(cache_key,
                                                      cache_value.dump(),
                                                      std::chrono::seconds(300));
        if (!cache_stored) {
            LOG(astra_log::Level::Error, "fail to store login cache when create user", login_user_name);
        }
    } catch (std::exception &error) {
        LOG(astra_log::Level::Error, "fail to store login cache when create user", error.what());
    }
}

//从readBuffer中读取数据并调用广播函数
void session::doRead() {
    auto self(shared_from_this());
    boost::asio::async_read_until(
        sessionSocker,
        readBuf,
        '\n',
        [this,self](const boost::system::error_code &errorCode, std::size_t length) {
            if (!errorCode) {
                std::istream is(&readBuf);
                std::string line;
                std::getline(is, line);

                try {
                    const auto readMsg = nlohmann::json::parse(line);
                    const auto type = readMsg.value("type", std::string{});
                    const auto data = readMsg.value("data", nlohmann::json::object());

                    if (type == "text") {
                        const auto receiver = data.value("receiver", std::string{});
                        const auto text = data.value("text", std::string{});
                        //发送方给的时间戳(格式不对时用服务端本地时间兜底),双方存同一个值
                        const auto clientTime = data.value("sendTime", std::string{});
                        const std::string sendTime = (clientTime.size() == 19) ? clientTime : nowLocalTimestamp();
                        //广播的同时，给自己存一份
                        saveMyChatHistory(myUserName, text, sendTime);
                        broadCast(text, receiver, sendTime);
                    } else if (type == "syncRequested") {
                        handle_sync(data);
                    } else {
                        const auto loginUserName = data.value("userName", std::string{});
                        const auto password = data.value("password", std::string{});
                        if (type == "loginRequested") {
                            handle_login(loginUserName, password);
                        } else {
                            handle_create_user(loginUserName, password);
                        }
                    }
                } catch (const std::exception &error) {
                    LOG(astra_log::Level::Error, error.what());
                }
                doRead();
            } else {
                sessionSet.erase(self);
            }
        });
}
