#pragma once
#include <SQLite++/SQLitepp.h>   // 提供 nlohmann::json
#include <string>

void testFunc1(const std::string &user, const std::string &pass);
void testFunc2(const std::string &user, const std::string &pass);
nlohmann::json testFunc3(const std::string &user);