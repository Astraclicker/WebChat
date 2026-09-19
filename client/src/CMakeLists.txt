# project(src)

FetchContent_MakeAvailable(boost)

add_executable(
        ${PROJECT_NAME}
        main.cpp
)

target_include_directories(
        ${PROJECT_NAME}
        PUBLIC
        ${CMAKE_SOURCE_DIR}/lib/SQL++
        ${CMAKE_CURRENT_SOURCE_DIR}
)


target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        Boost::asio
        Web
        GUI
)


if(WIN32)
    find_program(WINDEPLOYQT_EXE windeployqt HINTS "${CMAKE_PREFIX_PATH}/bin" REQUIRED)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${WINDEPLOYQT_EXE}" "$<TARGET_FILE:${PROJECT_NAME}>"
            COMMENT "Deploy Qt runtime dependencies"
    )
endif()

add_subdirectory(SQLite)
add_subdirectory(GUI)
add_subdirectory(Web)