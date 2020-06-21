cmake_minimum_required(VERSION 3.16)

project(MyPenet)

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
set(CMAKE_CXX_FLAGS_RELESE "$ENV{CXXFLAGS} -03 -Wall")

aux_source_directory(./penet DIR_SRCS)

add_executable(MyPenet ${DIR_SRCS})
