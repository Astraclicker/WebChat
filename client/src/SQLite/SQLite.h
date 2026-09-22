#pragma once
#include <SQLite++/SQLitepp.h>   // 提供 nlohmann::json
#include <string>
//向user表 插入用户名 密码数据
void my_Insert( std::string user,  std::string pass);
//user表 修改 用户名 密码数据
void my_Update(std::string user, std::string pass);
//user表中 查询指定的用户信息 空查所有用户消息
nlohmann::json my_Search( std::string user);