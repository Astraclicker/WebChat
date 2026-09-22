#include "web.h"
#include <cstdlib>
#include <iostream>
#include <memory>
#include "../MySQL/MySql.h"
#include <chrono>

//构造函数
session::session(tcp::socket socket, std::set<std::shared_ptr<session> > &sessions,astra_sql::Redispp &redis)
    : mysqlAPI(
          "127.0.0.1",
          astra_sql::MySQL_DEFAULT_PORT,
          "astraclicker",
          "1108372699a@A"
      ),
      sessionSocker(std::move(socket)),
      sessionSet(sessions),
      redisAPI(redis)
      {
    auto createRule = std::vector<astra_sql::createTableRule>{
        {"uid", "int", "not null auto_increment"},
        {"userName", "varchar(50)", "not null"},
        {"password", "varchar(50)", "not null"}
    };
    const astra_sql::primaryKeyRule pk{"uid"};
    const astra_sql::uniqueKeyRule uk{"userName"};
    mysqlAPI.switchDatabase("ChatServer");
    mysqlAPI.mysqlCreateTable("users", createRule, &pk, &uk);
}

//启动接口
void session::start() {
    sessionSet.insert(shared_from_this());
    doRead();
}

//广播给会话集合中匹配用户名的客户端
void session::broadCast(const std::string &msg, const std::string &receiver) const {
    nlohmann::json out;
    out["type"] = "text";
    out["data"]["receiver"] = receiver;
    out["data"]["sender"] = this->myUserName;
    out["data"]["text"] = msg;
    const std::string frame = out.dump() + "\n";

    for (auto &session: sessionSet) {
        //好友和群组功能作为拓展功能，目前不校验receiver，只排排除自己
        //TODO 校验receiver(作为拓展功能)
        if (session != this->shared_from_this()) {
            session->deliver(frame);
            std::cout << this->sessionSocker.remote_endpoint() << " send to ";
            std::cout << session->sessionSocker.remote_endpoint() << std::endl;
        }
    }
}

//将消息写入接收队列
void session::deliver(const std::string &msg) {
    const bool sendProgress = !sendMsgs.empty();
    sendMsgs.push_back(msg);
    if (!sendProgress) {
        doWrite();
    }
}

//将要发送的消息队列异步写入socket
void session::doWrite() {
    auto self(shared_from_this());
    boost::asio::async_write(
        sessionSocker,
        boost::asio::buffer(sendMsgs.front().data(), sendMsgs.front().size()),
        [this,self](const boost::system::error_code &errorCode, size_t) {
            if (!errorCode) {
                sendMsgs.pop_front();
                if (!sendMsgs.empty()) {
                    doWrite();
                }
            } else {
                sessionSet.erase(self);
            }
        });
}

void session::handle_login(const std::string &login_user_name,const std::string &password)
{
    const std::string cache_key = "login_cache:" + login_user_name;
    try
    {
        const auto cached_password = redisAPI.get_string(cache_key);
        if(cached_password && *cached_password == password)
        {

            //缓存命中在更新myUserName之前先更新生存时间
            //这里需要用ttl_refreshed稳稳接住,因为这次操作之前可能就过期了
            //expire的操作可能无法完成,所以我们要记录
            bool ttl_refreshed = this->redisAPI.expire_key(cache_key,std::chrono::seconds(300));
            if(!ttl_refreshed)
            {
                std::cerr << "login cache expired before TTL refresh: "
                << login_user_name << std::endl;
            }
            this->myUserName = login_user_name;

            nlohmann::json response;
            //原本这里由数据库相关逻辑执行,现在交给redis
            response["type"] = "mysqlLoginFeedBack";
            response["data"] = "success";
            deliver(response.dump() + "\n");

            std::cout << "Login cache hit: "
            << login_user_name << std::endl;

            //缓存命中登录成功
            std::cout << "login success" << std::endl;
            return;
        }
    }
    catch(const std::exception &error)
    {
        std::cerr << "failed to read login cache, fallback to MySQL: "
        << error.what() << std::endl;
    }

    std::string auth_user_name;
    login(mysqlAPI,
        login_user_name,password,
        sessionSocker,
        auth_user_name);
    if(auth_user_name.empty())//缓存未命中,查数据库查不到,登录失败
    {
        return;
    }
    //缓存未命中,查数据库回填
    this->myUserName = auth_user_name;

    //缓存未命中,如果查数据库查到了,就写入缓存,登录成功
    try
    {
        redisAPI.set_string(cache_key,
            password,
            std::chrono::seconds(300));
        std::cout << "login cache stored: "
        << auth_user_name
        << std::endl;
    }
    catch(const std::exception &error)
    {
        std::cerr << "fail to store login cache: "
        << error.what()
        << std::endl;
    }



}
void session::handle_create_user(const std::string &login_user_name,const std::string &password)
{
    if(!createUser(mysqlAPI,login_user_name,password,sessionSocker) == true)
    {
        return;
    }

    const std::string cache_key = "login_cache:" + login_user_name;
    try
    {
        const bool cache_stored = redisAPI.set_string(cache_key,password,std::chrono::seconds(300));
        if(!cache_stored)
        {
            std::cerr << "fail to store login cache when create user" << login_user_name << std::endl;
        }
    }
    catch(std::exception &error)
    {
        std::cerr << "fail to store login cache when create user"
        << error.what() << std::endl;
    }
}
//从readBuffer中读取数据并调用广播函数
void session::doRead() {
    auto self(shared_from_this());
    boost::asio::async_read_until(
        sessionSocker,
        readBuf,
        '\n',
        [this,self](const boost::system::error_code &errorCode, std::size_t length) {
            if (!errorCode) {
                std::istream is(&readBuf);
                std::string line;
                std::getline(is, line);

                try {
                    const auto readMsg = nlohmann::json::parse(line);
                    const auto type = readMsg.value("type", std::string{});
                    const auto data = readMsg.value("data", nlohmann::json::object());

                    if (type == "text") {
                        const auto receiver = data.value("receiver", std::string{});
                        const auto text = data.value("text", std::string{});
                        broadCast(text, receiver);
                    } else {
                        const auto loginUserName = data.value("userName", std::string{});
                        const auto password = data.value("password", std::string{});
                        if (type == "loginRequested") {
                            handle_login( loginUserName, password);
                        } else {
                            handle_create_user(loginUserName,password);
                        }
                    }
                } catch (const std::exception &error) {
                    std::cerr << error.what() << std::endl;
                }
                doRead();
            } else {
                sessionSet.erase(self);
            }
        });
}
