#include "MySQLPool.h"
#include <utility>
namespace astra_sql
{
    MySQLPool::MySQLPool(const std::string &host, unsigned int port, 
        const std::string &userName, const std::string &password,
        std::string schemaName,std::size_t capacity):
        host(host),port(port),userName(userName),
        password(password),schemaName(schemaName),
        maxConnectionSlots(capacity)
    {
        if (capacity == 0 || schemaName.empty())
        {
            throw std::invalid_argument("MySQL pool needs a positive capacity and schema");
        }
    }

    std::unique_ptr<MySQLpp> MySQLPool::createConnect()
    {
        auto connection = std::make_unique<MySQLpp>
        (
            host,port,userName,password
        );
        if(connection->switchDatabase(schemaName)!=SQLppError::success)
        {
            throw std::runtime_error("fail to select schema for mysql pool connection");
        }
        return connection;
    }

    void MySQLPool::returnConnection(std::unique_ptr<MySQLpp> conneciton)
    {
        //避免两个线程同时修改空闲队列
        std::lock_guard<std::mutex> lock(poolMutex);

        //move后,调用方之后不再持有对象,由空闲队列接管
        idleConnectionPool.push_back(std::move(conneciton));

        //对象仍然存在,所以占用名额不变

    }
    MySQLPool::Lease MySQLPool::borrow()
    {
        std::unique_ptr<MySQLpp> connection;
        {
            std::lock_guard<std::mutex> lock(poolMutex);
            //如果当前有空闲队列的话就直接用
            if(!idleConnectionPool.empty())
            {
                connection = std::move(idleConnectionPool.back());
                idleConnectionPool.pop_back();
            }
            //如果没有空闲队列而且已经到容量极限了
            else if(reservedSlots < maxConnectionSlots)
            {
                //先占住名额再新建连接,防止多个线程同时创建越过上限
                //其实我觉得slots这个设计也许肉眼可见的有瓶颈,但是先不管了
                ++reservedSlots;
            }
            else
            {
                //如果先不加等待机制,池满了就报告失败,仅为测试
                throw std::runtime_error("mysql pool is full");
            }
        }
        
        //现在处理slots++后的情况
        if(!connection)
        {
            try
            {
                connection = createConnect();
            }
            catch(...)
            {
                std::lock_guard<std::mutex> lock(poolMutex);
                --reservedSlots;
                throw;
            }
        }
        // Lease接管对象,离开作用域时由ReturnToPool自动归还。
        return Lease(connection.release(),ReturnToPool{this});
    }
    void MySQLPool::ReturnToPool::operator()(MySQLpp * connection)
    {
        if(connection != nullptr)
        {
            pool->returnConnection(std::unique_ptr<MySQLpp>(connection));
        }
    }
}