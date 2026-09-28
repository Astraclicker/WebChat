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

//把用户名转成聊天记录表名: chat_history_<用户名>
std::string chatHistoryTableName(const std::string &userName);

//创建用于存储聊天记录的表(每个用户一张)
void createChatHistoryTable(astra_sql::MySQLpp &mysqlAPI, const std::string &userName);

//把一条聊天记录写进 tableOwner 的那张表
bool saveChatHistory(astra_sql::MySQLpp &mysqlAPI, const std::string &tableOwner,
                     const std::string &sender, const std::string &text);