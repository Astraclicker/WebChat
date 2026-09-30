#include "web.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include "../MySQL/MySql.h"
#include <chrono>
#include <stdexcept>

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
    //然而S端选库成功不代表C端选库成功
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
        //TODO 校验receiver(作为拓展功能)
        if (session != this->shared_from_this()) {
            session->deliver(frame);
            std::cout << this->sessionSocker.remote_endpoint() << " send to ";
            std::cout << session->sessionSocker.remote_endpoint() << std::endl;
            //TODO 聊天记录写入mysql
        }
    }
}

//消息只在这里排队发送,不做"顺手存库":
//聊天记录只有一张表,只有发送方写一次就行了——如果接收方也写,一条消息会被存 N+1 份
void session::deliver(const std::string &msg) {
    //将消息写入接收队列
    const bool sendProgress = !sendMsgs.empty();
    sendMsgs.push_back(msg);
    if (!sendProgress) {
        doWrite();
    }
}

//把一条消息存进聊天记录表(全服同一张)
void session::saveMyChatHistory(const std::string &sender, const std::string &text, const std::string &sendTime) {
    //检测登录状态是否异常(没拿到 uid 说明还没登录成功)
    if (this->myUid <= 0) {
        return;
    }
    //存库失败单独抛出异常日志，不影响其他功能
    try {
        auto connection = mysqlPool.borrow();
        if (!saveChatHistory(*connection, sender, text, sendTime)) {
            std::cerr << "save chat history failed: " << this->myUserName << std::endl;
        }
    } catch (const std::exception &error) {
        std::cerr << "save chat history error: " << error.what() << std::endl;
    }
}

void session::handle_sync(const nlohmann::json &data) {
    if (this->myUid <= 0) {
        nlohmann::json out;
        out["type"] = "error";
        out["data"] = "请先登录";
        deliver(out.dump() + "\n");
        return;
    }

    //客户端报上来的"本地最后一条"(本地一条都没有时是空串)
    const std::string lastSender = data.value("lastSender", std::string{});
    const std::string lastText = data.value("lastText", std::string{});
    const std::string lastTime = data.value("lastTime", std::string{});

    nlohmann::json response;
    response["type"] = "syncResponse";

    try {
        auto connection = mysqlPool.borrow();

        //服务端这一侧的最新一条(全服同一张表的最新一条)
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

            std::cout << "sync for uid " << this->myUid << ": send " << messages.size()
                    << " recent records" << std::endl;
        }

        deliver(response.dump() + "\n");
    } catch (const std::exception &error) {
        std::cerr << "sync failed: " << error.what() << std::endl;
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
            }
        });
}

void session::handle_login(const std::string &login_user_name, const std::string &password) {
    const std::string cache_key = "login_cache:" + login_user_name;
    try {
        const auto cached_login = redisAPI.get_string(cache_key);
        if (cached_login) {
            //缓存里存的是 {"password":...,"uid":...}
            //(旧的纯密码格式会解析失败抛异常,直接落到下面的 MySQL 分支,相当于缓存未命中)
            const auto cached_data = nlohmann::json::parse(*cached_login);
            if (cached_data.value("password", std::string{}) == password) {
                bool ttl_refreshed = this->redisAPI.expire_key(cache_key, std::chrono::seconds(300));
                if (!ttl_refreshed) {
                    std::cerr << "login cache expired before TTL refresh: "
                            << login_user_name << std::endl;
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

                std::cout << "Login cache hit: "
                        << login_user_name << std::endl;

                //缓存命中登录成功
                std::cout << "login success" << std::endl;

                if (this->myUid > 0) {
                    createChatHistoryTable(*mysqlPool.borrow());
                }
                return;
            }
        }
    } catch (const std::exception &error) {
        std::cerr << "failed to read login cache, fallback to MySQL: "
                << error.what() << std::endl;
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
        //把 uid 和密码一起缓存,下次命中就不用再查库拿 uid 了
        nlohmann::json cache_value;
        cache_value["password"] = password;
        cache_value["uid"] = this->myUid;
        redisAPI.set_string(cache_key,
                            cache_value.dump(),
                            std::chrono::seconds(300));
        std::cout << "login cache stored: "
                << auth_user_name
                << std::endl;
    } catch (const std::exception &error) {
        std::cerr << "fail to store login cache: "
                << error.what()
                << std::endl;
    }
}

void session::handle_create_user(const std::string &login_user_name, const std::string &password) {
    //数据库连接的控制权在server而不是session
    //尽早归还数据库连接
    long long new_uid = 0;
    std::string frame;
    {
        auto connection = mysqlPool.borrow();
        const bool ok = createUser(*connection, login_user_name, password, new_uid, frame);
        //成功/失败都要给客户端回执,同样走发送队列,不直接写 socket
        if (!frame.empty()) {
            deliver(frame);
        }
        if (!ok) {
            return;
        }
        //注册成功时确保聊天记录表存在(全服一张,建表是幂等的)
        if (new_uid > 0) {
            createChatHistoryTable(*connection);
        }
    }

    const std::string cache_key = "login_cache:" + login_user_name;
    try {
        //和登录路径统一: 缓存里存 {"password":...,"uid":...}
        nlohmann::json cache_value;
        cache_value["password"] = password;
        cache_value["uid"] = new_uid;
        const bool cache_stored = redisAPI.set_string(cache_key,
                                                      cache_value.dump(),
                                                      std::chrono::seconds(300));
        if (!cache_stored) {
            std::cerr << "fail to store login cache when create user" << login_user_name << std::endl;
        }
    } catch (std::exception &error) {
        std::cerr << "fail to store login cache when create user"
                << error.what() << std::endl;
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
                    std::cerr << error.what() << std::endl;
                }
                doRead();
            } else {
                sessionSet.erase(self);
            }
        });
}
