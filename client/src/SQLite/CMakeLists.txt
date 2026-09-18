project(SQLite)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

FetchContent_MakeAvailable(SQL++)

add_library(
        ${PROJECT_NAME}
        SQLite.cpp
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
        SQLite++
)
