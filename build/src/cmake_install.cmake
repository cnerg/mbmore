# Install script for directory: /Users/mbmcgarry/git/mbmore/src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/Users/mbmcgarry/.local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "mbmore")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cyclus/." TYPE SHARED_LIBRARY FILES "/Users/mbmcgarry/git/mbmore/build/lib/cyclus/mbmore/libmbmore.dylib")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cyclus/./libmbmore.dylib" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cyclus/./libmbmore.dylib")
    execute_process(COMMAND "/opt/local/bin/install_name_tool"
      -id "libmbmore.dylib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cyclus/./libmbmore.dylib")
    execute_process(COMMAND /opt/local/bin/install_name_tool
      -add_rpath "/Users/mbmcgarry/.local/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cyclus/./libmbmore.dylib")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/opt/local/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cyclus/./libmbmore.dylib")
    endif()
  endif()
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "mbmore")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/cyclus" TYPE FILE FILES
    "/Users/mbmcgarry/git/mbmore/build/mbmore/mytest.h"
    "/Users/mbmcgarry/git/mbmore/build/mbmore/behavior_functions.h"
    "/Users/mbmcgarry/git/mbmore/build/mbmore/RandomEnrich.h"
    "/Users/mbmcgarry/git/mbmore/build/mbmore/RandomSink.h"
    )
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "mbmore_testing")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/Users/mbmcgarry/git/mbmore/build/bin/mbmore_unit_tests")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/mbmore_unit_tests" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/mbmore_unit_tests")
    execute_process(COMMAND /opt/local/bin/install_name_tool
      -add_rpath "/Users/mbmcgarry/.local/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/mbmore_unit_tests")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/opt/local/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/mbmore_unit_tests")
    endif()
  endif()
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "mbmore")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/mbmore" TYPE FILE FILES
    "/Users/mbmcgarry/git/mbmore/src/behavior_functions.h"
    "/Users/mbmcgarry/git/mbmore/src/mytest.h"
    "/Users/mbmcgarry/git/mbmore/src/RandomEnrich.h"
    "/Users/mbmcgarry/git/mbmore/src/RandomSink.h"
    )
endif()

