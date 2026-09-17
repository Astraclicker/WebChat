#include <sstream>
#include <iostream>

#include "MySQLpp.h"

namespace astra_sql {
    // 构造函数
    MySQLpp::MySQLpp(const std::string &host, unsigned int port, const std::string &UserName,
                     const std::string &password) {
        try {
            driver = sql::mysql::get_driver_instance();
            conn.reset(driver->connect("tcp://" + host + ":" + std::to_string(port), UserName, password));
            statement.reset(conn->createStatement());
        } catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
            return;
        }
        std::clog << "MySQL connect successfully" << std::endl;
    }

    // 切换数据库
    SQLppError MySQLpp::switchDatabase(const std::string &SchemaName) {
        try {
            conn->setSchema(SchemaName);
        } catch (const std::exception &e) {
            std::cerr << e.what() << '\n';
            return SQLppError::error_database;
        }
        return SQLppError::success;
    }

    // 新建数据库
    SQLppError MySQLpp::createDatabase(const std::string &SchemaName) {
        std::string sql = "CREATE DATABASE IF NOT EXISTS " + SchemaName;
        try {
            statement->execute(sql);
        } catch (const std::exception &e) {
            std::cerr << e.what() << '\n';
            return SQLppError::error_database;
        }
        return SQLppError::success;
    }

    // 删除数据库
    SQLppError MySQLpp::delDatabase(const std::string &SchemaName) {
        std::string sql = "DROP DATABASE IF EXISTS " + SchemaName;
        try {
            statement->execute(sql);
        } catch (const std::exception &e) {
            std::cerr << e.what() << '\n';
            return SQLppError::error_database;
        }
        return SQLppError::success;
    }

    // 增
    SQLppError MySQLpp::addItem(const std::string &tableName, const item &data, const mysqlItemType &types) {
        this->cmd = "insert into " + tableName + "(";
        auto cnt = data.size();
        for (int i = 0; i < cnt; i++) {
            if (i == cnt - 1) {
                this->cmd += data.at(i).first;
            } else {
                this->cmd += (data.at(i).first + ",");
            }
        }
        this->cmd += ")VALUE(";
        for (int i = 0; i < cnt; i++) {
            if (i == cnt - 1) {
                this->cmd += '?';
            } else {
                this->cmd += "?,";
            }
        }
        this->cmd += ");";

        try {
            stmt.reset(conn->prepareStatement(this->cmd));

            // 存放 Blob 数据流，需存活到 executeUpdate() 之后
            std::vector<std::unique_ptr<std::istringstream> > blobStreams;

            for (int i = 1; i <= cnt; i++) {
                const std::string &value = data.at(i - 1).second;
                switch (types.at(i - 1)) {
                    case mysqlDataType::BigInt:
                        stmt->setBigInt(i, value);
                        break;
                    case mysqlDataType::Blob: {
                        // setBlob 惰性读取流，流必须存活到 executeUpdate() 之后
                        blobStreams.emplace_back(std::make_unique<std::istringstream>(value));
                        stmt->setBlob(i, blobStreams.back().get());
                        break;
                    }
                    case mysqlDataType::Bool:
                        stmt->setBoolean(i, value == "true" || value == "1");
                        break;
                    case mysqlDataType::DataTime:
                        stmt->setDateTime(i, value);
                        break;
                    case mysqlDataType::Double:
                        stmt->setDouble(i, std::stod(value));
                        break;
                    case mysqlDataType::Int32:
                        stmt->setInt(i, std::stoi(value));
                        break;
                    case mysqlDataType::Int64:
                        stmt->setInt64(i, std::stoll(value));
                        break;
                    case mysqlDataType::Null:
                        stmt->setNull(i, sql::DataType::SQLNULL);
                        break;
                    case mysqlDataType::String:
                        stmt->setString(i, value);
                        break;
                    case mysqlDataType::Uint32:
                        stmt->setUInt(i, static_cast<uint32_t>(std::stoul(value)));
                        break;
                    case mysqlDataType::Uint64:
                        stmt->setUInt64(i, std::stoull(value));
                        break;
                }
            }

            stmt->executeUpdate();
        } catch (const std::exception &e) {
            this->cmd.clear();
            std::cerr << e.what() << '\n';
            return SQLppError::error_create;
        }
        this->cmd.clear();
        return SQLppError::success;
    }

    // 删
    SQLppError MySQLpp::delItem(const std::string &tableName, const itemRule &rule) {
        this->cmd = "delete from " + tableName;
        for (auto i = rule.begin(); i != rule.end(); i++) {
            cmd += ' ';
            cmd += (i == rule.begin()) ? "where" : (*i).link;
            cmd += ' ';
            cmd += (i->field + " " + i->op + "?");
        }

        try {
            auto cnt = rule.size();
            stmt.reset(conn->prepareStatement(this->cmd));
            for (int i = 0; i < cnt; i++) {
                stmt->setString(i + 1, rule[i].value);
            }
            stmt->execute();
        } catch (const std::exception &e) {
            this->cmd.clear();
            std::cerr << e.what() << '\n';
            return SQLppError::error_del;
        }
        this->cmd.clear();
        return SQLppError::success;
    }

    // 改
    SQLppError MySQLpp::updateItem(const std::string &tableName, const item &data, const mysqlItemType &types,
                                   const itemRule &rule) {
        auto cnt = data.size();
        if (cnt == 0) {
            std::cerr << "updateItem: no field specified to modify\n";
            return SQLppError::error_change;
        }

        this->cmd = "update " + tableName + " set ";
        for (int i = 0; i < cnt; i++) {
            this->cmd += data.at(i).first;
            this->cmd += (i == cnt - 1) ? " = ?" : " = ?,";
        }

        for (auto i = rule.begin(); i != rule.end(); ++i) {
            cmd += ' ';
            cmd += (i == rule.begin()) ? "where" : (*i).link;
            cmd += ' ';
            cmd += (i->field + " " + i->op + "?");
        }

        try {
            stmt.reset(conn->prepareStatement(this->cmd));
            // 存放 Blob 数据流，需存活到 executeUpdate() 之后
            std::vector<std::unique_ptr<std::istringstream> > blobStreams;
            for (int i = 1; i <= cnt; i++) {
                const std::string &value = data.at(i - 1).second;
                switch (types.at(i - 1)) {
                    case mysqlDataType::BigInt:
                        stmt->setBigInt(i, value);
                        break;
                    case mysqlDataType::Blob: {
                        // setBlob 惰性读取流，流必须存活到 executeUpdate() 之后
                        blobStreams.emplace_back(std::make_unique<std::istringstream>(value));
                        stmt->setBlob(i, blobStreams.back().get());
                        break;
                    }
                    case mysqlDataType::Bool:
                        stmt->setBoolean(i, value == "true" || value == "1");
                        break;
                    case mysqlDataType::DataTime:
                        stmt->setDateTime(i, value);
                        break;
                    case mysqlDataType::Double:
                        stmt->setDouble(i, std::stod(value));
                        break;
                    case mysqlDataType::Int32:
                        stmt->setInt(i, std::stoi(value));
                        break;
                    case mysqlDataType::Int64:
                        stmt->setInt64(i, std::stoll(value));
                        break;
                    case mysqlDataType::Null:
                        stmt->setNull(i, sql::DataType::SQLNULL);
                        break;
                    case mysqlDataType::String:
                        stmt->setString(i, value);
                        break;
                    case mysqlDataType::Uint32:
                        stmt->setUInt(i, static_cast<uint32_t>(std::stoul(value)));
                        break;
                    case mysqlDataType::Uint64:
                        stmt->setUInt64(i, std::stoull(value));
                        break;
                }
            }

            for (int j = 0; j < (int) rule.size(); j++) {
                stmt->setString(cnt + 1 + j, rule[j].value);
            }

            stmt->executeUpdate();
        } catch (const std::exception &e) {
            this->cmd.clear();
            std::cerr << e.what() << '\n';
            return SQLppError::error_change;
        }

        this->cmd.clear();
        return SQLppError::success;
    }

    // 查
    nlohmann::json MySQLpp::searchItem(const std::string &tableName, const std::vector<std::string> &data,
                                       const itemRule &rule) {
        this->cmd = "select ";
        if (data.empty()) {
            this->cmd += '*';
        } else {
            for (size_t i = 0; i < data.size(); i++) {
                if (i > 0) {
                    this->cmd += ',';
                }
                this->cmd += data[i];
            }
        }
        this->cmd += " from " + tableName;

        for (auto i = rule.begin(); i != rule.end(); i++) {
            this->cmd += ' ';
            this->cmd += (i == rule.begin()) ? "where" : i->link;
            this->cmd += ' ';
            this->cmd += i->field + " " + i->op + "?";
        }

        try {
            stmt.reset(conn->prepareStatement(this->cmd));
            for (int j = 0; j < static_cast<int>(rule.size()); j++) {
                stmt->setString(j + 1, rule[j].value);
            }
            // MySQL获取资源
            const std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
            sql::ResultSetMetaData *meta = res->getMetaData();
            const auto cols = meta->getColumnCount();

            nlohmann::json result = nlohmann::json::object();

            // 初始化每一列为空数组
            for (int c = 1; c <= cols; c++) {
                result[meta->getColumnLabel(c)] = nlohmann::json::array();
            }

            // 遍历结果集，向各列的数组中添加数据
            while (res->next()) {
                for (int c = 1; c <= cols; c++) {
                    result[meta->getColumnLabel(c)].push_back(res->getString(c));
                }
            }

            return result;
        } catch (const std::exception &e) {
            this->cmd.clear();
            std::cerr << e.what() << '\n';
            return nlohmann::json{};
        }

        this->cmd.clear();
    }
}
