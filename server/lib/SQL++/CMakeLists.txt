cmake_minimum_required(VERSION 4.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(SQL++)

# ===================== MySQL Connector/C++ 定位（跨平台） =====================
# MySQL 安装根目录的指定方式（按优先级）：
#   1) 命令行 -DMySQL_ROOT=<安装根目录>
#   2) 环境变量 MYSQL_ROOT 或 MYSQL_HOME
#   3) WIN32：回退到本地 lib/mysql_release、lib/mysql_debug
#   4) Unix(Linux/macOS)：自动 find_path/find_library 探测系统安装的 mysqlcppconn
if(NOT DEFINED MySQL_ROOT)
    if(DEFINED ENV{MYSQL_ROOT})
        set(MySQL_ROOT "$ENV{MYSQL_ROOT}")
    elseif(DEFINED ENV{MYSQL_HOME})
        set(MySQL_ROOT "$ENV{MYSQL_HOME}")
    else()
        set(MySQL_ROOT "")
    endif()
endif()

if(WIN32)
    # ---- Windows：使用官方二进制（区分 Release/Debug，含 vs14 目录与 DLL）----
    # 若指定了 MySQL_ROOT 但下面没有 mysql_release，仍回退到本地 lib
    if(NOT MySQL_ROOT OR NOT EXISTS "${MySQL_ROOT}/mysql_release")
        set(MySQL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/lib")
    endif()

    # Release
    set(MySQL_INCLUDE_DIR_RELEASE ${MySQL_ROOT}/mysql_release/include/jdbc)
    set(MySQL_LIB_DIR_RELEASE     ${MySQL_ROOT}/mysql_release/lib64/vs14)
    set(MySQL_DLL_DIR_RELEASE     ${MySQL_ROOT}/mysql_release/lib64)
    # Debug
    set(MySQL_INCLUDE_DIR_DEBUG   ${MySQL_ROOT}/mysql_debug/include/jdbc)
    set(MySQL_LIB_DIR_DEBUG       ${MySQL_ROOT}/mysql_debug/lib64/debug/vs14)
    set(MySQL_DLL_DIR_DEBUG       ${MySQL_ROOT}/mysql_debug/lib64/debug)

    # OpenSSL DLL（两个版本都在 lib64 根目录）
    set(MySQL_SSL_DLL_DIR_RELEASE ${MySQL_ROOT}/mysql_release/lib64)
    set(MySQL_SSL_DLL_DIR_DEBUG   ${MySQL_ROOT}/mysql_debug/lib64)

    # Debug 用 debug 版本（生成器表达式，按 CONFIG 展开）
    set(MySQL_INCLUDE_DIR
            $<$<CONFIG:Debug>:${MySQL_INCLUDE_DIR_DEBUG}>
            $<$<NOT:$<CONFIG:Debug>>:${MySQL_INCLUDE_DIR_RELEASE}>)
    set(MySQL_LIB_DIR
            $<$<CONFIG:Debug>:${MySQL_LIB_DIR_DEBUG}>
            $<$<NOT:$<CONFIG:Debug>>:${MySQL_LIB_DIR_RELEASE}>)
    set(MySQL_DLL_DIR
            $<$<CONFIG:Debug>:${MySQL_DLL_DIR_DEBUG}>
            $<$<NOT:$<CONFIG:Debug>>:${MySQL_DLL_DIR_RELEASE}>)

    # Windows 下按名称链接（导入库 mysqlcppconn.lib，靠 MySQL_LIB_DIR 定位）
    set(MySQL_LIBRARY mysqlcppconn)
    set(MySQL_COPY_DLLS TRUE)

else()
    # ---- Unix（Linux/macOS）：从 MySQL_ROOT 常规布局或系统探测 ----
    if(MySQL_ROOT)
        set(MySQL_INCLUDE_DIR "${MySQL_ROOT}/include/jdbc")
        find_library(MySQL_LIBRARY
            NAMES mysqlcppconn mysqlcppconn8 mysqlcppconn9 mysqlclient
            PATHS "${MySQL_ROOT}/lib" "${MySQL_ROOT}/lib64"
            NO_DEFAULT_PATH)
    else()
        find_path(MySQL_INCLUDE_DIR
            NAMES mysql_driver.h
            PATH_SUFFIXES include/jdbc include
            PATHS /usr /usr/local /opt/homebrew/opt/mysql-connector-c++ /opt/local)
        find_library(MySQL_LIBRARY
            NAMES mysqlcppconn mysqlcppconn8 mysqlcppconn9 mysqlclient
            PATHS /usr/lib /usr/local/lib /opt/homebrew/opt/mysql-connector-c++/lib)
    endif()

    if(NOT MySQL_INCLUDE_DIR OR NOT MySQL_LIBRARY)
        message(FATAL_ERROR
            "未找到 MySQL Connector/C++。请安装开发包，或通过 -DMySQL_ROOT=<path> 指定安装目录。")
    endif()

    get_filename_component(MySQL_LIB_DIR "${MySQL_LIBRARY}" DIRECTORY)
    set(MySQL_COPY_DLLS FALSE)
endif()

# 设置SQLite路径
set(SQLite_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/SQLiteCpp)

# 设置Redis路径
set(Redis_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/redis-plus-plus)
# 设置Hiredis路径
set(Hiredis_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/hiredis)

set(REDIS_PLUS_PLUS_BUILD_TEST OFF CACHE BOOL "Build tests for redis++" FORCE)
set(DISABLE_TESTS ON CACHE BOOL "Disable hiredis tests" FORCE)

add_subdirectory(Redis++)
add_subdirectory(MySQL++)
add_subdirectory(SQLite++)