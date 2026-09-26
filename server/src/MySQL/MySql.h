#pragma once
#include <MySQL++/MySQLpp.h>
#include <boost/asio.hpp>
using namespace boost::asio::ip;

//登录
void login(astra_sql::MySQLpp &mysqlAPI, const std::string &loginUserName, const std::string &password,
           tcp::socket &sessionSocker, std::string &userName);

//注册用户
//因为没有auth_user_name参数的存在来判断是否成功,重构一下,否则redis逻辑不好写
bool createUser(astra_sql::MySQLpp &mysqlAPI, const std::string &loginUserName, const std::string &password,
                tcp::socket &sessionSocker);

//保存一条聊天记录到chat_history,写入成功返回true
//时间戳由服务端生成,不接受客户端传入
bool saveChatHistory(astra_sql::MySQLpp &mysqlAPI, const std::string &sender, const std::string &text);

//TODO创建用于存储聊天记录的表(每个用户一张)
void createChatHistoryTable(astra_sql::MySQLpp &mysqlAPI, const std::string &userName);