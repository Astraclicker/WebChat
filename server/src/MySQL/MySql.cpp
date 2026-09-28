#include "MySql.h"
#include <chrono>
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
    // async_write 完成前缓冲区必须存活，由回调共同持有响应字符串。
    auto send_msg = std::make_shared<std::string>(back.dump() + '\n');
    boost::asio::async_write(
        sessionSocker, boost::asio::buffer(*send_msg),
        [send_msg](const boost::system::error_code &, size_t) {
        });
}

//注册用户
bool createUser(
    astra_sql::MySQLpp &mysqlAPI,
    const std::string &loginUserName,
    const std::string &password,
    tcp::socket &sessionSocker) {
    nlohmann::json back;
    back["type"] = "mysqlCreateUserFeedBack";

    bool ok = false;
    if (loginUserName.empty() || password.empty()) {
        // 协议请求必须有且仅有一次响应；校验失败也不能直接返回让客户端一直等待。
        std::cout << "createUser failed: empty userName or password" << std::endl;
    } else {
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

    if (ok) {
        back["data"] = "success";
    } else {
        back["data"] = "failed";
    }

    // 局部字符串会提前析构，因此把异步发送缓冲区的生命周期绑定到完成回调。
    auto send_msg = std::make_shared<std::string>(back.dump() + '\n');

    boost::asio::async_write(
        sessionSocker, boost::asio::buffer(*send_msg),
        [send_msg](const boost::system::error_code &, size_t) {
        });
    return ok;
}

//保存一条聊天记录，用tableOwner生成表名
bool saveChatHistory(
    astra_sql::MySQLpp &mysqlAPI,
    const std::string &tableOwner,
    const std::string &sender,
    const std::string &text
) {
    //获取时间戳
    const std::string currentTime = std::format("{:%Y-%m-%d %H:%M:%S}", std::chrono::system_clock::now());

    const astra_sql::item chatData{
        {"sender", sender},
        {"text", text},
        {"sendTime", currentTime.c_str()}
    };
    const astra_sql::mysqlItemType chatType{
        astra_sql::mysqlDataType::String,
        astra_sql::mysqlDataType::String,
        //DataTime对应setDateTime,接受"YYYY-MM-DD HH:MM:SS"格式的字符串
        astra_sql::mysqlDataType::DataTime
    };

    const auto result = mysqlAPI.addItem(chatHistoryTableName(tableOwner), chatData, chatType);
    return result == astra_sql::SQLppError::success;
}

//先把用户名转化成表名
std::string chatHistoryTableName(const std::string &userName) {
    //表名里只保留字母数字下划线
    std::string safe;
    safe.reserve(userName.size());
    for (const char c: userName) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '_') {
            safe += c;
        } else {
            safe += '_';
        }
    }
    //设立表名的字符数上限
    if (safe.size() > 50) {
        safe.resize(50);
    }
    return "chat_history_" + safe;
}

//创建聊天记录表
void createChatHistoryTable(astra_sql::MySQLpp &mysqlAPI, const std::string &userName) {
    auto createRule = std::vector<astra_sql::createTableRule>{
        {"uid", "int", "not null auto_increment"},
        {"sender", "varchar(50)", "not null"},
        {"text", "TEXT", "not null"},
        {"sendTime", "datetime", "not null"}
    };
    const astra_sql::primaryKeyRule pk{"uid"};

    //mysqlCreateTable内部就是"create table if not exists",重复调用不报错,所以不用先查存在性
    const auto result = mysqlAPI.mysqlCreateTable(chatHistoryTableName(userName), createRule, &pk, nullptr);
    if (result != astra_sql::SQLppError::success) {
        std::cerr << "create chat history table failed: " << userName << std::endl;
    }
}