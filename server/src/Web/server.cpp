#include "web.h"
#include <iostream>
#include <stdexcept>
#include <vector>
//构造函数
server::server(boost::asio::io_context &io, nlohmann::json config)
    : redisAPI(config["Redis"]["address"],
        config["Redis"]["port"],
        config["Redis"]["userName"],
        config["Redis"]["password"],
        0
    ),
    serverAcceptor(io, tcp::endpoint(tcp::v4(), config["Web"]["port"])),
      configFile(config) {
        nlohmann::json mysql_config = config["MySQL"];
        //启动一个临时的startup连接来准备表结构,这样session就不用总是切换数据库和查询表是否存在
    astra_sql::MySQLpp startup_mysql(
          mysql_config["address"],
          mysql_config["port"],
          mysql_config["userName"],
          mysql_config["password"]
      );
      
    if(startup_mysql.switchDatabase("ChatServer") !=astra_sql::SQLppError::success)
    {
        //因为创建新的数据库要有授权,所以这里不新建
        throw std::runtime_error("fail to select ChatServer database");
    }
    //没有表就建表
    if (!startup_mysql.mysqlTableExists("users")) {
        auto createRule = std::vector<astra_sql::createTableRule>{
            {"uid", "int", "not null auto_increment"},
            {"userName", "varchar(50)", "not null"},
            {"password", "varchar(50)", "not null"}
        };
        const astra_sql::primaryKeyRule pk{"uid"};
        const astra_sql::uniqueKeyRule uk{"userName"};

        if(startup_mysql.mysqlCreateTable("users", createRule, &pk, &uk)
            != astra_sql::SQLppError::success)
        {
            throw std::runtime_error("fail to create users table");
        }
    }
    doAccept();
}

//对接socket,把session塞到session_set里面
void server::doAccept() {
    serverAcceptor.async_accept([this](const boost::system::error_code &errorCode, tcp::socket clientSocket) {
        if (!errorCode) {
            std::cout << clientSocket.remote_endpoint() << " connect to ";
            std::cout << clientSocket.local_endpoint() << std::endl;
            std::make_shared<session>(std::move(clientSocket), configFile["MySQL"], redisAPI, sessionSet)->
                    start();
        }
        doAccept();
    });
}
