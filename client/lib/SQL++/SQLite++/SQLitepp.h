#pragma once

#include <sqlite3.h>

#include <vector>
#include <string>

#include "../include/SQL.h"
#include "../include/json.hpp"

namespace astra_sql {
    class SQLitepp {
    protected:
        //sqlite执行命令
        std::string cmd;
        //sqlite错误处理
        int checkError{};
        char *errMsg = nullptr;
        //sqlite执行接口
        sqlite3_stmt *stmt = nullptr;
        //sqlite数据库接口
        sqlite3 *db{};

    public:
        /**
         * @brief 构造函数
         * @param dbName 数据库名称
         * @param foreign_key 是否启用外键
         */
        SQLitepp(const std::string &dbName, bool foreign_key);

        /**
         * @brief sqlite创建表
         * @param tableName 要创建的表名
         * @param createRule 创建表的规则
         * @param primaryKey 主键规则
         * @param uniqueKey 为唯一键规则
         * @return 报错枚举
         */
        SQLppError sqliteCreateTable
        (
            const std::string &tableName,
            const std::vector<createTableRule> &createRule,
            const primaryKeyRule *primaryKey,
            const uniqueKeyRule *uniqueKey
        );

        /**
         * @brief 删除表
         * @param tableName 要删除的表名
         * @return 报错枚举
         * @warning 跑路啦兄弟，跑路啦
         */
        SQLppError sqliteDelTable(const std::string &tableName);

        /**
         * @brief 向表中插入数据
         * @param tableName 表名
         * @param data 插入的数据
         * @param type 插入的数据的类型
         * @return 报错枚举
         */
        SQLppError sqliteInsertItem(const std::string &tableName, const item &data, const sqliteItemType &type);

        /**
         * @brief 删除表中数据
         * @param tableName 表名
         * @param rule 删除规则
         * @return 报错枚举
         */
        SQLppError sqliteDelItem(const std::string &tableName, const itemRule &rule);

        /**
         * @brief 更改表中数据
         * @param tableName 要更改的表名
         * @param data 更改的数据
         * @param rule 更改规则
         * @return 报错枚举
         */
        SQLppError sqliteUpdateItem(const std::string &tableName, const item &data, const itemRule &rule);

        /**
         * @brief 查找表中数据
         * @param tableName 要查找的表名
         * @param data 要查找的表头
         * @param rule 查找限制
         * @return json格式的查找结果
         */
        nlohmann::json sqlitSearchItem(
            const std::string &tableName,
            const std::vector<std::string> &data,
            const itemRule &rule
        );

        //析构
        ~SQLitepp();
    };
}