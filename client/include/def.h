#pragma once
#include <string>

// 公共协议类型保持跨平台，不能在这里依赖 Windows 专用的字符编码 API。
enum class message_type
{
    text,
    login_requested,
    create_user_requested
};

struct message
{
    std::string data;
    message_type type;
};
