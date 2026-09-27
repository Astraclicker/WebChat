#include <iostream>
#include <Web/web.h>
#include <fstream>
#include <log.h>

int main(int argc, char *argv[]) {
    // 日志写到当前工作目录下的 server.log。想换路径就调 setLogFile。
    astra_log::setLogFile("server.log");
    // 想看全部日志用 Debug；上线想安静点就改成 Level::Info
    astra_log::setMinLevel(astra_log::Level::Debug);

    LOG(astra_log::Level::Info, "========== WebChat 服务端启动 ==========");

    try {
        if (argv[1] == nullptr) {
            throw std::runtime_error("must provide config file");
        }
        std::ifstream configFile(argv[1]);
        nlohmann::json configJson = nlohmann::json::parse(configFile);

        boost::asio::io_context io;
        server chat_server(io, configJson);
        LOG(astra_log::Level::Info, "已开始监听端口 ", configJson["Web"]["port"], "，等待客户端连接...");
        io.run();
        LOG(astra_log::Level::Info, "事件循环结束，服务端退出");
    } catch (std::exception &error) {
        LOG(astra_log::Level::Error, "服务端异常退出: ", error.what());
        return 1;
    }

    LOG(astra_log::Level::Info, "========== WebChat 服务端关闭 ==========");
    return 0;
}
