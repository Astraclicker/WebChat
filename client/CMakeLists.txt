# 项目没有使用 CMake 4.x 特性；3.28 与 Ubuntu 24.04 系统包一致，避免虚假的版本阻塞。
cmake_minimum_required(VERSION 3.28)
project(client)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include(FetchContent)


#使用utf-8
if(MSVC)
        add_compile_options(/utf-8)
else()
        add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

#Windows设置Qt路径
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
                set(CMAKE_PREFIX_PATH "D:/QT/6.11.1/mingw_64" CACHE PATH "" FORCE)
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
                set(CMAKE_PREFIX_PATH "D:/QT/6.11.1/msvc2022_64" CACHE PATH "" FORCE)
        endif()
endif()

#导入qt
find_package(Qt6
        COMPONENTS
        Core
        Gui
        Widgets
        # 当前界面没有音频功能，不应强制要求 Qt Multimedia。
        REQUIRED
)

#按配置将可执行文件输出到bin目录
foreach(_cfg Debug Release RelWithDebInfo MinSizeRel)
        string(TOUPPER "${_cfg}" _CFG)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_CFG} ${PROJECT_SOURCE_DIR}/bin/${_cfg})
endforeach()
unset(_cfg)
unset(_CFG)

#设置路径宏
FetchContent_Declare(
        boost
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/lib/boost
)

FetchContent_Declare(
        SQL++
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/SQL++
)

set(SQL++_DIR ${CMAKE_SOURCE_DIR}/lib/SQL++)
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

#设置子目录
add_subdirectory(src)
