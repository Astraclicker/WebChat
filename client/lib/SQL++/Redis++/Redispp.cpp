#include "Redispp.h"

namespace astra_sql {
    Redispp::Redispp(
        const std::string &hostName,
        const int port,
        const std::string *userName,
        const std::string *password,
        const int db
    ) {
        opts = new sw::redis::ConnectionOptions;
        opts->host = hostName;
        opts->port = port;
        if (userName != nullptr) {
            opts->user = *userName;
        }
        if (password != nullptr) {
            opts->password = *password;
        }

        opts->db = db;

        try {
            redis = new sw::redis::Redis(*opts);
            redis->ping();
        } catch (const std::exception &e) {
            std::cerr << e.what() << '\n';
            return;
        }

        std::clog << "Redis connect successfully" << std::endl;
    }
}
