#include <iostream>
#include <Web/web.h>

int main()
{
    try
    {
        boost::asio::io_context io;
        
        // 监听端口必须与客户端固定连接的 9191 一致，否则两端都启动却永远无法建立会话。
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
