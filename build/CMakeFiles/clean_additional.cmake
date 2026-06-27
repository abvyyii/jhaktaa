# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "RelWithDebInfo")
  file(REMOVE_RECURSE
  "CMakeFiles\\jhatkaa_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\jhatkaa_autogen.dir\\ParseCache.txt"
  "jhatkaa_autogen"
  )
endif()
