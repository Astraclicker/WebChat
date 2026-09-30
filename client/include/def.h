#pragma once
#include <ctime>
#include <string>

enum class message_type
{
    text,
    login_requested,
    create_user_requested,
    sync_requested
};

struct message
{
    std::string data;
    message_type type;
};

//本地时间戳(YYYY-MM-DD HH:MM:SS)
inline std::string nowLocalTimestamp() {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
#if defined(_WIN32)
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    char buffer[32] = {};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &local);
    return buffer;
}
