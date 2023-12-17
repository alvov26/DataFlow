cmake_minimum_required(VERSION 3.27)
project(DataFlow)

set(CMAKE_CXX_STANDARD 20)

add_executable(DataFlow
        src/main.cpp
        src/ast.cpp
        src/tokens.cpp
        src/parser.cpp
        src/analysis.cpp
)

target_include_directories(DataFlow
        PUBLIC ${PROJECT_SOURCE_DIR}/include
)
