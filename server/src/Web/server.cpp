#include "web.h"
#include <iostream>
#include <log.h>

//构造函数
server::server(boost::asio::io_context &io, nlohmann::json config)
    : serverAcceptor(io, tcp::endpoint(tcp::v4(), config["Web"]["port"])),
      configFile(config) {
    LOG(astra_log::Level::Info, "监听端口 ", config["Web"]["port"], " 绑定成功");
    doAccept();
}

void server::doAccept() {
    serverAcceptor.async_accept([this](const boost::system::error_code &errorCode, tcp::socket clientSocket) {
        if (!errorCode) {
            // 取对端地址（用带 error_code 的重载，避免抛异常）
            boost::system::error_code endpointError;
            const auto remote = clientSocket.remote_endpoint(endpointError);
            if (endpointError) {
                LOG(astra_log::Level::Warn, "新连接接入，但读取对端地址失败: ", endpointError.message());
            } else {
                LOG(astra_log::Level::Info, "新连接接入: ", remote);
            }
            std::make_shared<session>(std::move(clientSocket), configFile["MySQL"], configFile["Redis"], sessionSet)->
                    start();
        } else {
            LOG(astra_log::Level::Error, "accept 失败: ", errorCode.message());
        }
        doAccept();
    });
}
