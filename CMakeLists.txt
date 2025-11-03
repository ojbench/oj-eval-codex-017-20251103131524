cmake_minimum_required(VERSION 3.10)
project(oj_problem_017 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(code src/main.cpp)
target_compile_options(code PRIVATE -O2 -pipe -fno-asm -fno-rtti -fno-exceptions)

