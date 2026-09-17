#pragma once
#include <string>
#include<iostream>

#include "../include/SQL.h"

#include <sw/redis++/redis++.h>

namespace astra_sql {
    class Redispp {
    private:
        // Redis连接配置
        sw::redis::ConnectionOptions *opts;
        // Redis操作接口
        sw::redis::Redis *redis;

    public:
        /**
         * @brief 构造函数
         * @param hostName host地址
         * @param port 端口
         * @param userName 用户名指针(没有传入nullptr)
         * @param password 密码(没有传入nullptr)
         * @param db 数据库编号
         */
        Redispp(
            const std::string &hostName,
            int port,
            const std::string *userName,
            const std::string *password,
            int db
        );
    };
}
