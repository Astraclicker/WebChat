# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles/client_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/client_autogen.dir/ParseCache.txt"
  "client_autogen"
  "src/GUI/CMakeFiles/GUI_autogen.dir/AutogenUsed.txt"
  "src/GUI/CMakeFiles/GUI_autogen.dir/ParseCache.txt"
  "src/GUI/GUI_autogen"
  "src/Web/CMakeFiles/Web_autogen.dir/AutogenUsed.txt"
  "src/Web/CMakeFiles/Web_autogen.dir/ParseCache.txt"
  "src/Web/Web_autogen"
  )
endif()
