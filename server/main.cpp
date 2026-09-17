#include <iostream>
#include <Web/web.h>

int main() {
    try {
        boost::asio::io_context io;
        server Server(io, 8080);
        std::cout << "Chat server started on port 8080" << std::endl;
        io.run();
    } catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}
