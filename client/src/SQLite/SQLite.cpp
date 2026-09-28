#include "SQLite.h"
#include <SQLite++/SQLitepp.h>
#include <chrono>
#include <ctime>
#include <cstdio>
#include <string>
#include <cstdint>
using namespace astra_sql;
using  std::vector;
using  std::string;



//创建数据库 数据表
namespace {
    //两个 函数局部静态变量 就是这个文件的全局变量
    //"user.db" 是相对路径，它落在进程的当前工作目录（CWD) 也就是终端 在哪里启动 就在哪里创建user.db这个 需要注意
    constexpr auto kDbFile = "user.db";
    constexpr auto kTable = "user";

    constexpr auto CTable = "chat";

    // 返回sql类 的函数地址 也就是会返回关于用户表的地址
    SQLitepp &db() {
        static SQLitepp inst(kDbFile, false); //库  静态变量只会运行一次
        static bool once = [] {
            const vector<createTableRule> cols{
                //用户表结构
                {"uid", "INTEGER", "PRIMARY KEY AUTOINCREMENT"},
                {"username", "TEXT", "NOT NULL UNIQUE"},
                {"pass", "TEXT", "NOT NULL"},
            };


        // 那个隐形lambda函数
        const vector<createTableRule> chat{
                    {"cid",    "INTEGER", "PRIMARY KEY AUTOINCREMENT"},
                    {"owner",  "TEXT",    "NOT NULL"},
                    {"sender", "TEXT",    "NOT NULL"},
                    {"text",   "TEXT",    "NOT NULL"},
                    {"ts",     "INTEGER", "NOT NULL"},
                };
            inst.sqliteCreateTable(kTable, cols, nullptr, nullptr);
            inst.sqliteCreateTable(CTable, chat, nullptr, nullptr);
            return true;
        }();
        (void) once; // 类型强转 仅仅防止变量未使用的报错
        return inst; //库
    }

}

void my_Insert(string user, string pass) {
    db().sqliteInsertItem(
        kTable, //表名
        {{"username", user}, {"pass", pass}}, // item：顺序即绑定顺序
        {
            sqliteDataType::Text, // type：长度必须相同
            sqliteDataType::Text
        });
}

void my_Update(string user, string pass) {
    db().sqliteUpdateItem(
        kTable, //表名
        {{"pass", pass}}, // 要改的字段
        {{"username", "=", user, "AND"}}); // where 条件
}

nlohmann::json my_Search(string user) {
    return db().sqlitSearchItem(
        kTable, //表名
        {"uid", "username", "pass"}, // 要查的内容，不能为空
        {{"username", "=", user, "AND"}}); // 空则查全部
}


nlohmann::json my_SearchAll() {
    // 第三个参数传空 = 不带 where；注意第二个参数（列）不能为空，否则 SQL 语法错
    return db().sqlitSearchItem(kTable, {"uid", "username", "pass"}, {});
}

void my_Delete(const string &user) {
    db().sqliteDelItem(kTable, {{"username", "=", user, "AND"}});
}

void my_chatInsert(string user, string spend,string text) {
    time_t t = time(nullptr);
     // 时间戳
    db().sqliteInsertItem(
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
void my_chatdelete(const string &id) {
    db().sqliteDelItem(CTable, {{"cid", "=", id, "AND"}});
}

nlohmann::json my_chatSearch(const string &user) {
    return db().sqlitSearchItem(kTable, {"cid", "owner", "sender","text","ts"}, {{"owner", "=", user, "AND"}});
};