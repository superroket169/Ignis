cmake_minimum_required(VERSION 3.21)
project(TerminalChess)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(IGNIS_CORE_SOURCES
  src/search.cpp
  src/init.cpp
  src/magicboard.cpp
  src/bitboard.cpp
  src/fen.cpp
)

add_executable(ignis ${IGNIS_CORE_SOURCES} src/main.cpp)
add_executable(ignis-uci ${IGNIS_CORE_SOURCES} src/uci.cpp)

target_compile_options(ignis PRIVATE
    -O3
    -funroll-loops
    -Wall
)

target_compile_options(ignis-uci PRIVATE
    -O3
    -funroll-loops
    -Wall
)