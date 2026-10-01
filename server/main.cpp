#include <iostream>
#include <Web/web.h>
#include <fstream>
#include <log.h>

int main(int argc, char *argv[]) {
    astra_log::setLogFile("WebChat.log");
    try {
        //读取json配置,包括监听地址和端口,MySQL和Redis配置
        if (argv[1] == nullptr) {
            throw std::runtime_error("must provide config file");
        }
        std::ifstream configFile(argv[1]);
        nlohmann::json configJson = nlohmann::json::parse(configFile);

        boost::asio::io_context io;

        //创建服务器类,并运行
        server chat_server(io, configJson);
        LOG(astra_log::Level::Info, "Chat server started on port ", configJson["Web"]["port"]);
        io.run();
    } catch (std::exception &error) {
        LOG(astra_log::Level::Error, "error: ", error.what());
        return 1;
    }
    return 0;
}
