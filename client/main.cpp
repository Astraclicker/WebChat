#include <GUI/mainWidget.h>
#include <Web/client.h>
#include <thread>
#include <fstream>
#include <iostream>

int main(int argc, char *argv[]) {
    if (argv[1] == nullptr) {
        std::cerr << "must provide config file" << std::endl;
        return 1;
    }
    std::ifstream configFile(argv[1]);
    nlohmann::json configJson = nlohmann::json::parse(configFile);

    boost::asio::io_context io;
    tcp::resolver resolver(io);
    const auto endpoints = resolver.resolve(configJson["Web"]["address"], configJson["Web"]["port"]);
    chatClient chat(io, endpoints);

    std::thread client_thread([&]() {
        try {
            io.run();
        } catch (std::exception &error) {
            std::cerr << "Exception: " << error.what() << std::endl;
        }
    });
    // 创建应用程序
    const QApplication app(argc, argv);
    // 创建窗口
    mainWidget window_main(nullptr, "chat", chat);
    const int result = QApplication::exec();

    // GUI 退出后先让仍在运行的 Asio 关闭 socket，再等待网络线程结束。
    chat.close();
    client_thread.join();
    return result;
}
