#include "SQLite.h"
#include <algorithm>
#include <def.h>
#include <iostream>
#include <log.h>
#include <string>
using namespace astra_sql;
using std::vector;
using std::string;

my_SQLite::my_SQLite(const long long uid, const std::string &userName, const std::string &password)
    : uid(uid), userName(userName) {
    //库名用 uid: 用户名可能是中文,直接当文件名会乱码
    db = new SQLitepp(std::to_string(uid) + ".db", false);
    if (!db->sqliteTableExists(userDataTable)) {
        //用户表结构
        const vector<createTableRule> cols{
            {"userName", "TEXT", "NOT NULL UNIQUE"},
            {"password", "TEXT", "NOT NULL"},
        };
        db->sqliteCreateTable(userDataTable, cols, nullptr, nullptr);
    }
    if (!db->sqliteTableExists(ChatDataTable)) {
        //聊天数据表结构
        const vector<createTableRule> chat{
            {"sender", "TEXT", "NOT NULL"},
            {"text", "TEXT", "NOT NULL"},
            {"sendTime", "TEXT", "NOT NULL"},
        };
        db->sqliteCreateTable(ChatDataTable, chat, nullptr, nullptr);
    }
    //创建表之后直接插入用户数据(存在的是用户名,不是 uid)
    //注意: userName 上有 UNIQUE 约束,同一用户第二次登录这条必然失败,只提示不影响使用
    const auto user_result = db->sqliteInsertItem(
        userDataTable,
        {{"userName", userName}, {"password", password}},
        {sqliteDataType::Text, sqliteDataType::Text}
    );
    if (user_result != SQLppError::success) {
        LOG(astra_log::Level::Error,
            "insert local user row failed (uid=", uid, ", user=", userName,
            ", maybe already exists), code=", static_cast<int>(user_result));
    }
}

my_SQLite::~my_SQLite() = default;

void my_SQLite::chatDataInsert(const std::string &sender, const std::string &text, const std::string &sendTime) const {
    const std::string stamp = (sendTime.size() == 19) ? sendTime : nowLocalTimestamp();

    const auto result = db->sqliteInsertItem(
        ChatDataTable, //表名
        {{"sender", sender}, {"text", text}, {"sendTime", stamp}},
        {
            sqliteDataType::Text,
            sqliteDataType::Text,
            sqliteDataType::Text,
        });

    //失败必须自己打日志: 这个函数不会抛异常,只靠返回值表示结果
    if (result != SQLppError::success) {
        LOG(astra_log::Level::Error,
            "insert chat data failed (uid=", uid, ", user=", userName,
            ", sender=", sender, "), code=", static_cast<int>(result));
    }
}

//读取本用户的全部聊天记录
nlohmann::json my_SQLite::chatDataSearch() const {
    return db->sqlitSearchItem(
        ChatDataTable, //表名
        {"sender", "text", "sendTime"}, //要查的列,不能为空
        {}); //查询条件: 空 = 全部
}

nlohmann::json my_SQLite::lastMessage() {
    const auto all = chatDataSearch();
    const auto senders = all.value("sender", nlohmann::json::array());
    const auto texts = all.value("text", nlohmann::json::array());
    const auto times = all.value("sendTime", nlohmann::json::array());

    const std::size_t count = std::min({senders.size(), texts.size(), times.size()});
    if (count == 0) {
        return nlohmann::json::object();
    }

    //"YYYY-MM-DD HH:MM:SS" 直接按字符串比大小就是按时间比大小
    std::size_t newest = 0;
    for (std::size_t i = 1; i < count; ++i) {
        if (times[i].get<std::string>() >= times[newest].get<std::string>()) {
            newest = i;
        }
    }

    nlohmann::json row;
    row["sender"] = senders[newest];
    row["text"] = texts[newest];
    row["sendTime"] = times[newest];
    return row;
}

bool my_SQLite::hasMessage(const std::string &sender, const std::string &text, const std::string &sendTime) const {
    const auto found = db->sqlitSearchItem(
        ChatDataTable,
        {"sender"}, //只要有一列返回就说明存在
        {
            {"sender", "=", sender, "AND"},
            {"text", "=", text, "AND"},
            {"sendTime", "=", sendTime, "AND"}
        });
    return !found.value("sender", nlohmann::json::array()).empty();
}
