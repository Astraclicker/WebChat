project(src)

# Log 要放在前面：Web 会链接它，得先有这个 target
add_subdirectory(Log)
add_subdirectory(Web)
add_subdirectory(MySQL)
