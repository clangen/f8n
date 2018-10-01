#cmake -DCMAKE_BUILD_TYPE=Release .
#cmake -DCMAKE_BUILD_TYPE=Debug .

cmake_minimum_required(VERSION 3.0)

project(f8n)

set (f8n_VERSION_MAJOR 0)
set (f8n_VERSION_MINOR 1)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/Modules)

# SET (CMAKE_BUILD_TYPE DEBUG)
set (LIBRARY_OUTPUT_PATH ${f8n_SOURCE_DIR}/lib)

include_directories (
  "${f8n_SOURCE_DIR}/src/"
  "${f8n_SOURCE_DIR}/src/3rdparty/include"
)

# "/usr/local" doesn't seem to be included by default on macOS 10.12+
if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
  link_directories ("/usr/local/lib")
  include_directories("/usr/local/include")
endif (CMAKE_SYSTEM_NAME MATCHES "Darwin")

set (F8N_SRCS
    ./src/3rdparty/src/sqlite/sqlite3.c
    ./src/f8n/debug/debug.cpp
    ./src/f8n/i18n/Locale.cpp
    ./src/f8n/runtime/Message.cpp
    ./src/f8n/runtime/MessageQueue.cpp
    ./src/f8n/plugins/Plugins.cpp
    ./src/f8n/preferences/Preferences.cpp
    ./src/f8n/environment/Environment.cpp
)

add_library(f8n STATIC ${F8N_SRCS})

# target_link_libraries(f8n curses panel)

#file(GLOB sdk_headers "src/*.h")

#install(
#    FILES ${sdk_headers}
#    DESTINATION include/f8n
#)

install(
  FILES lib/libf8n.a
  DESTINATION lib/
)
