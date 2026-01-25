cmake_minimum_required(VERSION 3.21)
project(TerminalChess)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(test
  tests/test.cpp
  src/init.cpp
  src/magicboard.cpp
  src/bitboard.cpp
  src/fen.cpp
)

target_compile_options(test PRIVATE 
    -O3 
    -funroll-loops 
    -Wall
)