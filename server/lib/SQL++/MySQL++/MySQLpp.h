#pragma once
#include <memory>
#include <string>
#include <vector>

#include <mysql_driver.h>
#include <cppconn/connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

#include "../include/SQL.h"
#include "../include/json.hpp"

namespace astra_sql {
    class MySQLpp {
    private:
        // MySQL连接
        std::unique_ptr<sql::Connection> conn;
        // MySQL驱动
        sql::mysql::MySQL_Driver *driver;
        // MySQL执行接口
        std::unique_ptr<sql::PreparedStatement> stmt;
        std::unique_ptr<sql::Statement> statement;
        // 储存MySQL命令
        std::string cmd;

    public:
        /**
         * @brief mysqlpp类构造函数
         * @param host MySQL服务器地址
         * @param port MySQL端口
         * @param UserName MySQL用户名
         * @param password MySQL密码
         */

        MySQLpp(const std::string &host, unsigned int port, const std::string &UserName, const std::string &password);

        // 析构函数
        ~MySQLpp() = default;

        /**
         * @brief 切换操作的数据库
         * @param SchemaName 切换到的数据库名
         */

        SQLppError switchDatabase(const std::string &SchemaName);

        /**
         * @brief 创建数据库
         * @param SchemaName 要创建的数据库名称
         */
        SQLppError createDatabase(const std::string &SchemaName);

        /**
         * @brief 删库
         * @param SchemaName 要删除的数据库名称
         * @warning 跑路啦兄弟，跑路啦！！
         */
        SQLppError delDatabase(const std::string &SchemaName);

        /**
         * @brief 为表结构增加项目
         * @param tableName 表名
         * @param data 增加内容
         * @param types 增加内容的数据类型
         */
        SQLppError addItem(const std::string &tableName, const item &data, const mysqlItemType &types);

        /**
         * @brief 为表结构删除项目
         * @param tableName 表名
         * @param rule 删除约束
         */
        SQLppError delItem(const std::string &tableName, const itemRule &rule);

        /**
         * @brief 为表结构更新项目
         * @param tableName 表名
         * @param data 更新内容
         * @param rule 更新内容的数据类型
         */
        SQLppError updateItem(const std::string &tableName, const item &data, const mysqlItemType &types,
                              const itemRule &rule);

        /**
         * @brief   查找表结构中的内容
         * @param tableName 表名
         * @param data 需查找的表头
         * @param rule 查找约束
         * @return json格式的查找结果
         */
        nlohmann::json searchItem(const std::string &tableName, const std::vector<std::string> &data,
                                  const itemRule &rule);
    };
}
