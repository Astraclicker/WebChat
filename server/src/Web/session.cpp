#include "web.h"
#include <cstdlib>
#include <iostream>
#include <memory>
#include "../MySQL/MySql.h"

//构造函数
session::session(tcp::socket socket, std::set<std::shared_ptr<session> > &sessions)
    : mysqlAPI(
          "127.0.0.1",
          astra_sql::MySQL_DEFAULT_PORT,
          "astraclicker",
          "1108372699a@A"
      ),
      sessionSocker(std::move(socket)),
      sessionSet(sessions) {
    auto createRule = std::vector<astra_sql::createTableRule>{
        {"uid", "int", "not null auto_increment"},
        {"userName", "varchar(50)", "not null"},
        {"password", "varchar(50)", "not null"}
    };
    const astra_sql::primaryKeyRule pk{"uid"};
    const astra_sql::uniqueKeyRule uk{"userName"};
    mysqlAPI.switchDatabase("ChatServer");
    mysqlAPI.mysqlCreateTable("users", createRule, &pk, &uk);
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
        //好友和群组功能作为拓展功能，目前不校验receiver，只排排除自己
        //TODO 校验receiver(作为拓展功能)
        if (session != this->shared_from_this()) {
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
