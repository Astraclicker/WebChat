#include "MySql.h"
#include <iostream>
#include <memory>
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
    auto send_msg = std::make_shared<std::string>(back.dump() + '\n');
    boost::asio::async_write(
        sessionSocker, boost::asio::buffer(*send_msg),
        [send_msg](const boost::system::error_code &, size_t) {
        });
}

//注册用户
void createUser(
    astra_sql::MySQLpp &mysqlAPI,
    const std::string &loginUserName,
    const std::string &password,
    tcp::socket &sessionSocker)
{
    nlohmann::json back;
    back["type"] = "mysqlCreateUserFeedBack";

    bool ok = false;
    if (loginUserName.empty() || password.empty())
    {
        // 协议请求必须有且仅有一次响应；校验失败也不能直接返回让客户端一直等待。
        std::cout << "createUser failed: empty userName or password" << std::endl;
    }
    else
    {
        const astra_sql::item userData{
            {"userName", loginUserName},
            {"password", password}
        };
        const astra_sql::mysqlItemType userType{
            astra_sql::mysqlDataType::String,
            astra_sql::mysqlDataType::String,
        };
        const auto result = mysqlAPI.addItem("users", userData, userType);
        ok = result == astra_sql::SQLppError::success;
    }

    if (ok)
    {
        back["data"] = "success";
    }
    else
    {
        back["data"] = "failed";
    }

    auto send_msg = std::make_shared<std::string>(back.dump() + '\n');

    boost::asio::async_write(
        sessionSocker, boost::asio::buffer(*send_msg),
        [send_msg](const boost::system::error_code &, size_t) {
        });
}
