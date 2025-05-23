cmake_minimum_required(VERSION 3.20)

project(jt808)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)

option(JT808_BUILD_EXAMPLES "Build jt808 examples" OFF)

set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O2 -Wall -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wfatal-errors")

set(CMAKE_LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_INCLUDE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/include)

set(jt808_include_dirs ${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(SYSTEM ${jt808_include_dirs})

aux_source_directory(src DIR_SRCS)
set(HDRS include/jt808/multimedia_upload.h
  include/jt808/bcd.h
  include/jt808/protocol_parameter.h
  include/jt808/socket_util.h
  include/jt808/util.h
  include/jt808/parser.h
  include/jt808/packager.h
  include/jt808/terminal_parameter.h
  include/jt808/location_report.h
  include/jt808/area_route.h
  include/jt808/client.h
  include/jt808/server.h
)

# add_subdirectory(nmeaparser)

add_library(${PROJECT_NAME}
  ${DIR_SRCS} ${HDRS}
)

if(WIN32)
target_link_libraries(${PROJECT_NAME}
  ws2_32
)
endif(WIN32)

set_target_properties(${PROJECT_NAME} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${jt808_include_dirs})

if (JT808_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif (JT808_BUILD_EXAMPLES)
