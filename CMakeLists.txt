cmake_minimum_required(VERSION 3.8)
project(FPTree)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES main.c main.h test_fptree.c)
add_executable(FPTree ${SOURCE_FILES})