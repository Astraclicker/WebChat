#pragma once
#include <SQLite++/SQLitepp.h>   // 提供 nlohmann::json
#include <string>

void testFunc1( std::string user,  std::string pass);
void testFunc2(std::string user, std::string pass);
nlohmann::json testFunc3( std::string user);