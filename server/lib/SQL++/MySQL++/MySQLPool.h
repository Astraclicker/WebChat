#pragma once
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <mutex>
#include <vector>

#include "MySQLpp.h"

namespace astra_sql
{
    class MySQLPool
    {
        private:
            //保护空闲队列和名额
            std::mutex poolMutex;

            struct ReturnToPool
            {
                MySQLPool * pool;
                void operator()(MySQLpp *connection);
            };
 
            std::string host;
            unsigned int port;
            std::string userName;
            std::string password;
            std::string schemaName;

            //空闲队列持有完整的mysqlpp,借出后对象会暂时离开队列
            std::vector<std::unique_ptr<MySQLpp>> idleConnectionPool;

            std::size_t reservedSlots = 0;//占用名额
            std::size_t maxConnectionSlots;//池容量

            //只负责造出已选库的对象
            std::unique_ptr<MySQLpp> createConnect();
            void returnConnection(std::unique_ptr<MySQLpp> connection);
        public:
            //lease类似一个资源借用句柄
            //Lease只能移动,统一时刻只能有一个凭证拥有这个对象
            //Lease 不只是知道“我拥有哪个连接”，它还知道“我死的时候把连接交给谁”
            //里面负责执行return动作的就交给结构体重载()
            //有点类似lambda,但是它实现lease的生命周期跨越,让lease回到调用者
            //真实对象+删除器
            using Lease = std::unique_ptr<MySQLpp,ReturnToPool>;
            //优先取空闲对象,没有空闲而且依然有名额的时候才调用createConnect
            Lease borrow();

            MySQLPool(const std::string &host, unsigned int port, 
            const std::string &userName, const std::string &password,
            std::string schemaName,std::size_t capacity);
    };
}