#include "web.h"
#include <iostream>

//构造函数
server::server(boost::asio::io_context &io, nlohmann::json config)
    : redisAPI(config["Redis"]["address"],
        config["Redis"]["port"],
        config["Redis"]["userName"],
        config["Redis"]["password"],
        0
    ),
    serverAcceptor(io, tcp::endpoint(tcp::v4(), config["Web"]["port"])),
      configFile(config) {
    doAccept();
}

void server::doAccept() {
    serverAcceptor.async_accept([this](const boost::system::error_code &errorCode, tcp::socket clientSocket) {
        if (!errorCode) {
            std::cout << clientSocket.remote_endpoint() << " connect to ";
            std::cout << clientSocket.local_endpoint() << std::endl;
            std::make_shared<session>(std::move(clientSocket), configFile["MySQL"], redisAPI, sessionSet)->
                    start();
        }
        doAccept();
    });
}
