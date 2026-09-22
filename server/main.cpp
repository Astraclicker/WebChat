#include <iostream>
#include <Web/web.h>
#include <fstream>

int main(int argc, char *argv[]) {
    try {
        if (argv[1] == nullptr) {
            throw std::runtime_error("must provide config file");
        }
        std::ifstream configFile(argv[1]);
        nlohmann::json configJson = nlohmann::json::parse(configFile);

        boost::asio::io_context io;
        server chat_server(io, configJson);
        std::cout << "Chat server started on port 9191" << std::endl;
        io.run();
    } catch (std::exception &error) {
        std::cerr << "error: " << error.what() << std::endl;
        return 1;
    }
    return 0;
}
