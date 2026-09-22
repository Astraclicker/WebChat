#include "Redispp.h"

namespace astra_sql {
    Redispp::Redispp(
        const std::string &hostName,
        const int port,
        const std::string *userName,
        const std::string *password,
        const int db
    ) {
        // opts = new sw::redis::ConnectionOptions;
        //使用临时对象

        sw::redis::ConnectionOptions connection_options;
        connection_options.host = hostName;
        connection_options.port = port;
        if (userName != nullptr) {
            connection_options.user = *userName;
        }
        if (password != nullptr) {
            connection_options.password = *password;
        }
        connection_options.db = db;

        //如果超时
        connection_options.connect_timeout = std::chrono::milliseconds(1000);
        connection_options.socket_timeout = std::chrono::milliseconds(1000);


        redis_client = std::make_unique<sw::redis::Redis>(connection_options);
            // redis_client->ping();
            //使用懒连接
            //把构造和连接分开,ping应该作为独立的其他的健康检查
            //PING 成功不保证下一毫秒的 GET 一定成功,直接get即可
            //GET 本身就会尝试连接并报告异常。
        //如果try catch捕获异常return这样返回依然会导致构造函数发生,然后对象被建立
        //日志往外面放一下
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
