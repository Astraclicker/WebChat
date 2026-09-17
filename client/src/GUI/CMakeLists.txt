project(GUI)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_library(
        ${PROJECT_NAME}
        SHARED
        mainWidget.cpp
        serverWidget.cpp
)

target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Web
)