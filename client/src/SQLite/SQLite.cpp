#include "SQLite.h"
#include "../../lib/SQL++/SQLite++/SQLitepp.h"
#include <SQLite++/SQLitepp.h>
//初始化数据库？ 反正是建立了一个类
//存储
using  namespace  astra_sql;
using  namespace  std;


namespace {
    //两个全局变量
    //"user.db" 是相对路径，它落在进程的当前工作目录（CWD) 也就是终端 在哪里启动 就在哪里创建user.db这个 需要注意
    constexpr auto kDbFile = "user.db";
    constexpr auto kTable  = "user";

    // sql类 的全局变量 也就是会返回关于用户表的地址
     SQLitepp &db() {
        static SQLitepp inst(kDbFile, false);//库
        static bool once = [] {
            const vector<createTableRule> cols{
                    {"uid",      "INTEGER", "PRIMARY KEY AUTOINCREMENT"},
                    {"username", "TEXT",    "NOT NULL UNIQUE"},
                    {"pass",     "TEXT",    "NOT NULL"},
                };
            inst.sqliteCreateTable(kTable, cols, nullptr, nullptr);
            return true;
        }();// 那个lambda函数
        (void)once;//原地调用once
        return inst;//库
    }
}

// void testFunc() {
//     astra_sql::SQLitepp test("test.db", false);//sql 类
//     const std::vector<astra_sql::createTableRule> user{
//             {"uid",      "INTEGER", "PRIMARY KEY AUTOINCREMENT"},
//             {"username", "TEXT",    "NOT NULL UNIQUE"},
//             {"pass",     "TEXT",    "NOT NULL"},
//         };
//     db=test;
//     test.sqliteCreateTable("user", user, nullptr, nullptr);
//     //重复的表名？
// }
void testFunc1(string user,string pass) {
    db().sqliteInsertItem(
        kTable
        ,                                                 //表名
        {{"username", user}, {"pass", pass}}
        ,                        // item：顺序即绑定顺序
        {sqliteDataType::Text,                            // type：长度必须相同
         sqliteDataType::Text});

}
void testFunc2(const string &user, const string &pass) {
    db().sqliteUpdateItem(
        kTable,                 //表名
        {{"pass", pass}},                        // 要改的字段
        {{"username", "=", user, "AND"}});       // where 条件
}
nlohmann::json testFunc3(const string &user) {
    return db().sqlitSearchItem(
        kTable,      //表名
        {"uid", "username", "pass"},             // 要查的内容，不能为空
        {{"username", "=", user, "AND"}});       // 空则查全部
}