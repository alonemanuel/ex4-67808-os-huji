cmake_minimum_required(VERSION 3.12)
project(EX5)

set(CMAKE_CXX_STANDARD 14)

add_executable(EX5 VirtualMemory.cpp VirtualMemory.h PhysicalMemory.cpp PhysicalMemory.h SimpleTest.cpp MemoryConstants.h)