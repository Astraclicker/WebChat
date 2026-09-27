#include "Redispp.h"

namespace astra_sql {
    Redispp::Redispp(
        const std::string &hostName,
        const int port,
        const std::string &userName,
        const std::string &password,
        const int db
    ) {
        // opts = new sw::redis::ConnectionOptions;
        //使用临时对象
        sw::redis::ConnectionOptions connection_options;
        sw::redis::ConnectionPoolOptions conneciton_pool_option;
        conneciton_pool_option.size = 4;//池总量
        conneciton_pool_option.wait_timeout = std::chrono::milliseconds(500);
        connection_options.host = hostName;
        connection_options.port = port;
        if (!userName.empty()) {
            connection_options.user = userName;
        }
        if (!userName.empty()) {
            connection_options.password = password;
        }
        connection_options.db = db;

        //如果超时
        connection_options.connect_timeout = std::chrono::milliseconds(1000);
        connection_options.socket_timeout = std::chrono::milliseconds(1000);


        redis_client = std::make_unique<sw::redis::Redis>
        (connection_options,conneciton_pool_option);
    }

    std::optional<std::string> Redispp::get_string(const std::string &key)
    {
        const auto redis_value = redis_client->get(key);
        if(!redis_value)
        {
            return std::nullopt;
        }
        return *redis_value;
    }

    bool Redispp::set_string(const std::string &key,
    const std::string &value,
    const std::chrono::seconds ttl)
    {
        return redis_client->set(key,value,ttl);
    }

    bool Redispp::delete_key(const std::string &key)
    {
        const long long deleted_count = redis_client->del(key);
        return deleted_count > 0;
    }

    bool Redispp::connect_check()
    {
        return redis_client->ping() == "PONG";
    }
    bool Redispp::expire_key(const std::string &key,std::chrono::seconds ttl)
    {
        return redis_client->expire(key,ttl);
    }
}
