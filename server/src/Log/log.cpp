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
// 这里只用到 SetConsoleOutputCP 这一个系统函数，所以不整个 include <windows.h>。
// 那样子会把一大堆宏（min / max / ERROR 之类）倒进来，很容易和其他头文件打架,
// 也会让编译变慢。自己声明一下，链接的时候照样能找到 kernel32 里的实现。
extern "C" int __stdcall SetConsoleOutputCP(unsigned int codePage);
#endif

namespace astra_log {

    // ========================================================================
    //  等级名
    // ========================================================================

    const char *levelName(const Level level) {
        // 返回的都是字符串字面量，生命周期是整程序，返回指针是安全的
        switch (level) {
            case Level::Debug: return "DEBUG";
            case Level::Info: return "INFO";
            case Level::Warn: return "WARN";
            case Level::Error: return "ERROR";
        }
        // 正常走不到这里，防御性地返回一个占位符
        return "UNKNOWN";
    }


    // ========================================================================
    //  内部工具（外面看不到）
    // ========================================================================

    namespace {

        // 默认日志文件名，路径相对于程序当前工作目录。
        // 双击 exe 运行时，当前目录就是 exe 所在目录，所以日志会和 exe 放一起。
        constexpr const char *DEFAULT_FILE_NAME = "server.log";

        // 默认单文件上限：4 MB。超过就把旧文件改名，重新写新的。
        constexpr std::size_t DEFAULT_MAX_FILE_SIZE = 4 * 1024 * 1024;

        // 取当前时间，格式化成 2026-09-27 14:03:22.123。
        //
        // 为什么要自己拼毫秒？因为 put_time 只能格式化到秒，
        // 而排查问题时，同一秒里发生的先后顺序往往很关键，
        // 所以这里单独把毫秒取出来接在后面。
        std::string timeStamp() {
            using namespace std::chrono;

            // 当前时刻
            const auto now = system_clock::now();
            // 取出毫秒部分：总毫秒数对 1000 取余
            const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
            // 转成 C 的 time_t，方便用 localtime 拆成年月日时分秒
            const std::time_t t = system_clock::to_time_t(now);

            std::tm tmBuf{};
#ifdef _WIN32
            // Windows 用 localtime_s，Linux 用 localtime_r，
            // 两个都不是标准 C++ 的，所以要分开写
            localtime_s(&tmBuf, &t);
#else
            localtime_r(&t, &tmBuf);
#endif

            std::ostringstream oss;
            oss << std::put_time(&tmBuf, "%Y-%m-%d %H:%M:%S")
                    // setfill('0') + setw(3) 把 7 毫秒补成 "007"
                    << '.' << std::setfill('0') << std::setw(3) << ms.count();
            return oss.str();
        }

        // 由当前日志文件名推出历史文件名：server.log -> server.1.log
        //
        // 做法是把扩展名拆下来，在中间插一个 ".1"。
        // 拆成 目录 / 主名 / 扩展名 三段分别处理，这样即使路径里有目录
        // （比如 logs/server.log）也能正确得到 logs/server.1.log。
        std::string rotatedName(const std::string &path) {
            const std::filesystem::path p(path);
            const std::string stem = p.stem().string(); // server
            const std::string ext = p.extension().string(); // .log
            return (p.parent_path() / (stem + ".1" + ext)).string();
        }

    } // namespace


    // ========================================================================
    //  日志器本体
    // ========================================================================

    namespace detail {

        // 全局唯一的日志器。
        //
        // 为什么用单例？因为日志系统本质上是「全程序共享的一个输出通道」：
        // 大家都往同一个文件写，就必然要共用同一个文件流和同一把锁。
        // 做成单例，这些共享状态就只有一份，不用到处传。
        class Logger {
        public:
            // 取全局实例。C++11 起，函数内的 static 变量初始化是线程安全的，
            // 所以这里不需要额外加锁。
            static Logger &instance() {
                static Logger logger;
                return logger;
            }

            // ---- 设置项 ----

            void setFile(std::string path) {
                std::lock_guard<std::mutex> lock(mutex_);
                // 换文件之前先把旧的关掉，否则句柄会一直占着
                if (file_.is_open()) {
                    file_.close();
                }
                filePath_ = std::move(path);
                openFile();
            }

            // 等级和开关都是单独的一个原子变量，改动时不加锁也不会读到半截值
            void setMinLevel(const Level level) { minLevel_.store(level); }
            void setConsoleEnabled(const bool enabled) { consoleEnabled_.store(enabled); }

            void setMaxFileSize(const std::size_t bytes) {
                std::lock_guard<std::mutex> lock(mutex_);
                // 设成 0 会让每次写完都切分，没有意义，所以兜个底
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
                // 返回一份拷贝：调用方拿到之后不用管锁，也不会被后续写入改掉
                return counter_;
            }

            void resetStats() {
                std::lock_guard<std::mutex> lock(mutex_);
                counter_ = Stats{};
            }

            // ---- 写日志 ----

            void write(const Level level, const std::string &message) {
                // 第一步：等级不够就直接返回。
                // 这里不加锁，读的是原子量，几乎不花时间 —— 被过滤掉的日志
                // 连字符串都不会去拼，这是把过滤放在最前面的原因。
                if (level < minLevel_.load()) {
                    return;
                }

                // 第二步：拼出这一行的正文。
                // 这一步只碰局部变量，不涉及共享状态，所以也在锁外面做，
                // 尽量缩短持锁时间。
                std::ostringstream line;
                line << '[' << timeStamp() << "] "
                        << '[' << levelName(level) << "] "
                        << message;
                const std::string text = line.str();

                // 第三步：加锁，动共享状态
                std::lock_guard<std::mutex> lock(mutex_);

                // 计数：只统计真正写出去的，被过滤的不算
                ++counter_.total;
                switch (level) {
                    case Level::Debug: ++counter_.debug; break;
                    case Level::Info: ++counter_.info; break;
                    case Level::Warn: ++counter_.warn; break;
                    case Level::Error: ++counter_.error; break;
                }

                // 第一次真的要写日志时，才把文件真正打开（懒加载）。
                //
                // 以前是在构造函数里就打开，那样即使程序从头到尾没写过一行日志，
                // 也会在磁盘上留下一个空文件；而且 setLogFile() 改路径之前，
                // 默认那个 server.log 已经先被创建出来了，会多一个没用的空文件。
                if (!file_.is_open()) {
                    openFile();
                }

                // 写之前先看看文件是不是太大了，太大就切分
                rotateIfTooBig();

                if (file_.is_open()) {
                    file_ << text << '\n';
                    // 每行都 flush。不 flush 的话，日志会先攒在内存缓冲区里，
                    // 程序一旦崩溃或被杀掉，最后那批日志就没了 ——
                    // 而恰恰是崩溃前的日志最有价值，所以这里宁可慢一点。
                    file_.flush();
                }

                if (consoleEnabled_.load()) {
                    std::cout << text << std::endl;
                }
            }

        private:
            Logger() {
#ifdef _WIN32
                // Windows 控制台默认是 GBK 代码页，直接输出 UTF-8 的中文会乱码。
                // 切到 65001（UTF-8）就正常了。
                SetConsoleOutputCP(65001);
#endif
                // 注意：这里刻意不打开日志文件，等到第一次写的时候再开。
                // 原因见 write() 里的说明。
            }

            // 打开日志文件。用 app（追加）模式，重启程序不会清空以前的日志。
            void openFile() {
                file_.open(filePath_, std::ios::out | std::ios::app);
                if (!file_.is_open()) {
                    std::cerr << "[astra_log] 无法打开日志文件: " << filePath_ << std::endl;
                }
            }

            // 检查文件是否超过上限，超了就轮转。调用时必须已经持有锁。
            void rotateIfTooBig() {
                if (!file_.is_open()) {
                    return;
                }

                // 用文件流的写指针位置当作当前文件大小。
                // 因为是追加模式打开的，打开时 tellp 就等于原文件大小。
                const std::streamoff currentSize = file_.tellp();
                if (currentSize < 0) {
                    // 取不到位置（极少见），那就干脆不切分，保证不影响正常写日志
                    return;
                }
                if (static_cast<std::size_t>(currentSize) < maxFileSize_) {
                    return;
                }

                // 先把当前文件关掉，Windows 下不关是没法改名的
                file_.close();

                std::error_code ec;
                const std::string rotated = rotatedName(filePath_);
                // 旧的历史文件直接删掉：只保留一份历史，不会无限堆积
                std::filesystem::remove(rotated, ec);
                ec.clear();
                // 当前文件改名为历史文件
                std::filesystem::rename(filePath_, rotated, ec);
                if (ec) {
                    std::cerr << "[astra_log] 日志文件切分失败: " << ec.message() << std::endl;
                }

                // 重新开一个新的空文件接着写
                openFile();
            }

            // ---- 成员 ----

            std::string filePath_{DEFAULT_FILE_NAME}; // 当前日志文件路径
            std::ofstream file_; // 文件流
            std::size_t maxFileSize_{DEFAULT_MAX_FILE_SIZE}; // 单文件上限（字节）
            std::atomic<Level> minLevel_{Level::Debug}; // 最低记录等级
            std::atomic<bool> consoleEnabled_{true}; // 是否同时打印到终端
            std::mutex mutex_; // 保护 file_ 和 counter_
            Stats counter_{}; // 运行统计
        };

    } // namespace detail


    // ========================================================================
    //  对外接口：都只是把请求转给单例
    // ========================================================================

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
