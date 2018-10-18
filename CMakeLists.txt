#cmake -DCMAKE_BUILD_TYPE=Release .
#cmake -DCMAKE_BUILD_TYPE=Debug .

cmake_minimum_required(VERSION 3.1)

project(f8n)

set (f8n_VERSION_MAJOR 0)
set (f8n_VERSION_MINOR 1)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")

set (LIBRARY_OUTPUT_PATH ${f8n_SOURCE_DIR}/__output)

include_directories (
  "${f8n_SOURCE_DIR}/src/"
  "${f8n_SOURCE_DIR}/src/3rdparty/include"
)

set (F8N_SRCS
  ./src/3rdparty/src/sqlite/sqlite3.c
  ./src/3rdparty/src/base64/base64.cpp
  ./src/f8n/f8n.cpp
  ./src/f8n/db/Connection.cpp
  ./src/f8n/db/ScopedTransaction.cpp
  ./src/f8n/db/Statement.cpp
  ./src/f8n/debug/debug.cpp
  ./src/f8n/i18n/Locale.cpp
  ./src/f8n/runtime/Message.cpp
  ./src/f8n/runtime/MessageQueue.cpp
  ./src/f8n/plugins/Plugins.cpp
  ./src/f8n/preferences/Preferences.cpp
  ./src/f8n/environment/Environment.cpp
)

add_library(f8n STATIC ${F8N_SRCS})

target_link_libraries(f8n dl pthread)

#file(GLOB sdk_headers "src/*.h")

#install(
#  FILES ${sdk_headers}
#  DESTINATION include/f8n
#)

# install(
#   FILES lib/libf8n.a
#   DESTINATION lib/
# )
