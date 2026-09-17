project(SQLite++)

add_library(
        ${PROJECT_NAME}
        SHARED
        SQLitepp.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        SQLiteCpp
)

add_subdirectory(${SQLite_DIR} ${CMAKE_BINARY_DIR}/SQL++/SQLiteCpp)