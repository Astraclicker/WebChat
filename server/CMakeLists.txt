# 项目没有使用 CMake 4.x 特性；使用发行版自带的 3.28 即可构建。
cmake_minimum_required(VERSION 3.28)
project(server)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 缺失的仓库子模块不能作为依赖来源，统一查找系统 Boost。
find_package(Boost REQUIRED)

#使用utf-8
if (MSVC)
    add_compile_options(/utf-8)
else ()
    add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
endif ()

set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(SQL++_DIR ${CMAKE_SOURCE_DIR}/lib/SQL++)

#添加子目录
add_subdirectory(src)

#按配置将可执行文件输出到bin目录
foreach (_cfg Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${_cfg}" _CFG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_CFG} ${PROJECT_SOURCE_DIR}/bin/${_cfg})
endforeach ()
unset(_cfg)
unset(_CFG)

add_executable(${PROJECT_NAME} main.cpp)

target_include_directories(
        ${PROJECT_NAME}
        PUBLIC
        ${CMAKE_SOURCE_DIR}/lib/SQL++
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        Web
)
