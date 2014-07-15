file(GLOB_RECURSE sources_BaseObjects BaseObjects/*.cpp Baseobjects/*.h)
file(GLOB_RECURSE sources_JsEngine JsEngine/*.cpp Jsengine/*.h)
file(GLOB_RECURSE sources_Network Network/*.cpp Network/*.h)
file(GLOB_RECURSE sources_Other Other/*.cpp Other/*.h)
file(GLOB_RECURSE sources_Scripts Scripts/*.cpp Scripts/*.h)
file(GLOB_RECURSE sources_Subsystem Subsystem/*.cpp Subsystem/*.h)

set(system_STAT_SRCS
  ${system_STAT_SRCS}
  ${sources_BaseObjects}
  ${sources_JsEngine}
  ${sources_Network}
  ${sources_Other}
  ${sources_Scripts}
  ${sources_Subsystem}
)

include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/dep/spidermonkey
  ${CMAKE_SOURCE_DIR}/src/uox3
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/BaseObjects
  ${CMAKE_CURRENT_SOURCE_DIR}/JsEngine
  ${CMAKE_CURRENT_SOURCE_DIR}/Network
  ${CMAKE_CURRENT_SOURCE_DIR}/Other
  ${CMAKE_CURRENT_SOURCE_DIR}/Scripts
  ${CMAKE_CURRENT_SOURCE_DIR}/Subsystem
  ${MYSQL_INCLUDE_DIR}
  ${OPENSSL_INCLUDE_DIR}
)

add_library(system STATIC
  ${system_STAT_SRCS}
)