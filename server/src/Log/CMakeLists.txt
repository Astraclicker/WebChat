project(Log)

# 日志系统。
#
# 这里刻意做成 STATIC（静态库），而不是 SHARED（动态库）：
#
#   静态库会被直接编进「最终链接它的那个目标」里。这个工程里只有 Web 库
#   链接它（见 src/Web/CMakeLists.txt），所以整个程序里就只有一份
#   Logger 实例 —— 文件流、互斥锁、运行统计都只有一份，这才是单例该有的样子。
#
#   如果做成 SHARED，或者让 Web 和 MySQL 都各链一份静态库，
#   那就会出现好几个互不相干的单例：各自开一个文件句柄往同一个文件里写，
#   stats() 也只能看到自己那份计数，等于白做。
#
#   注意：Web 是用 PRIVATE 链接 Log 的。这样 Log 就只存在于 Web.dll 内部，
#   不会又被链进 exe 一遍（那样 exe 里会出现第二个实例）。
#   exe 里用到日志时，符号是从 Web.dll 里导出来的（Web 设了
#   WINDOWS_EXPORT_ALL_SYMBOLS），拿到的还是同一个单例。
#
add_library(
        ${PROJECT_NAME}
        STATIC
        log.cpp
)

# 让 log.cpp 自己能找到 log.h（server/include/log.h）
target_include_directories(
        ${PROJECT_NAME}
        PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)
