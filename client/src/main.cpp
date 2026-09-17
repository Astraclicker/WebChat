#include <GUI/mainWidget.h>
#include <Web/client.h>
#include <thread>
#include <iostream>

int main(int argc, char *argv[])
{
    boost::asio::io_context io;
    tcp::resolver resolver(io);
    const auto endpoints = resolver.resolve("127.0.0.1", "9191");
    chatClient chat(io, endpoints);

    std::thread client_thread([&]()
    {
        try
        {
            io.run();
        }
        catch (std::exception &error)
        {
            std::cerr << "Exception: " << error.what() << std::endl;
        }
    });
    // 创建应用程序
    const QApplication app(argc, argv);
    // 创建窗口
    mainWidget window_main(nullptr, "chat", chat);
    const int result = QApplication::exec();
    chat.close();
    client_thread.join();
    return result;
}
