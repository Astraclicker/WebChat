project(Web)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

FetchContent_MakeAvailable(boost)

add_library(
        ${PROJECT_NAME}
        session.cpp
        server.cpp
)

target_include_directories(
        ${PROJECT_NAME}
        PUBLIC
        ${INCLUDE_DIR}
)

target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        Boost::asio
        MySQL
        Redis++
)

# 日志库。用 PRIVATE 链接：
#   Log 会被编进 Web.dll 内部，只此一份单例；
#   同时不会顺着链接接口传给 exe，避免 exe 里又出现第二个实例。
# exe（server/main.cpp）要用日志时，符号从 Web.dll 导出即可。
target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        Log
)
