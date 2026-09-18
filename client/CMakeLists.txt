cmake_minimum_required(VERSION 3.28)
project(client)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
find_package(Boost REQUIRED)

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
        REQUIRED
)

#按配置将可执行文件输出到bin目录
foreach(_cfg Debug Release RelWithDebInfo MinSizeRel)
        string(TOUPPER "${_cfg}" _CFG)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_CFG} ${PROJECT_SOURCE_DIR}/bin/${_cfg})
endforeach()
unset(_cfg)
unset(_CFG)

set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

#设置子目录
add_subdirectory(src)

add_executable(
        ${PROJECT_NAME}
        # 程序入口已经归入 src；显式写出新路径，避免干净配置时仍查找旧位置。
        src/main.cpp
)

# Windows 下自动复制 Qt 运行库；Linux 直接使用系统安装的 Qt。
if(WIN32)
        find_program(WINDEPLOYQT_EXE windeployqt HINTS "${CMAKE_PREFIX_PATH}/bin" REQUIRED)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND "${WINDEPLOYQT_EXE}" "$<TARGET_FILE:${PROJECT_NAME}>"
                COMMENT "Deploy Qt runtime dependencies"
        )
endif()

target_include_directories(
        ${PROJECT_NAME}
        PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)


target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        Boost::headers
        Web
        GUI
)
