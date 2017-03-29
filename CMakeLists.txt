cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")
   
project(pgmpitunelib)

set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(TEST_DIR "${CMAKE_SOURCE_DIR}/test")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}")

set(INCLUDE_PLATFORM_CONFIG_FILE "${CMAKE_SOURCE_DIR}/platform_files/default.cmake" 
    CACHE STRING "Configure project to use a specific platform file")
include(${INCLUDE_PLATFORM_CONFIG_FILE})

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} "include")
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} "src")


SET(MY_COMPILE_FLAGS "")

option(OPTION_ENABLE_LOGGING   "Enable logging" off)
option(OPTION_ENABLE_DEBUGGING "Enable debugging" off)
option(OPTION_ENABLE_ALGID_STORING "Enable tracing algorithmic IDs" on)
option(OPTION_ENABLE_MEMALIGNED_BUFFERS "Allocate buffers using posix_memalign [default: calloc]" off)
set(OPTION_BUFFER_ALIGNMENT 64 CACHE STRING "Alignment to use for buffer allocation")

if(OPTION_ENABLE_LOGGING)
    SET(MY_COMPILE_FLAGS "${MY_COMPILE_FLAGS} -DUSE_LOGGING")
endif()


if(OPTION_ENABLE_DEBUGGING)
    SET(MY_COMPILE_FLAGS "${MY_COMPILE_FLAGS} -g")
endif()

if(OPTION_ENABLE_ALGID_STORING)
    SET(MY_COMPILE_FLAGS "${MY_COMPILE_FLAGS} -DUSE_ALGID_STORING")
endif()

if(OPTION_ENABLE_MEMALIGNED_BUFFERS)
    SET(MY_COMPILE_FLAGS "${MY_COMPILE_FLAGS} -DOPTION_ENABLE_MEMALIGNED_BUFFERS")
endif()

if(OPTION_BUFFER_ALIGNMENT)
    SET(MY_COMPILE_FLAGS "${MY_COMPILE_FLAGS} -DOPTION_BUFFER_ALIGNMENT=${OPTION_BUFFER_ALIGNMENT}")
endif()


set(PGMPI_COLL_FILES
${SRC_DIR}/collectives/allgather.c
${SRC_DIR}/collectives/allreduce.c
${SRC_DIR}/collectives/alltoall.c
${SRC_DIR}/collectives/bcast.c
${SRC_DIR}/collectives/gather.c
${SRC_DIR}/collectives/reduce.c
${SRC_DIR}/collectives/reduce_scatter_block.c
${SRC_DIR}/collectives/scan.c
${SRC_DIR}/collectives/scatter.c
)

set(PGMPI_LIB_FILES
${SRC_DIR}/pgmpi_algid_store.c
${SRC_DIR}/pgmpi_tune.c
${SRC_DIR}/bufmanager/pgmpi_buf.c
${SRC_DIR}/collectives/collective_modules.c
${SRC_DIR}/config/pgmpi_config_reader.c
${SRC_DIR}/config/pgmpi_config.c
${SRC_DIR}/log/zf_log.c
${SRC_DIR}/map/hashtable_int.c
${SRC_DIR}/util/keyvalue_store.c
${SRC_DIR}/util/pgmpi_parse_cli.c
${SRC_DIR}/pgmpi_mpihook.c
)

add_library(pgmpicli
${PGMPI_COLL_FILES}
${PGMPI_LIB_FILES}
${SRC_DIR}/pgmpi_mpihook_cli.c
)

add_library(pgmpituned
${PGMPI_COLL_FILES}
${PGMPI_LIB_FILES}
${SRC_DIR}/tuning/pgmpi_function_replacer.c
${SRC_DIR}/tuning/pgmpi_profile.c
${SRC_DIR}/tuning/pgmpi_profile_reader.c
${SRC_DIR}/pgmpi_mpihook_tuned.c
)


SET_TARGET_PROPERTIES(pgmpicli   PROPERTIES COMPILE_FLAGS "${MY_COMPILE_FLAGS} -DUSE_PMPI")
SET_TARGET_PROPERTIES(pgmpituned PROPERTIES COMPILE_FLAGS "${MY_COMPILE_FLAGS}")


add_executable(pgmpi_info
${SRC_DIR}/pgmpi_info.c
)
TARGET_LINK_LIBRARIES(pgmpi_info pgmpicli)

add_executable(test1
${TEST_DIR}/libtest/test1.c
)
TARGET_LINK_LIBRARIES(test1 pgmpicli)

add_executable(test2
${TEST_DIR}/libtest/test1.c
)
TARGET_LINK_LIBRARIES(test2 pgmpituned)

add_executable(maptest1
${TEST_DIR}/maptest/maptest1.c
)
TARGET_LINK_LIBRARIES(maptest1 pgmpituned)



add_executable(testcoll
${TEST_DIR}/colltest/tests.c
${TEST_DIR}/colltest/test_collectives.c
)
TARGET_LINK_LIBRARIES(testcoll pgmpicli)



set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "1")
set(CPACK_SOURCE_GENERATOR "TBZ2")

set(PGMPITUNE_NAME ${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PGMPITUNE_NAME}-src")
set(DIST_ARCHIVE "${PGMPITUNE_NAME}-dist")

SET(CPACK_SOURCE_IGNORE_FILES
"/CMakeFiles/"
"/_CPack_Packages/"
"/bin/"
"/lib/"
"/Makefile"
"\\\\.tar\\\\.bz2$"
"/\\\\.git/"
"/\\\\.settings/"
"/\\\\..*"
"CMakeCache.txt"
"cmake_install\\\\.cmake"
"CPackConfig\\\\.cmake"
"CPackSourceConfig\\\\.cmake"
"sources\\\\.org"
"${CPACK_SOURCE_IGNORE_FILES}"
)

INCLUDE(CPack)


