#include "MySql.h"
#include <chrono>
#include <iostream>
#include <memory>
#include <vector>
#include <def.h>

namespace {
    //全服只有这一张聊天记录表: 所有用户的消息都写在这里
    //(不再"一个用户一张表",登录时客户端拿本地最后一条和这张表的最后一条比对)
    constexpr const char *kChatTableName = "chat_messages";
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
    //searchItem 把每一列都按字符串数组返回,uid 在里面也是字符串
    const auto uid_list = json.is_object()
                              ? json.value("uid", nlohmann::json::array())
                              : nlohmann::json::array();
    const bool ok = json.is_object()
                    && !json.value("userName", nlohmann::json::array()).empty()
                    && !uid_list.empty();

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

//保存一条聊天记录(全服同一张表)
bool saveChatHistory(
    astra_sql::MySQLpp &mysqlAPI,
    const std::string &sender,
    const std::string &text,
    const std::string &sendTime
) {
    //时间戳以发送方给的为准(两端存同一个值,登录时才能比对"最后一条"),
    //发送方没给或者格式不对时用服务端本地时间兜底
    const std::string stamp = (sendTime.size() == 19) ? sendTime : nowLocalTimestamp();

    const astra_sql::item chatData{
        {"sender", sender},
        {"text", text},
        {"sendTime", stamp}
    };
    const astra_sql::mysqlItemType chatType{
        astra_sql::mysqlDataType::String,
        astra_sql::mysqlDataType::String,
        //DataTime对应setDateTime,接受"YYYY-MM-DD HH:MM:SS"格式的字符串
        astra_sql::mysqlDataType::DataTime
    };

    const auto result = mysqlAPI.addItem(kChatTableName, chatData, chatType);
    return result == astra_sql::SQLppError::success;
}

//取聊天记录表里最近的 limit 条(升序,最后一条就是最新的一条)
nlohmann::json fetchLatestChatRows(astra_sql::MySQLpp &mysqlAPI, const int limit) {
    const auto rows = mysqlAPI.searchLatestRows(
        kChatTableName,
        {"sender", "text", "sendTime"},
        "sendTime",
        limit);
    if (!rows.is_object()) {
        return nlohmann::json::object();
    }
    return rows;
}

//创建聊天记录表(全服就这一张)
void createChatHistoryTable(astra_sql::MySQLpp &mysqlAPI) {
    auto createRule = std::vector<astra_sql::createTableRule>{
        {"sender", "varchar(50)", "not null"},
        {"text", "TEXT", "not null"},
        {"sendTime", "datetime", "not null"}
    };

    //mysqlCreateTable内部就是"create table if not exists",重复调用不报错,所以不用先查存在性
    const auto result = mysqlAPI.mysqlCreateTable(kChatTableName, createRule, nullptr, nullptr);
    if (result != astra_sql::SQLppError::success) {
        std::cerr << "create chat table failed: " << kChatTableName << std::endl;
    }
}
