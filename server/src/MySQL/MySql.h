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
