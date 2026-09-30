#include "MySql.h"
#include <iostream>
#include <memory>
#include <vector>
#include <def.h>

namespace {
    constexpr auto kChatTableName = "chat_messages";
}

//登录
void login(
    astra_sql::MySQLpp &mysqlAPI,
    const std::string &loginUserName,
    const std::string &password,
    std::string &userName,
    long long &uid,
    std::string &frame
) {
    const std::vector<std::string> userData{
        "uid",
        "userName",
        "password"
    };
    const astra_sql::itemRule rule{
        {"userName", "=", loginUserName, "and"},
        {"password", "=", password, "and"}
    };
    const auto json = mysqlAPI.searchItem("users", userData, rule);

    //searchItem 把每一列都按字符串数组返回
    const auto uid_list = json.is_object() ? json.value("uid", nlohmann::json::array()) : nlohmann::json::array();
    const bool ok = json.is_object() && !json.value("userName", nlohmann::json::array()).empty() && !uid_list.empty();

    nlohmann::json back;
    back["type"] = "mysqlLoginFeedBack";
    if (ok) {
        back["data"] = "success";
        userName = loginUserName;
        try {
            uid = std::stoll(uid_list.front().get<std::string>());
        } catch (const std::exception &) {
            uid = 0;
        }
        back["uid"] = uid;
    } else {
        back["data"] = "failed";
    }
    frame = back.dump() + '\n';
}

//注册用户
bool createUser(
    astra_sql::MySQLpp &mysqlAPI,
    const std::string &loginUserName,
    const std::string &password,
    long long &uid,
    std::string &frame) {
    nlohmann::json back;
    back["type"] = "mysqlCreateUserFeedBack";

    bool ok = false;
    long long new_uid = 0;
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

        if (ok) {
            //addItem 不回传自增主键,插入成功后按用户名把新 uid 查回来
            const auto json = mysqlAPI.searchItem(
                "users",
                {"uid"},
                {{"userName", "=", loginUserName, "and"}});
            const auto uid_list = json.is_object()
                                      ? json.value("uid", nlohmann::json::array())
                                      : nlohmann::json::array();
            if (!uid_list.empty()) {
                try {
                    new_uid = std::stoll(uid_list.front().get<std::string>());
                } catch (const std::exception &) {
                    new_uid = 0;
                }
            }
        }
    }

    uid = new_uid;

    if (ok) {
        back["data"] = "success";
        //客户端要用 uid 命名本地 SQLite 库,所以和注册结果一起下发
        back["uid"] = new_uid;
    } else {
        back["data"] = "failed";
    }

    //回执交给调用方(session)通过它自己的发送队列发出,原因同 login()
    frame = back.dump() + '\n';
    return ok;
}

//保存一条聊天记录
bool saveChatHistory(
    astra_sql::MySQLpp &mysqlAPI,
    const std::string &sender,
    const std::string &text,
    const std::string &sendTime
) {
    //发送方没给或者格式不对时以服务端本地时间为准
    const std::string stamp = (sendTime.size() == 19) ? sendTime : nowLocalTimestamp();

    const astra_sql::item chatData{
        {"sender", sender},
        {"text", text},
        {"sendTime", stamp}
    };
    const astra_sql::mysqlItemType chatType{
        astra_sql::mysqlDataType::String,
        astra_sql::mysqlDataType::String,
        astra_sql::mysqlDataType::DataTime
    };

    const auto result = mysqlAPI.addItem(kChatTableName, chatData, chatType);
    return result == astra_sql::SQLppError::success;
}

//取聊天记录表里最近的 limit 条(升序,最后一条就是最新的一条)
nlohmann::json fetchLatestChatRows(astra_sql::MySQLpp &mysqlAPI, const int limit) {
    auto rows = mysqlAPI.searchLatestRows(
        kChatTableName,
        {"sender", "text", "sendTime"},
        "sendTime",
        limit);
    if (!rows.is_object()) {
        return nlohmann::json::object();
    }
    return rows;
}

//创建聊天记录表
void createChatHistoryTable(astra_sql::MySQLpp &mysqlAPI) {
    if (!mysqlAPI.mysqlTableExists(kChatTableName)) {
        const auto createRule = std::vector<astra_sql::createTableRule>{
            {"sender", "varchar(50)", "not null"},
            {"text", "TEXT", "not null"},
            {"sendTime", "datetime", "not null"}
        };

        const auto result = mysqlAPI.mysqlCreateTable(kChatTableName, createRule, nullptr, nullptr);
        if (result != astra_sql::SQLppError::success) {
            std::cerr << "create chat table failed: " << kChatTableName << std::endl;
        }
    }
}
