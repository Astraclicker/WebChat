#include <iostream>
#include <Web/web.h>
#include <fstream>

int main(int argc, char *argv[]) {
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
        std::cout << "Chat server started on port " << configJson["Web"]["port"] << std::endl;
        io.run();
    } catch (std::exception &error) {
        std::cerr << "error: " << error.what() << std::endl;
        return 1;
    }
    return 0;
}
