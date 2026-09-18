#include "web.h"
#include <cstdlib>
#include <iostream>
#include <memory>
#include "../MySQL/MySql.h"

namespace
{
    // 连接信息由启动进程注入，避免把开发者账号和密码硬编码进仓库。
    std::string get_environment_value(const char *name, const char *fallback)
    {
        const char *value = std::getenv(name);
        return value == nullptr ? fallback : value;
    }

    unsigned int get_database_port()
    {
        return static_cast<unsigned int>(std::stoul(
            get_environment_value("WEBCHAT_DB_PORT", "3306")
        ));
    }
}

//构造函数
session::session(tcp::socket socket, std::set<std::shared_ptr<session> > &sessions)
    // 学习项目仍为“每个会话一条同步数据库连接”；并发扩大后会阻塞 Asio 线程，后续再引入连接池。
    : mysqlAPI(
          get_environment_value("WEBCHAT_DB_HOST", "127.0.0.1"),
          get_database_port(),
          get_environment_value("WEBCHAT_DB_USER", "webchat"),
          get_environment_value("WEBCHAT_DB_PASSWORD", "")
      ),
      sessionSocker(std::move(socket)),
      sessionSet(sessions)
{
    mysqlAPI.switchDatabase(get_environment_value("WEBCHAT_DB_NAME", "ChatServer"));
}

//启动接口
void session::start() {
    sessionSet.insert(shared_from_this());
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
        // 原设计排除了发送者：只有一个客户端时，即使给自己发消息也永远没有反馈。
        // 学习版仍固定发送给 root，但把消息同时回显给发送者，形成最小可观察闭环。
        if (session.get() == this || session->myUserName == receiver) {
            session->deliver(frame);
            std::cout << this->sessionSocker.remote_endpoint() << " send to ";
            std::cout << session->sessionSocker.remote_endpoint() << std::endl;
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
                sessionSet.erase(self);
            }
        });
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
                        broadCast(text, receiver);
                    } else {
                        const auto loginUserName = data.value("userName", std::string{});
                        const auto password = data.value("password", std::string{});
                        if (type == "loginRequested") {
                            login(mysqlAPI, loginUserName, password, sessionSocker, this->myUserName);
                        } else {
                            createUser(mysqlAPI, loginUserName, password, sessionSocker);
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
