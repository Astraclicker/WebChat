#include "web.h"
#include <iostream>

//构造函数
//不需要传入redispp,这是server的成员,让他自己初始化
server::server(boost::asio::io_context &io, const short port) : 
    redis_api(
        "127.0.0.1",
        6379,
        nullptr,
        nullptr,
        0
    ),
    serverAcceptor(io, tcp::endpoint(tcp::v4(), port)) {
        if(!redis_api.connect_check())
        {
            throw std::runtime_error("Redis connection failed");
        }
        doAccept();
}

void server::doAccept() {
    serverAcceptor.async_accept([this](const boost::system::error_code &errorCode, tcp::socket clientSocket) {
        if (!errorCode) {
            std::cout << clientSocket.remote_endpoint() << " connect to ";
            std::cout << clientSocket.local_endpoint() << std::endl;
            std::make_shared<session>(std::move(clientSocket), sessionSet,redis_api)->start();
        }
        doAccept();
    });
}
