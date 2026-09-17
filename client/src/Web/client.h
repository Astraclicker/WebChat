#pragma once
#include <boost/asio.hpp>
#include <string>
#include <deque>
#include<def.h>
#include<json.hpp>
using boost::asio::ip::tcp;

class chatClient {
protected:
    boost::asio::io_context &io_;
    tcp::socket clientSocket;
    boost::asio::streambuf readBuf;
    std::deque<std::string> writeMsgs;

    //连接到服务端
    void Connect(const tcp::resolver::results_type &endpoints);

    //从readBuf中读取数据
    void doRead();

    //将writeMsgs发送到服务器socket
    void doWrite();

public:
    //前端获取消息接口
    std::deque<nlohmann::json> requestedDeque;

    //构造函数
    chatClient(boost::asio::io_context &io, const tcp::resolver::results_type &endpoints);

    //由发送线程调用,把要发送的消息投递到事件循环线程
    void write(message &outgoing_message);

    //断开与服务端的连接
    void close();
};
