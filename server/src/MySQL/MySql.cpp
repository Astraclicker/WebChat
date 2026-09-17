#include "MySql.h"
#include <iostream>
#include <vector>

//登录
void login(
    astra_sql::MySQLpp &mysqlAPI,
    const std::string &loginUserName,
    const std::string &password,
    tcp::socket &sessionSocker,
    std::string &userName
) {
    const std::vector<std::string> userData{
        "userName",
        "password"
    };
    const astra_sql::itemRule rule{
        {"userName", "=", loginUserName, "and"},
        {"password", "=", password, "and"}
    };
    const auto json = mysqlAPI.searchItem("users", userData, rule);
    const bool ok = json.is_object() && !json.value("userName", nlohmann::json::array()).empty();

    nlohmann::json back;
    back["type"] = "mysqlLoginFeedBack";
    if (ok) {
        back["data"] = "success";
        userName = loginUserName;
    } else {
        back["data"] = "failed";
    }
    auto sendMsg = back.dump() + '\n';
    boost::asio::async_write(
        sessionSocker, boost::asio::buffer(sendMsg),
        [](const boost::system::error_code &, size_t) {
        });
}

//注册用户
void createUser(
    astra_sql::MySQLpp &mysqlAPI,
    const std::string &loginUserName,
    const std::string &password,
    tcp::socket &sessionSocker) {
    if (loginUserName.empty() || password.empty()) {
        std::cout << "createUser failed: empty userName or password" << std::endl;
        return;
    }

    const astra_sql::item userData{
        {"userName", loginUserName},
        {"password", password}
    };
    const astra_sql::mysqlItemType userType{
        astra_sql::mysqlDataType::String,
        astra_sql::mysqlDataType::String,
    };
    const auto result = mysqlAPI.addItem("users", userData, userType);

    const bool ok = result == astra_sql::SQLppError::success;
    nlohmann::json back;
    back["type"] = "mysqlCreateUserFeedBack";
    if (ok) {
        back["data"] = "success";
    } else {
        back["data"] = "failed";
    }

    auto sendMsg = back.dump() + '\n';

    boost::asio::async_write(
        sessionSocker, boost::asio::buffer(sendMsg),
        [](const boost::system::error_code &, size_t) {
        });
}
