cmake_minimum_required(VERSION 3.27)
project(DataFlow)

set(CMAKE_CXX_STANDARD 20)

add_executable(DataFlow
        main.cpp
        ast.cpp
)
