project(Redis++)

add_library(
    ${PROJECT_NAME}
    SHARED
    Redispp.cpp
)

#导出所有符号
set_target_properties(${PROJECT_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

set(BUILD_SHARED_LIBS OFF)
set(DISABLE_TESTS ON CACHE BOOL "Disable hiredis tests" FORCE)
set(REDIS_PLUS_PLUS_BUILD_STATIC ON)
set(REDIS_PLUS_PLUS_BUILD_SHARED OFF)
set(REDIS_PLUS_PLUS_BUILD_TEST OFF CACHE BOOL "Build tests for redis++" FORCE)

# Linux/macOS 下 hiredis/redis++ 静态库以 -fPIC 编译，链接进共享库 libRedis++.so
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 先用 lib 下自带的 hiredis 源码，OVERRIDE_FIND_PACKAGE 让 redis-plus-plus 的 find_package(hiredis) 复用
include(FetchContent)
set(FETCHCONTENT_SOURCE_DIR_HIREDIS ${Hiredis_DIR})
FetchContent_Declare(hiredis OVERRIDE_FIND_PACKAGE)
FetchContent_MakeAvailable(hiredis)

# redis-plus-plus 的 FindHiredis 需要这个变量指向包含 hiredis/ 头文件目录的父目录
get_filename_component(hiredis_INCLUDE_DIRS ${Hiredis_DIR} DIRECTORY)

add_subdirectory(${Redis_DIR} ${CMAKE_BINARY_DIR}/SQL++/redis-plus-plus)

if(REDIS_PLUS_PLUS_BUILD_SHARED)
    set(Redispp_LINK_LIB redis++::redis++)
else()
    set(Redispp_LINK_LIB redis++::redis++_static)
endif()


target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC
    ${Redispp_LINK_LIB}
)
