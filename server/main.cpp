#include <iostream>
#include <Web/web.h>

int main()
{
    try
    {
        boost::asio::io_context io;
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
