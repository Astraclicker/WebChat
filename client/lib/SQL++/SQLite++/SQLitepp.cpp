
#include "SQLitepp.h"
#include <iostream>
#include <stdexcept>

#include "SQLiteCpp/ExecuteMany.h"

namespace astra_sql {
    //构造函数
    SQLitepp::SQLitepp(const std::string &dbName, const bool foreign_key) {
        if (sqlite3_open(dbName.c_str(), &db) != SQLITE_OK) {
            std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
            if (foreign_key) {
                sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
            }
            return;
        }
        std::clog << "SQLite connect successfully" << std::endl;
    }

    //创建表
    SQLppError SQLitepp::sqliteCreateTable
    (
        const std::string &tableName,
        const std::vector<createTableRule> &createRule,
        const primaryKeyRule *primaryKey,
        const uniqueKeyRule *uniqueKey
    ) {
        cmd = "create table if not exists " + tableName + " ( ";
        for (const auto &i: createRule) {
            cmd += i.field + " " + i.type + " " + i.restriction + ",";
        }
        if (primaryKey != nullptr) {
            cmd += "primary key (";
            for (auto i = primaryKey->begin(); i != primaryKey->end(); ++i) {
                cmd += i == primaryKey->begin() ? *i : "," + *i;
            }
            cmd += "),";
        }
        if (uniqueKey != nullptr) {
            cmd += "unique (";
            for (auto i = uniqueKey->begin(); i != uniqueKey->end(); ++i) {
                cmd += i == uniqueKey->begin() ? *i : "," + *i;
            }
            cmd += "),";
        }
        cmd.pop_back();
        cmd += " );";
        try {
            checkError = sqlite3_exec(db, cmd.c_str(), nullptr, nullptr, &errMsg);
            if (checkError != SQLITE_OK) {
                std::cerr << sqlite3_errmsg(db) << std::endl;
            }
        } catch (std::exception &error) {
            std::cerr << error.what() << std::endl << errMsg << std::endl;
            cmd.clear();
            return SQLppError::error_create;
        }

        cmd.clear();
        return SQLppError::success;
    }

    //删表
    SQLppError SQLitepp::sqliteDelTable(const std::string &tableName) {
        cmd = "drop table if exists " + tableName;
        try {
            checkError = sqlite3_exec(db, cmd.c_str(), nullptr, nullptr, &errMsg);
            if (checkError != SQLITE_OK) {
                std::cerr << sqlite3_errmsg(db) << std::endl;
            }
        } catch (std::exception &error) {
            std::cerr << error.what() << std::endl << errMsg << std::endl;
            cmd.clear();
            return SQLppError::error_create;
        }
        cmd.clear();
        return SQLppError::error_del;
    }

    //向表内插入数据
    SQLppError SQLitepp::sqliteInsertItem(const std::string &tableName, const item &data, const sqliteItemType &type) {
        if (data.size() != type.size()) {
            return SQLppError::error_create;
        }
        const auto cnt = data.size();
        cmd = "insert into " + tableName + "(";
        for (auto i = data.begin(); i != data.end(); ++i) {
            cmd += i == data.begin() ? i->first : "," + i->first;
        }
        cmd += ')';
        cmd += "values(";
        for (auto i = 0; i < cnt; i++) {
            cmd += i == 0 ? "?" : ",?";
        }
        cmd += ");";
        sqlite3_prepare_v2(db, cmd.c_str(), -1, &stmt, nullptr);

        for (int i = 0; i < cnt; i++) {
            switch (type[i]) {
                case sqliteDataType::Blob:
                    sqlite3_bind_blob(stmt, i + 1, data[i].second.c_str(), -1,SQLITE_STATIC);
                    break;
                case sqliteDataType::Double:
                    sqlite3_bind_double(stmt, i + 1, std::stod(data[i].second));
                    break;
                case sqliteDataType::Int:
                    sqlite3_bind_int(stmt, i + 1, std::stoi(data[i].second));
                    break;
                case sqliteDataType::Int64:
                    sqlite3_bind_int64(stmt, i + 1, std::stoll(data[i].second));
                    break;
                case sqliteDataType::Null:
                    sqlite3_bind_null(stmt, i + 1);
                    break;
                case sqliteDataType::Text:
                    sqlite3_bind_text(stmt, i + 1, data[i].second.c_str(), -1,SQLITE_STATIC);
                    break;
            }
        }
        try {
            checkError = sqlite3_step(stmt);
            if (checkError != SQLITE_DONE) {
                throw std::runtime_error("insert failed");
            }
        } catch (std::exception &error) {
            std::cerr << error.what() << std::endl << sqlite3_errmsg(db) << std::endl;
            cmd.clear();
            return SQLppError::error_create;
        }
        cmd.clear();
        return SQLppError::success;
    }

    //删除表中数据
    SQLppError SQLitepp::sqliteDelItem(const std::string &tableName, const itemRule &rule) {
        if (rule.empty()) {
            return SQLppError::error_del;
        }
        //准备语句
        cmd = "delete from " + tableName;
        for (auto i = rule.begin(); i != rule.end(); ++i) {
            cmd += i == rule.begin() ? " where" : " " + i->link;
            cmd += " " + i->field + " " + i->op + " " + "?";
        }
        cmd += ';';
        const auto cnt = rule.size();
        sqlite3_prepare_v2(db, cmd.c_str(), -1, &stmt, nullptr);
        //绑定参数
        for (int i = 0; i < cnt; i++) {
            sqlite3_bind_text(stmt, i + 1, rule[i].value.c_str(), -1,SQLITE_STATIC);
        }
        //执行语句
        try {
            checkError = sqlite3_step(stmt);
            if (checkError != SQLITE_DONE) {
                throw std::runtime_error("delete error");
            }
        } catch (std::exception &error) {
            std::cerr << error.what() << std::endl << sqlite3_errmsg(db) << std::endl;
            cmd.clear();
            return SQLppError::error_del;
        }
        cmd.clear();
        return SQLppError::success;
    }

    //更改表中数据
    SQLppError SQLitepp::sqliteUpdateItem(const std::string &tableName, const item &data, const itemRule &rule) {
        //准备语句
        cmd = "update " + tableName + " set ";
        for (auto i = data.begin(); i != data.end(); ++i) {
            cmd += i == (data.end() - 1) ? i->first + " =?" : i->first + " =?,";
        }
        for (auto i = rule.begin(); i != rule.end(); ++i) {
            cmd += i == rule.begin() ? " where" : " " + i->link;
            cmd += " " + i->field + " " + i->op + " " + "?";
        }
        sqlite3_prepare_v2(db, cmd.c_str(), -1, &stmt, nullptr);
        const auto cnt_data = data.size();
        const auto cnt_rule = rule.size();
        //绑定参数
        for (auto i = 0; i < cnt_data; i++) {
            sqlite3_bind_text(stmt, i + 1, data[i].second.c_str(), -1,SQLITE_STATIC);
        }
        for (auto i = 0; i < cnt_rule; i++) {
            sqlite3_bind_text(stmt, i + cnt_data + 1, rule[i].value.c_str(), -1,SQLITE_STATIC);
        }
        //执行语句
        try {
            checkError = sqlite3_step(stmt);
            if (checkError != SQLITE_DONE) {
                throw std::runtime_error("update error");
            }
        } catch (std::exception &error) {
            std::cerr << error.what() << std::endl << sqlite3_errmsg(db) << std::endl;
            cmd.clear();
            return SQLppError::error_change;
        }
        cmd.clear();
        return SQLppError::success;
    }

    //查找表中数据
    nlohmann::json SQLitepp::sqlitSearchItem(
        const std::string &tableName,
        const std::vector<std::string> &data,
        const itemRule &rule
    ) {
        //准备语句
        cmd = "select ";
        for (auto i = data.begin(); i != data.end(); ++i) {
            cmd += i == data.begin() ? *i : "," + *i;
        }
        cmd += " from " + tableName;
        for (auto i = rule.begin(); i != rule.end(); ++i) {
            cmd += i == rule.begin() ? " where" : " " + i->link;
            cmd += " " + i->field + " " + i->op + " " + "?";
        }
        //绑定参数
        sqlite3_prepare_v2(db, cmd.c_str(), -1, &stmt, nullptr);
        const auto cnt_rule = rule.size();
        for (auto i = 0; i < cnt_rule; i++) {
            sqlite3_bind_text(stmt, i + 1, rule[i].value.c_str(), -1,SQLITE_STATIC);
        }
        //执行语句,写入结果
        const auto cnt_data = data.size();
        nlohmann::json result = nlohmann::json::object();
        for (auto i = 0; i < cnt_data; i++) {
            result[data[i]] = nlohmann::json::array();
        }

        try {
            while ((checkError = sqlite3_step(stmt)) == SQLITE_ROW) {
                for (auto i = 0; i < cnt_data; i++) {
                    result[data[i]].push_back(reinterpret_cast<const char *>(sqlite3_column_text(stmt, i)));
                }
            }

            if (checkError != SQLITE_DONE) {
                throw std::runtime_error("select error");
            }
        } catch (std::exception &error) {
            std::cerr << error.what() << std::endl << sqlite3_errmsg(db) << std::endl;
            cmd.clear();
            return nlohmann::json{};
        }
        cmd.clear();;
        return result;
    }

    //析构
    SQLitepp::~SQLitepp() {
        sqlite3_close(db);
        sqlite3_free(errMsg);
        sqlite3_finalize(stmt);
    }
}