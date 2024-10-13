cmake_minimum_required(VERSION 3.16)
project(niffler)

set(CMAKE_CXX_STANDARD 20)

add_executable(
        niffler
        Utility/pffft.c
        benchmark.cpp
        io_helper.cpp
        main.cpp
)

file(GLOB DATA "data_source/*")
file(COPY ${DATA} DESTINATION data_source)