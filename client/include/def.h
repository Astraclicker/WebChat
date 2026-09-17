#pragma once
#include <string>

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
