#pragma once
#include <boost/asio.hpp>
#include <string>
#include <deque>
#include <mutex>
#include<def.h>
#include<json.hpp>
using boost::asio::ip::tcp;

class chatClient {
protected:
    boost::asio::io_context &io_;
    tcp::socket clientSocket;
    boost::asio::streambuf readBuf;

    //保存发送的消息的队列
    std::deque<std::string> writeMsgs;

    // Asio 与 Qt 分属不同线程，接收队列通过互斥保护的接口访问。
    std::mutex requested_mutex;

    //保存接收的消息的队列
    std::deque<nlohmann::json> requested_messages;

    //连接到服务端
    void Connect(const tcp::resolver::results_type &endpoints);

    //从readBuf中读取数据
    void doRead();

    //将writeMsgs发送到服务器socket
    void doWrite();

public:
    //构造函数
    chatClient(boost::asio::io_context &io, const tcp::resolver::results_type &endpoints);

    //由发送线程调用,把要发送的消息投递到事件循环线程
    void write(message &outgoing_message);

    //断开与服务端的连接
    void close();

    // Qt 线程以非阻塞方式取走一条消息；队列为空时立即返回。
    bool try_pop_message(nlohmann::json &incoming_message);
};
