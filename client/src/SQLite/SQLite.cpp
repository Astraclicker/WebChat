
#include "SQLite.h"

#include <chrono>
#include <ctime>
#include <cstdio>
#include <string>
#include <cstdint>
using namespace astra_sql;
using  std::vector;
using  std::string;





my_SQLite::my_SQLite(){
    static SQLitepp inst("sqlite.db", false);
    kTable="user";
    static bool once = [] {
        if (inst.sqliteTableExists("user")) {
            return true;
        }
        const vector<createTableRule> cols{
            //用户表结构
            {"uid", "INTEGER", "PRIMARY KEY AUTOINCREMENT"},
            {"username", "TEXT", "NOT NULL UNIQUE"},
            {"pass", "TEXT", "NOT NULL"},
        };


        inst.sqliteCreateTable("user", cols, nullptr, nullptr);
        return true;

    } ();
    (void)once;
    db=&inst;
}
my_SQLite::~my_SQLite() = default;
void my_SQLite::my_init(const std::string &user) {
    CTable=user;

    if (db->sqliteTableExists(CTable)) {
        return;
    }
    const vector<createTableRule> chat{
                    {"cid",    "INTEGER", "PRIMARY KEY AUTOINCREMENT"},
                    {"owner",  "TEXT",    "NOT NULL"},
                    {"sender", "TEXT",    "NOT NULL"},
                    {"text",   "TEXT",    "NOT NULL"},
                    {"ts",     "INTEGER", "NOT NULL"},
                };

            db->sqliteCreateTable(CTable, chat, nullptr, nullptr);



}

void my_SQLite::my_Insert(string user, string pass) {
    db->sqliteInsertItem(
        kTable, //表名
        {{"username", user}, {"pass", pass}}, // item：顺序即绑定顺序
        {
            sqliteDataType::Text, // type：长度必须相同
            sqliteDataType::Text
        });
}

void my_SQLite::my_Update(string user, string pass){
    db->sqliteUpdateItem(
        kTable, //表名
        {{"pass", pass}}, // 要改的字段
        {{"username", "=", user, "AND"}}); // where 条件
}

nlohmann::json my_SQLite::my_Search(string user) {
    return db->sqlitSearchItem(
        kTable, //表名
        {"uid", "username", "pass"}, // 要查的内容，不能为空
        {{"username", "=", user, "AND"}}); // 空则查全部
}


nlohmann::json my_SQLite::my_SearchAll() {
    // 第三个参数传空 = 不带 where；注意第二个参数（列）不能为空，否则 SQL 语法错
    return db->sqlitSearchItem(kTable, {"uid", "username", "pass"}, {});
}

void my_SQLite::my_Delete(const string &user) {
    db->sqliteDelItem(kTable, {{"username", "=", user, "AND"}});
}

void my_SQLite::my_chatInsert(string user, string spend,string text) {
    time_t t = time(nullptr);
     // 时间戳
    db->sqliteInsertItem(
        CTable, //表名
        {{"owner", user}, {"sender", spend}, {"text", text},{"ts",std::to_string(t)}}, // item：顺序即绑定顺序
        {
            sqliteDataType::Text, // type：长度必须相同
            sqliteDataType::Text,
            sqliteDataType::Text,
            sqliteDataType::Int64
        });
}
//由于只接受string 自然传string
void my_SQLite::my_chatdelete(const string &id) {
    db->sqliteDelItem(CTable, {{"cid", "=", id, "AND"}});
}

nlohmann::json my_SQLite::my_chatSearch(const string &user) {
    return db->sqlitSearchItem(CTable, {"cid", "owner", "sender","text","ts"}, {{"owner", "=", user, "AND"}});
};