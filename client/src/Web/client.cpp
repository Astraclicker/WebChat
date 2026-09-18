#include "client.h"
#include <iostream>

//构造函数
chatClient::chatClient(
    boost::asio::io_context &io,
    const tcp::resolver::results_type &endpoints
) : io_(io), clientSocket(io) {
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

void chatClient::doRead() {
    boost::asio::async_read_until(
        clientSocket,
        readBuf,
        '\n',
        [this](const boost::system::error_code &errorCode, std::size_t) {
            if (!errorCode) {
                std::istream is(&readBuf);
                std::string line;
                std::getline(is, line);
                try {
                    const auto readMsg = nlohmann::json::parse(line);
                    // 完整 JSON 解析完成后再短暂持锁，避免阻塞 Qt 线程。
                    std::lock_guard<std::mutex> lock(requested_mutex);
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

void chatClient::doWrite() {
    if (writeMsgs.empty()) {
        return;
    }
    boost::asio::async_write(
        clientSocket,
        boost::asio::buffer(writeMsgs.front().data(), writeMsgs.front().size()),
        [this](const boost::system::error_code &ec, std::size_t) {
            if (!ec) {
                writeMsgs.pop_front();
                if (!writeMsgs.empty()) {
                    doWrite();
                }
            } else {
                std::cerr << "[client]write error: " << ec.message() << std::endl;
            }
        });
}

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
        writeMsgs.push_back(sendJson.dump() + '\n');
        if (!writeInProgress) {
            doWrite();
        }
    });
}

void chatClient::close() {
    boost::asio::post(io_, [this]() {
        clientSocket.close();
    });
}

bool chatClient::try_pop_message(nlohmann::json &incoming_message)
{
    std::lock_guard<std::mutex> lock(requested_mutex);
    if (requested_messages.empty())
    {
        return false;
    }

    incoming_message = std::move(requested_messages.front());
    requested_messages.pop_front();
    return true;
}
