#include "web.h"
#include <iostream>

//构造函数
server::server(boost::asio::io_context &io, const short port) : serverAcceptor(io, tcp::endpoint(tcp::v4(), port)) {
    doAccept();
}

void server::doAccept() {
    serverAcceptor.async_accept([this](const boost::system::error_code &errorCode, tcp::socket clientSocket) {
        if (!errorCode) {
            std::cout << clientSocket.remote_endpoint() << " connect to ";
            std::cout << clientSocket.local_endpoint() << std::endl;
            std::make_shared<session>(std::move(clientSocket), sessionSet)->start();
        }
        doAccept();
    });
}
