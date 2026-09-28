#pragma once
#include <SQLite++/SQLitepp.h>
#include <string>

void my_Insert(std::string user, std::string pass);
void my_Update(std::string user, std::string pass);
nlohmann::json my_Search(std::string user) ;

nlohmann::json my_SearchAll();
//删除某个本机账号
void my_Delete(const std::string &user);
//聊天记录消息的插入
void my_chatInsert(std::string user,std::string spend,std::string text);
//返回用户拥有的聊天记录
nlohmann::json my_chatSearch(const std::string &user) ;

void my_chatdelete(const std::string &id);