STRING(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}" BUILDING_IN_SOURCE)

IF (BUILDING_IN_SOURCE)
  MESSAGE(FATAL_ERROR "
This project requires an out of source build. Create a separate
build directory and run 'cmake path_to_project [options]' from there
after removed the file 'CMakeCache.txt' found in this directory.
")
ENDIF()