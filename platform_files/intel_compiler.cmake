# Intel compiler
#find_package(MPI REQUIRED)

SET(CMAKE_C_COMPILER mpiicc)
SET(CMAKE_CXX_COMPILER mpiicpc)
SET(CMAKE_C_FLAGS "-O3 -Wall -std=c99 -D_POSIX_C_SOURCE=200809L") 

SET(BUILD_SHARED_LIBS 1)

message(STATUS "Using the Intel compiler: ${CMAKE_C_COMPILER}")


