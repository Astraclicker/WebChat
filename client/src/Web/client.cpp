#include "client.h"
#include "../SQLite/SQLite.h"
#include <iostream>
#include <memory>

//构造函数
chatClient::chatClient(
    boost::asio::io_context &io,
    const tcp::resolver::results_type &endpoints
) : io_(io), clientSocket(io) {
    //TODO sqlite创表 my
    Connect(endpoints);
}

void chatClient::Connect(const tcp::resolver::results_type &endpoints) {
    boost::asio::async_connect(
        clientSocket,
        endpoints,
        [this](const boost::system::error_code &errorCode, const tcp::endpoint &) {
            if (!errorCode) {
                std::cout << "[client]" << "connect to " << clientSocket.remote_endpoint() << std::endl;
                doRead();
            } else {
                std::cerr << "[client]connect failed: " << errorCode.message() << std::endl;
            }
        }
    );
}

//将clientSocket中的数据读取到readBuf
void chatClient::doRead() {
    boost::asio::async_read_until(
        clientSocket,
        readBuf,
        '\n',
        [this](const boost::system::error_code &errorCode, std::size_t) {
            //从clientSocket读取完后,把readBuf中的数据解析为json并投递到requested_messages
            if (!errorCode) {
                std::istream is(&readBuf);
                std::string line;
                std::getline(is, line);
                try {
                    const auto readMsg = nlohmann::json::parse(line);
                    // 完整 JSON 解析完成后再短暂持锁，避免阻塞 Qt 线程。
                    std::lock_guard lock(requested_mutex);
                    requested_messages.push_back(readMsg);
                } catch (std::exception &error) {
                    std::cerr << error.what() << std::endl;
                }
                doRead();
            } else {
                std::cerr << "[client]read error: " << errorCode.message() << std::endl;
            }
        });
}

//根据message的类型来构造json对象，并将其投递到事件循环线程
void chatClient::write(message &outgoing_message) {
    nlohmann::json sendJson;
    switch (outgoing_message.type) {
        case message_type::text: {
            sendJson["type"] = "text";
            sendJson["data"] = nlohmann::json::parse(outgoing_message.data);
            break;
        }
        case message_type::login_requested: {
            sendJson["type"] = "loginRequested";
            sendJson["data"] = nlohmann::json::parse(outgoing_message.data);
            break;
        }
        case message_type::create_user_requested: {
            sendJson["type"] = "createUserRequested";
            sendJson["data"] = nlohmann::json::parse(outgoing_message.data);
            break;
        }
    }

    boost::asio::post(io_, [this, sendJson]() {
        const bool writeInProgress = !writeMsgs.empty();
        writeMsgs.push_back(sendJson);
        if (!writeInProgress) {
            doWrite();
        }
    });
}

//将writeMsgs中的第一条数据写入clientSocket
void chatClient::doWrite() {
    if (writeMsgs.empty()) {
        return;
    }

    auto payload = std::make_shared<std::string>(writeMsgs.front().dump() + '\n');
    boost::asio::async_write(
        clientSocket,
        boost::asio::buffer(*payload),
        //写入完成调用回调函数,弹出第一条数据
        [this, payload](const boost::system::error_code &errorCode, std::size_t) {
            if (!errorCode) {
                writeMsgs.pop_front();
                if (!writeMsgs.empty()) {
                    doWrite();
                }
            } else {
                std::cerr << "[client]write error: " << errorCode.message() << std::endl;
            }
        });
}

//关闭本客户端连接
void chatClient::close() {
    boost::asio::post(io_, [this]() {
        clientSocket.close();
    });
}

bool chatClient::try_pop_message(nlohmann::json &incoming_message) {
    std::lock_guard lock(requested_mutex);
    if (requested_messages.empty()) {
        return false;
    }

    incoming_message = std::move(requested_messages.front());
    requested_messages.pop_front();
    return true;
}
