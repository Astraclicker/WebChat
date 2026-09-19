#include <iostream>
#include <Web/web.h>

int main()
{
    try
    {
        boost::asio::io_context io;
        // 没有设置端口转发, wsl设置相同端口可以解决超时和无法连接数据库的问题
        server chat_server(io, 9191);
        std::cout << "Chat server started on port 9191" << std::endl;
        io.run();
    }
    catch (std::exception &error)
    {
        std::cerr << "Exception: " << error.what() << std::endl;
        return 1;
    }
    return 0;
}
