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
)
