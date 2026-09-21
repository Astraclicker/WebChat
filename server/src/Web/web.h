#pragma once
#include <boost/asio.hpp>
#include <set>
#include <deque>
#include<def.h>
#include <json.hpp>
#include "../MySQL/MySql.h"
#include <Redis++/Redispp.h>
using namespace boost::asio::ip;

//会话类
class session : public std::enable_shared_from_this<session> {
protected:
    //mysql接口
    astra_sql::MySQLpp mysqlAPI;
    //每个会话维护一个socket
    tcp::socket sessionSocker;
    //读取缓冲区
    boost::asio::streambuf readBuf;
    //发送消息队列
    std::deque<std::string> sendMsgs;
    //维护会话集合(此处为server类中sessionSet的引用)
    std::set<std::shared_ptr<session> > &sessionSet;

    //将要发送的消息队列异步写入socket
    void doWrite();

    //从readBuffer中读取数据并调用广播函数
    void doRead();

    //广播给会话集合中的每一个客户端
    void broadCast(const std::string &msg, const std::string &receiver) const;

public:
    //本socket用户名
    std::string myUserName{};

    //构造函数
    session(tcp::socket socket,
            std::set<std::shared_ptr<session> > &sessions);

    //将消息写入发送队列
    void deliver(const std::string &msg);

    //返回会话维护的socket
    tcp::socket &getSocket() {
        return this->sessionSocker;
    }

    void start();
};

//服务器类，接受连接，管理会话
class server {
protected:
    tcp::acceptor serverAcceptor;
    std::set<std::shared_ptr<session> > sessionSet;


    void doAccept();

public:
    //构造函数
    server(boost::asio::io_context &io, short port);
};
