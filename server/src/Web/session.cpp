#include "web.h"
#include <cstdlib>
#include <iostream>
#include <memory>
#include "../MySQL/MySql.h"
#include <chrono>
#include <log.h>

//构造函数
session::session(
    tcp::socket socket,
    const nlohmann::json &mysqlConfig,
    const nlohmann::json &redisConfig,
    std::set<std::shared_ptr<session> > &sessions
)
    : mysqlAPI(
          mysqlConfig["address"],
          mysqlConfig["port"],
          mysqlConfig["userName"],
          mysqlConfig["password"]
      ),
      redisAPI(
          redisConfig["address"],
          redisConfig["port"],
          redisConfig["userName"],
          redisConfig["password"],
          0
      ),
      sessionSocker(std::move(socket)),
      sessionSet(sessions) {
    if (mysqlAPI.switchDatabase("ChatServer") == astra_sql::SQLppError::success) {
        LOG(astra_log::Level::Info, "MySQL 就绪，已切换到数据库 ChatServer");
    } else {
        LOG(astra_log::Level::Error, "MySQL 切换数据库 ChatServer 失败，本会话的数据库操作会失败");
    }

    if (!mysqlAPI.mysqlTableExists("users")) {
        LOG(astra_log::Level::Info, "数据表 users 不存在，正在创建");
        auto createRule = std::vector<astra_sql::createTableRule>{
            {"uid", "int", "not null auto_increment"},
            {"userName", "varchar(50)", "not null"},
            {"password", "varchar(50)", "not null"}
        };
        const astra_sql::primaryKeyRule pk{"uid"};
        const astra_sql::uniqueKeyRule uk{"userName"};

        if (mysqlAPI.mysqlCreateTable("users", createRule, &pk, &uk) == astra_sql::SQLppError::success) {
            LOG(astra_log::Level::Info, "数据表 users 创建成功");
        } else {
            LOG(astra_log::Level::Error, "数据表 users 创建失败");
        }
    } else {
        LOG(astra_log::Level::Debug, "数据表 users 已存在");
    }
}

//启动接口
void session::start() {
    sessionSet.insert(shared_from_this());
    LOG(astra_log::Level::Info, "会话建立，当前在线会话数 = ", sessionSet.size());
    doRead();
}

//广播给会话集合中匹配用户名的客户端
void session::broadCast(const std::string &msg, const std::string &receiver) const {
    nlohmann::json out;
    out["type"] = "text";
    out["data"]["receiver"] = receiver;
    out["data"]["sender"] = this->myUserName;
    out["data"]["text"] = msg;
    const std::string frame = out.dump() + "\n";

    for (auto &session: sessionSet) {
        //好友和群组功能作为拓展功能，目前不校验receiver，只排排除自己
        //TODO 校验receiver(作为拓展功能)
        if (session != this->shared_from_this()) {
            session->deliver(frame);
            LOG(astra_log::Level::Info, "消息转发: ", this->myUserName, " -> ", receiver, " : ", msg);
            //TODO 聊天记录写入mysql
        }
    }
}

//将消息写入接收队列
void session::deliver(const std::string &msg) {
    const bool sendProgress = !sendMsgs.empty();
    sendMsgs.push_back(msg);
    if (!sendProgress) {
        doWrite();
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
                LOG(astra_log::Level::Warn, "写数据失败，移除会话: ", errorCode.message());
                sessionSet.erase(self);
            }
        });
}

void session::handle_login(const std::string &login_user_name, const std::string &password) {
    // 注意：日志里只记录用户名，不记录密码
    const std::string cache_key = "login_cache:" + login_user_name;
    try {
        const auto cached_password = redisAPI.get_string(cache_key);
        if (cached_password && *cached_password == password) {
            bool ttl_refreshed = this->redisAPI.expire_key(cache_key, std::chrono::seconds(300));
            if (!ttl_refreshed) {
                LOG(astra_log::Level::Warn, "登录缓存续期失败，可能已过期: ", login_user_name);
            }
            this->myUserName = login_user_name;

            nlohmann::json response;
            //原本这里由数据库相关逻辑执行,现在交给redis
            response["type"] = "mysqlLoginFeedBack";
            response["data"] = "success";
            deliver(response.dump() + "\n");

            LOG(astra_log::Level::Info, "登录成功（Redis 缓存命中）: ", login_user_name);
            return;
        }
    } catch (const std::exception &error) {
        LOG(astra_log::Level::Warn, "读取登录缓存失败，回退查询 MySQL: ", error.what());
    }

    LOG(astra_log::Level::Debug, "登录缓存未命中，转查 MySQL: ", login_user_name);

    std::string auth_user_name;
    login(mysqlAPI,
          login_user_name, password,
          sessionSocker,
          auth_user_name);
    if (auth_user_name.empty()) //缓存未命中,查数据库查不到,登录失败
    {
        LOG(astra_log::Level::Warn, "登录失败，用户名或密码错误: ", login_user_name);
        return;
    }
    //缓存未命中,查数据库回填
    this->myUserName = auth_user_name;

    //缓存未命中,如果查数据库查到了,就写入缓存,登录成功
    try {
        redisAPI.set_string(cache_key,
                            password,
                            std::chrono::seconds(300));
        LOG(astra_log::Level::Info, "登录成功（MySQL 校验通过，已回填缓存）: ", auth_user_name);
    } catch (const std::exception &error) {
        LOG(astra_log::Level::Warn, "写入登录缓存失败: ", error.what());
    }
}

void session::handle_create_user(const std::string &login_user_name, const std::string &password) {
    if (!createUser(mysqlAPI, login_user_name, password, sessionSocker) == true) {
        LOG(astra_log::Level::Warn, "注册未成功，跳过写入登录缓存: ", login_user_name);
        return;
    }

    const std::string cache_key = "login_cache:" + login_user_name;
    try {
        const bool cache_stored = redisAPI.set_string(cache_key, password, std::chrono::seconds(300));
        if (!cache_stored) {
            LOG(astra_log::Level::Warn, "注册后写入登录缓存失败: ", login_user_name);
        }
    } catch (std::exception &error) {
        LOG(astra_log::Level::Warn, "注册后写入登录缓存异常: ", error.what());
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
                        LOG(astra_log::Level::Debug, "收到文本消息 ", this->myUserName,
                            " -> ", receiver, " : ", text);
                        broadCast(text, receiver);
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
                    LOG(astra_log::Level::Error, "消息解析失败: ", error.what(), "  原始内容: ", line);
                }
                doRead();
            } else {
                sessionSet.erase(self);
                LOG(astra_log::Level::Info, "客户端断开，移除会话，剩余在线会话数 = ",
                    sessionSet.size(), "  (", errorCode.message(), ")");
            }
        });
}
