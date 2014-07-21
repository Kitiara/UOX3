file(GLOB sources_localdir *.cpp *.h)

set(uox3_SRCS
  ${uox3_SRCS}
  ${sources_localdir}
)

if( WIN32 )
  set(uox3_SRCS
    ${uox3_SRCS}
    ${sources_windows_Debugging}
  )
  if ( MSVC )
    set(uox3_SRCS
      ${uox3_SRCS}
      uox3.rc
    )
  endif()
endif()

include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/dep/spidermonkey
  ${CMAKE_SOURCE_DIR}/src/system/BaseObjects
  ${CMAKE_SOURCE_DIR}/src/system/JsEngine
  ${CMAKE_SOURCE_DIR}/src/system/Network
  ${CMAKE_SOURCE_DIR}/src/system/Other
  ${CMAKE_SOURCE_DIR}/src/system/Scripts
  ${CMAKE_SOURCE_DIR}/src/system/Subsystem
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${MYSQL_INCLUDE_DIR}
  ${OPENSSL_INCLUDE_DIR}
)

add_executable(uox3
  ${uox3_SRCS}
)

if( NOT WIN32 )
  set_target_properties(uox3 PROPERTIES
    COMPILE_DEFINITIONS _UOX3_CONFIG="${CONF_DIR}/uox.ini"
  )
endif()

target_link_libraries(uox3
  system
  wsock32.lib
  ws2_32.lib
  ${MYSQL_LIBRARY}
  ${OPENSSL_LIBRARIES}
  ${CMAKE_SOURCE_DIR}/dep/lib/js32.lib
)

if( WIN32 )
  if ( MSVC )
    set(destinationdir "${CMAKE_BINARY_DIR}/bin/$(ConfigurationName)")
  elseif ( MINGW )
    set(destinationdir "${CMAKE_BINARY_DIR}/bin")
  else()
    set(destinationdir "${CMAKE_INSTALL_PREFIX}")
    install(TARGETS uox3 DESTINATION "${CMAKE_INSTALL_PREFIX}")
  endif()
elseif( UNIX )
  set(destinationdir "${CONF_DIR}")
  install(TARGETS uox3 DESTINATION bin)
endif()

file(GLOB_RECURSE allfiles RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/content" "content/*")

foreach(each_file ${allfiles})
  set(destinationfile "${destinationdir}/${each_file}")
  set(sourcefile "${CMAKE_CURRENT_SOURCE_DIR}/content/${each_file}")
    add_custom_command(TARGET uox3
      POST_BUILD
      COMMAND ${CMAKE_COMMAND}
      -Ddestinationfile=${destinationfile}
      -Dsourcefile=${sourcefile}
      -P ${CMAKE_CURRENT_SOURCE_DIR}/check.cmake
    )
endforeach(each_file)