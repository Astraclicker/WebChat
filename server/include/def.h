#pragma once
#include <ctime>
#include <string>

//本地时间戳(YYYY-MM-DD HH:MM:SS)
//注意不能用 std::format 直接格式化 std::chrono::system_clock::now():那样得到的是 UTC,
//UTC+8 环境下会比本地时间差 8 小时,而且两端存的 sendTime 会永远对不上。
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
