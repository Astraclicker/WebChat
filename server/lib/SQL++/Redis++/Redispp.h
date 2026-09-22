#pragma once
#include <string>
#include <memory>//提供智能指针
#include <chrono>//明确单位超时
#include <optional>

 

#include "../include/SQL.h"

#include <sw/redis++/redis++.h>

namespace astra_sql {
    class Redispp {
    private:

        // Redis操作接口
        std::unique_ptr<sw::redis::Redis> redis_client;

    public:
        /**
         * @brief Redis数据访问封装类
         * @details
         * 为web_chat业务逻辑提供Redis数据读写接口 
         */
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

        /**
         * @brief 读取键得到值
         * @param key 要读取的Redis键
         * @return 键存在时返回字符串,否则返回std::nullopt
         * @throw Redis连接或者命令异常
         */
        //使用这个类型来判断缓存是否命中
        std::optional<std::string> get_string(const std::string &key);

        /**
         * @brief 设置指定键的值和ttl
         */
        bool set_string(const std::string &key,
            const std::string &value,
            const std::chrono::seconds ttl );
        
        /**
         * @brief 删除指定键,删除成功返回true
         */
        bool delete_key(const std::string &key);

        /**
         * @brief:为redis添加健康检查,服务器在redis可用的时候接收客户端
         */
        bool connect_check();

    };
}
