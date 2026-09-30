#pragma once
#include <SQLite++/SQLitepp.h>
#include <string>

class my_SQLite {
protected:
    astra_sql::SQLitepp *db = nullptr;
    long long uid = 0;
    //用户名只写进本地的 userData 表,不参与文件名
    std::string userName;

    //用户数据表名
    std::string userDataTable = "userData";
    //聊天数据表名
    std::string ChatDataTable = "chatData";

public:
    //构造函数: uid 来自服务端登录/注册响应
    my_SQLite(long long uid, const std::string &userName, const std::string &password);

    //析构函数
    ~my_SQLite();

    //向SQLite插入聊天数据
    //sendTime: 由调用方给(收到别人消息时用服务端回传的那个值,双方存的才是同一个时间戳);
    //          为空时退回本机当前时间(本地时区)
    void chatDataInsert(const std::string &sender, const std::string &text, const std::string &sendTime) const;

    //读取本用户的全部聊天记录
    //返回 {"sender":[...],"text":[...],"sendTime":[...]},各列等长、按行号对齐(见 SQLitepp::sqlitSearchItem)
    nlohmann::json chatDataSearch() const;

    //本地"最新的那一条"(按 sendTime 取最大,和服务端比对时的口径一致)
    //一条都没有时返回空对象
    nlohmann::json lastMessage();

    //本地是否已经有这一条(登录补拉时用来去重)
    bool hasMessage(const std::string &sender, const std::string &text, const std::string &sendTime) const;

    //本地库对应的 uid(库文件名就是 <uid>.db)
    [[nodiscard]] long long getUid() const {
        return uid;
    }
};
