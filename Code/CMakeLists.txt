cmake_minimum_required(VERSION 3.12)
project(Code)

set(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_executable(ex4 MemoryConstants.h PhysicalMemory.cpp PhysicalMemory.h VirtualMemory.cpp VirtualMemory.h SimpleTest.cpp)