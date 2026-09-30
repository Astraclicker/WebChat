#pragma once
#include <MySQL++/MySQLpp.h>
#include <boost/asio.hpp>
using namespace boost::asio::ip;

//登录
void login(astra_sql::MySQLpp &mysqlAPI, const std::string &loginUserName, const std::string &password,
           std::string &userName, long long &uid, std::string &frame);

//注册用户
bool createUser(astra_sql::MySQLpp &mysqlAPI, const std::string &loginUserName, const std::string &password,
                long long &uid, std::string &frame);

//创建聊天记录表(全服就这一张)
void createChatHistoryTable(astra_sql::MySQLpp &mysqlAPI);

//把一条聊天记录写进聊天记录表
bool saveChatHistory(astra_sql::MySQLpp &mysqlAPI,
                     const std::string &sender, const std::string &text, const std::string &sendTime);

nlohmann::json fetchLatestChatRows(astra_sql::MySQLpp &mysqlAPI, int limit);
