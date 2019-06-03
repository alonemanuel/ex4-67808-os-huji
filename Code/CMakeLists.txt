cmake_minimum_required(VERSION 3.12)
project(EX4)

set(CMAKE_CXX_STANDARD 14)

add_executable(EX4 VirtualMemory.cpp VirtualMemory2.h PhysicalMemory.cpp PhysicalMemory.h
        FlowExampleSimulation.cpp MemoryConstants.h)
