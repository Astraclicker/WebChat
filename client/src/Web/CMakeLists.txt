project(Web)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_library(
        ${PROJECT_NAME}
        SHARED
        client.cpp
)

target_include_directories(
        ${PROJECT_NAME}
        PUBLIC
        ${INCLUDE_DIR}
)

target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        Boost::headers
)
