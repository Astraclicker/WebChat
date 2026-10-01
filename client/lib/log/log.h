#pragma once

#include <cstddef>
#include <sstream>
#include <string>

// 用法：
//     astra_log::log("内容", astra_log::Level::Info);
//     LOG(astra_log::Level::Warn, "磁盘剩余 ", percent, "%");
//
// 输出：[2026-09-29 14:03:22.123] [INFO] 内容

namespace astra_log {

    // 等级枚举
    enum class Level {
        Debug = 0, // 调试细节
        Info = 1,  // 正常节点
        Warn = 2,  // 有问题但能跑
        Error = 3, // 出错了
    };

    // 等级转字符串，例如 Level::Info -> "INFO"
    const char *levelName(Level level);

    // 写一条日志
    void log(const std::string &message, Level level);

    // 把缓冲区刷到磁盘
    void flush();

    // 换日志文件，默认 app.log
    void setLogFile(const std::string &path);

    // 设置最低记录等级，低于它的会被丢掉，默认 Debug
    void setMinLevel(Level level);

    // 是否同时打印到终端，默认 true
    void setConsoleEnabled(bool enabled);

    // 单个日志文件的大小上限，默认 4 MB，超过就切分
    void setMaxFileSize(std::size_t bytes);

    // 运行统计
    struct Stats {
        std::size_t total = 0;
        std::size_t debug = 0;
        std::size_t info = 0;
        std::size_t warn = 0;
        std::size_t error = 0;
    };

    // 取当前统计
    Stats stats();

    // 把统计清零
    void resetStats();

    // 把任意几个值拼成字符串，给 log() 用（C++17 折叠表达式）
    template<typename... Args>
    std::string format(Args &&... args) {
        std::ostringstream oss;
        (oss << ... << args);
        return oss.str();
    }

} // namespace astra_log

// 用宏是因为函数没法接收变长参数
#define LOG(level, ...) ::astra_log::log(::astra_log::format(__VA_ARGS__), level)
