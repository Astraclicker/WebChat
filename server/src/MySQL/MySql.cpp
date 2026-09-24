#include "MySql.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

//登录
void login(
    astra_sql::MySQLpp &mysqlAPI,
    const std::string &loginUserName,
    const std::string &password,
    tcp::socket &sessionSocker,
    std::string &userName
) {
    const std::vector<std::string> userData{
        "userName",
        "password"
    };
    const astra_sql::itemRule rule{
        {"userName", "=", loginUserName, "and"},
        {"password", "=", password, "and"}
    };
    const auto json = mysqlAPI.searchItem("users", userData, rule);
    const bool ok = json.is_object() && !json.value("userName", nlohmann::json::array()).empty();

    nlohmann::json back;
    back["type"] = "mysqlLoginFeedBack";
    if (ok) {
        back["data"] = "success";
        userName = loginUserName;
    } else {
        back["data"] = "failed";
    }
    // async_write 完成前缓冲区必须存活，由回调共同持有响应字符串。
    auto send_msg = std::make_shared<std::string>(back.dump() + '\n');
    boost::asio::async_write(
        sessionSocker, boost::asio::buffer(*send_msg),
        [send_msg](const boost::system::error_code &, size_t) {
        });
}

//注册用户
bool createUser(
    astra_sql::MySQLpp &mysqlAPI,
    const std::string &loginUserName,
    const std::string &password,
    tcp::socket &sessionSocker)
{
    nlohmann::json back;
    back["type"] = "mysqlCreateUserFeedBack";

    bool ok = false;
    if (loginUserName.empty() || password.empty())
    {
        // 协议请求必须有且仅有一次响应；校验失败也不能直接返回让客户端一直等待。
        std::cout << "createUser failed: empty userName or password" << std::endl;
    }
    else
    {
        const astra_sql::item userData{
            {"userName", loginUserName},
            {"password", password}
        };
        const astra_sql::mysqlItemType userType{
            astra_sql::mysqlDataType::String,
            astra_sql::mysqlDataType::String,
        };
        const auto result = mysqlAPI.addItem("users", userData, userType);
        ok = result == astra_sql::SQLppError::success;
    }

    if (ok)
    {
        back["data"] = "success";
    }
    else
    {
        back["data"] = "failed";
    }

    // 局部字符串会提前析构，因此把异步发送缓冲区的生命周期绑定到完成回调。
    auto send_msg = std::make_shared<std::string>(back.dump() + '\n');

    boost::asio::async_write(
        sessionSocker, boost::asio::buffer(*send_msg),
        [send_msg](const boost::system::error_code &, size_t) {
        });
    return ok;
}

//保存一条聊天记录
bool saveChatHistory(
    astra_sql::MySQLpp &mysqlAPI,
    const std::string &sender,
    const std::string &receiver,
    const std::string &text
) {
    //时间戳由服务端生成:客户端时钟不可信,记录的落地时间不该由发送方决定
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now()
    );
    //两个平台函数名、参数顺序、返回值含义都不同:localtime_r是(时间,结构)返回空指针,
    //MSVC的localtime_s是(结构,时间)返回非0,写反了会踩坏栈。localTime值初始化防读到垃圾
    std::tm localTime{};
#if defined(_WIN32)
    if (localtime_s(&localTime, &nowTime) != 0) {
        std::cerr << "localtime_s failed" << std::endl;
        return false;
    }
#else
    //用_r版本:localtime返回静态缓冲区,不能用于多线程io_context
    if (localtime_r(&nowTime, &localTime) == nullptr) {
        std::cerr << "localtime_r failed" << std::endl;
        return false;
    }
#endif
    std::ostringstream timeStream;
    timeStream << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");

    const astra_sql::item chatData{
        {"sender", sender},
        {"receiver", receiver},
        {"text", text},
        {"sendTime", timeStream.str()}
    };
    const astra_sql::mysqlItemType chatType{
        astra_sql::mysqlDataType::String,
        astra_sql::mysqlDataType::String,
        astra_sql::mysqlDataType::String,
        //DataTime对应setDateTime,接受"YYYY-MM-DD HH:MM:SS"格式的字符串
        astra_sql::mysqlDataType::DataTime
    };

    const auto result = mysqlAPI.addItem("chat_history", chatData, chatType);
    return result == astra_sql::SQLppError::success;
}
