#pragma once
#include <SQLite++/SQLitepp.h>
#include <string>



class my_SQLite{
    protected:


    astra_sql::SQLitepp *db=nullptr;
    std::string chat;

    std::string kTable;
    std::string CTable;

    public:

    my_SQLite();
    ~my_SQLite();
    /*
     *寻找对应用户的 数据表 CTable接受user值 不存在建立对应用户名的数据表
     *user 要建立的用户名对应的消息表
     *
     *
     */
    void my_init(const std::string &user);

    /*
   * 插入数据 用户表
   *user 用户名
   *pass 密码
   */
    void my_Insert(std::string user, std::string pass);
    /*
    *更改 对应用户密码
    *user 用户
    *pass 密码
    */
    void my_Update(std::string user, std::string pass);
    /*
    * 查询用户表对应用户信息 账户密码 返回json
    *user 用户
    */
    nlohmann::json my_Search(std::string user) ;

    /*
    *将当前用户表所有用户和密码 返回为json
    *
    */
    nlohmann::json my_SearchAll();
    /*
     *删除用户表中指定用户
    *user 用户名
     */
    void my_Delete(const std::string &user);


    /*
    *插入数据 消息表 需要用户 发送人 文本
    *user 拥有消息的用户
    *spend 发送消息的用户
    *text 消息
    */
    void my_chatInsert(std::string user,std::string spend,std::string text);
    /*
     *需先my_init 选中指定的用户表
       *返回 指定 用户 的聊天记录
       *user 用户名


       */
    nlohmann::json my_chatSearch(const std::string &user) ;
    /*
     *需先my_init 选中指定的用户表
        *删除消息表中指定id的消息
        *id
        *
        */
    void my_chatdelete(const std::string &id);

};
