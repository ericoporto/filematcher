cmake_minimum_required(VERSION 3.16.5)
project(filematcher)

set(CMAKE_CXX_STANDARD 11)
set(FILEMATCHER_SOURCES main.cpp)
add_executable(filematcher)

target_sources(filematcher PRIVATE ${FILEMATCHER_SOURCES})