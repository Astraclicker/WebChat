project(MySQL)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

FetchContent_MakeAvailable(SQL++)
FetchContent_MakeAvailable(boost)

add_library(
        ${PROJECT_NAME}
        SHARED
        MySql.cpp
)

target_include_directories(
        ${PROJECT_NAME}
        PUBLIC
        ${INCLUDE_DIR}
        ${SQL++_DIR}
)

target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        MySQL++
        Boost::asio
)
