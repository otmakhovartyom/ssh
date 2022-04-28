cmake_minimum_required(VERSION 3.1)

project(Server_SHH VERSION 0.1)

set(EXEC_CLIENT "client")

message(huy ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sources/*.c)

add_executable(${EXEC_CLIENT} ${SOURCES})

target_include_directories(${EXEC_CLIENT} PRIVATE headers)