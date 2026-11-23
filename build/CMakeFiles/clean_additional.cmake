# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles\\RoboCellTests_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\RoboCellTests_autogen.dir\\ParseCache.txt"
  "CMakeFiles\\RoboCell_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\RoboCell_autogen.dir\\ParseCache.txt"
  "RoboCellTests_autogen"
  "RoboCell_autogen"
  )
endif()
