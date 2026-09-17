#pragma once
#include <MySQL++/MySQLpp.h>
#include <boost/asio.hpp>
using namespace boost::asio::ip;

//登录
void login(astra_sql::MySQLpp &mysqlAPI, const std::string &loginUserName, const std::string &password,
           tcp::socket &sessionSocker, std::string &userName);

//注册用户
void createUser(astra_sql::MySQLpp &mysqlAPI, const std::string &loginUserName, const std::string &password,
                tcp::socket &sessionSocker);
