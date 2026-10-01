#include "log.h"
#include <atomic>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <system_error>
#include <utility>

#ifdef _WIN32
extern "C" int __stdcall SetConsoleOutputCP(unsigned int codePage);
#endif

namespace astra_log {
    const char *levelName(const Level level) {
        switch (level) {
            case Level::Debug:
                return "DEBUG";
            case Level::Info:
                return "INFO";
            case Level::Warn:
                return "WARN";
            case Level::Error:
                return "ERROR";
        }
        return "UNKNOWN";
    }

    namespace {
        // 默认日志文件名
        constexpr const char *DEFAULT_FILE_NAME = "app.log";

        // 默认单文件上限 4 MB
        constexpr std::size_t DEFAULT_MAX_FILE_SIZE = 4 * 1024 * 1024;

        // 当前时间，形如 2026-09-29 14:03:22.123
        // put_time 只能到秒，毫秒单独取出来拼在后面
        std::string timeStamp() {
            using namespace std::chrono;

            const auto now = system_clock::now();
            const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
            const std::time_t t = system_clock::to_time_t(now);

            std::tm tmBuf{};
#ifdef _WIN32
            localtime_s(&tmBuf, &t); // Windows 用 localtime_s
#else
            localtime_r(&t, &tmBuf); // Linux 用 localtime_r
#endif

            std::ostringstream oss;
            oss << std::put_time(&tmBuf, "%Y-%m-%d %H:%M:%S")
                    << '.' << std::setfill('0') << std::setw(3) << ms.count();
            return oss.str();
        }

        // app.log -> app.1.log
        std::string rotatedName(const std::string &path) {
            const std::filesystem::path p(path);
            const std::string stem = p.stem().string();
            const std::string ext = p.extension().string();
            return (p.parent_path() / (stem + ".1" + ext)).string();
        }
    } // namespace

    namespace detail {
        // 全局唯一的日志器。日志是"全程序共用的输出通道"，
        // 所以文件流和锁只需要一份。
        class Logger {
        public:
            // C++11 起函数内 static 的初始化是线程安全的
            static Logger &instance() {
                static Logger logger;
                return logger;
            }

            void setFile(std::string path) {
                std::lock_guard<std::mutex> lock(mutex_);
                if (file_.is_open()) {
                    file_.close();
                }
                filePath_ = std::move(path);
                openFile();
            }

            // 等级和开关是原子量，读的时候不用加锁
            void setMinLevel(const Level level) { minLevel_.store(level); }
            void setConsoleEnabled(const bool enabled) { consoleEnabled_.store(enabled); }

            void setMaxFileSize(const std::size_t bytes) {
                std::lock_guard<std::mutex> lock(mutex_);
                maxFileSize_ = bytes == 0 ? DEFAULT_MAX_FILE_SIZE : bytes;
            }

            void flush() {
                std::lock_guard<std::mutex> lock(mutex_);
                if (file_.is_open()) {
                    file_.flush();
                }
            }

            Stats stats() {
                std::lock_guard<std::mutex> lock(mutex_);
                return counter_; // 返回拷贝，调用方不用管锁
            }

            void resetStats() {
                std::lock_guard<std::mutex> lock(mutex_);
                counter_ = Stats{};
            }

            void write(const Level level, const std::string &message) {
                // 等级不够直接丢掉，连字符串都不拼
                if (level < minLevel_.load()) {
                    return;
                }

                // 拼日志正文。只碰局部变量，所以在锁外面做
                std::ostringstream line;
                line << '[' << timeStamp() << "] "
                        << '[' << levelName(level) << "] "
                        << message;
                const std::string text = line.str();

                std::lock_guard<std::mutex> lock(mutex_);

                // 只统计真正写出去的
                ++counter_.total;
                switch (level) {
                    case Level::Debug:
                        ++counter_.debug;
                        break;
                    case Level::Info:
                        ++counter_.info;
                        break;
                    case Level::Warn:
                        ++counter_.warn;
                        break;
                    case Level::Error:
                        ++counter_.error;
                        break;
                }

                // 第一次真要写的时候才打开文件（懒加载）
                if (!file_.is_open()) {
                    openFile();
                }

                rotateIfTooBig();

                if (file_.is_open()) {
                    file_ << text << '\n';
                    file_.flush(); // 每行都刷，程序崩了日志也不丢
                }

                if (consoleEnabled_.load()) {
                    std::cout << text << std::endl;
                }
            }

        private:
            Logger() {
#ifdef _WIN32
                SetConsoleOutputCP(65001); // 控制台切 UTF-8，中文才不乱码
#endif
                // 这里不打开文件，等第一次写的时候再开
            }

            // 追加模式，重启程序不清空旧日志
            void openFile() {
                file_.open(filePath_, std::ios::out | std::ios::app);
                if (!file_.is_open()) {
                    std::cerr << "[astra_log] 无法打开日志文件: " << filePath_ << std::endl;
                }
            }

            // 超过上限就切分，调用时必须已经持有锁
            void rotateIfTooBig() {
                if (!file_.is_open()) {
                    return;
                }

                // 追加模式下，tellp 就是当前文件大小
                const std::streamoff currentSize = file_.tellp();
                if (currentSize < 0) {
                    return;
                }
                if (static_cast<std::size_t>(currentSize) < maxFileSize_) {
                    return;
                }

                // Windows 下不关文件没法改名
                file_.close();

                std::error_code ec;
                const std::string rotated = rotatedName(filePath_);
                std::filesystem::remove(rotated, ec); // 只留一份历史
                ec.clear();
                std::filesystem::rename(filePath_, rotated, ec);
                if (ec) {
                    std::cerr << "[astra_log] 日志文件切分失败: " << ec.message() << std::endl;
                }

                openFile();
            }

            std::string filePath_{DEFAULT_FILE_NAME};
            std::ofstream file_;
            std::size_t maxFileSize_{DEFAULT_MAX_FILE_SIZE};
            std::atomic<Level> minLevel_{Level::Debug};
            std::atomic<bool> consoleEnabled_{true};
            std::mutex mutex_; // 保护 file_ 和 counter_
            Stats counter_{};
        };
    } // namespace detail

    // 下面这些都是把请求转给单例
    void log(const std::string &message, const Level level) {
        detail::Logger::instance().write(level, message);
    }

    void flush() {
        detail::Logger::instance().flush();
    }

    void setLogFile(const std::string &path) {
        detail::Logger::instance().setFile(path);
    }

    void setMinLevel(const Level level) {
        detail::Logger::instance().setMinLevel(level);
    }

    void setConsoleEnabled(const bool enabled) {
        detail::Logger::instance().setConsoleEnabled(enabled);
    }

    void setMaxFileSize(const std::size_t bytes) {
        detail::Logger::instance().setMaxFileSize(bytes);
    }

    Stats stats() {
        return detail::Logger::instance().stats();
    }

    void resetStats() {
        detail::Logger::instance().resetStats();
    }
} // namespace astra_log
