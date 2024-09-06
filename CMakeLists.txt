cmake_minimum_required(VERSION 3.22)

# Prevent CMake from searching the install prefix during the find_* commands
set(CMAKE_FIND_NO_INSTALL_PREFIX TRUE)

project(
		pgmpitunelib
		VERSION 0.3.0
)

set(TEST_DIR "${CMAKE_SOURCE_DIR}/test")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}")

find_package(MPI REQUIRED)
SET(BUILD_SHARED_LIBS 1)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} "include")
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} "src")

SET(MY_COMPILE_FLAGS "")

SET(MY_EXTERNAL_LIBRARY_INCLUDES "")
SET(MY_EXTERNAL_LIBRARY_LIBRARIES "")

option(OPTION_ENABLE_LOGGING   "Enable logging" off)
option(OPTION_ENABLE_DEBUGGING "Enable debugging" off)
option(OPTION_ENABLE_TESTS "Enable tests" off)
option(OPTION_ENABLE_ALGID_STORING "Enable tracing algorithmic IDs" on)

set(PATH_LANE_COLL "" CACHE STRING "Path to lane collectives")
set(PATH_CIRCULANTS "" CACHE STRING "Path to circulant collectives")
set(PATH_SCHEDULE_COLL "" CACHE STRING "Path to schedule collectives")


set(OPTION_BUFFER_ALIGNMENT "" CACHE STRING "Alignment in bytes to use for buffer allocation [default: no alignment (allocation with calloc)]")

if(OPTION_ENABLE_LOGGING)
    SET(MY_COMPILE_FLAGS "${MY_COMPILE_FLAGS} -DUSE_LOGGING")
endif()

if(OPTION_ENABLE_DEBUGGING)
    SET(MY_COMPILE_FLAGS "${MY_COMPILE_FLAGS} -g")
endif()

if(OPTION_ENABLE_ALGID_STORING)
    SET(MY_COMPILE_FLAGS "${MY_COMPILE_FLAGS} -DUSE_ALGID_STORING")
endif()
if(OPTION_BUFFER_ALIGNMENT)
    SET(MY_COMPILE_FLAGS "${MY_COMPILE_FLAGS} -DOPTION_BUFFER_ALIGNMENT=${OPTION_BUFFER_ALIGNMENT}")
endif()

set(PGMPI_COLL_FILES
src/collectives/allgather.c
src/collectives/allgather_impl.c
src/collectives/allreduce.c
src/collectives/allreduce_impl.c
src/collectives/alltoall.c
src/collectives/alltoall_impl.c
src/collectives/bcast.c
src/collectives/bcast_impl.c
src/collectives/gather.c
src/collectives/gather_impl.c
src/collectives/reduce.c
src/collectives/reduce_impl.c
src/collectives/reduce_scatter_block.c
src/collectives/reduce_scatter_block_impl.c
src/collectives/scan.c
src/collectives/scan_impl.c
src/collectives/scatter.c
src/collectives/scatter_impl.c
)

###############################################################################
find_path(CIRC_INCLUDE_DIR mpi_circulants.h PATHS ${PATH_LANE_COLL}/include)
find_library(CIRC_LIBRARIES NAMES mpicirculants PATHS ${PATH_LANE_COLL}/lib)

if(NOT CIRC_INCLUDE_DIR)
	message(STATUS "mpi_circulants.h not found. Please set PATH_CIRCULANTS to enable this library.")
endif()

if(NOT CIRC_LIBRARIES)
	message(STATUS "Library 'mpicirculants' not found. Please set PATH_CIRCULANTS to enable this library.")
endif()

if(CIRC_INCLUDE_DIR AND CIRC_LIBRARIES)
	message(STATUS "Extension: mpi-circulant collectives enabled: ${CIRC_LIBRARIES}")
	SET(MY_COMPILE_FLAGS "${MY_COMPILE_FLAGS} -DHAVE_CIRCULANTS")
	SET(HAVE_LANE_COLL 1)

	SET(MY_EXTERNAL_LIBRARY_INCLUDES ${MY_EXTERNAL_LIBRARY_INCLUDES} ${CIRC_INCLUDE_DIR})
	SET(MY_EXTERNAL_LIBRARY_LIBRARIES ${MY_EXTERNAL_LIBRARY_LIBRARIES} ${CIRC_LIBRARIES})
endif()

###############################################################################
find_path(LANE_COLL_INCLUDE_DIR tuw_lanecoll.h PATHS ${PATH_LANE_COLL}/include)
find_library(LANE_COLL_LIBRARIES NAMES lanec PATHS ${PATH_LANE_COLL}/lib)

if(NOT LANE_COLL_INCLUDE_DIR)
	message(STATUS "tuw_lanecoll.h not found. Please set PATH_LANE_COLL to enable this library.")
endif()

if(NOT LANE_COLL_LIBRARIES)
	message(STATUS "Library 'lanec' not found. Please set PATH_LANE_COLL to enable this library.")
endif()

if(LANE_COLL_INCLUDE_DIR AND LANE_COLL_LIBRARIES)
	message(STATUS "Extension: lane collectives enabled: ${LANE_COLL_LIBRARIES}")
	SET(MY_COMPILE_FLAGS "${MY_COMPILE_FLAGS} -DHAVE_LANE_COLL")
	SET(HAVE_LANE_COLL 1)

	SET(MY_EXTERNAL_LIBRARY_INCLUDES ${MY_EXTERNAL_LIBRARY_INCLUDES} ${LANE_COLL_INCLUDE_DIR})
	SET(MY_EXTERNAL_LIBRARY_LIBRARIES ${MY_EXTERNAL_LIBRARY_LIBRARIES} ${LANE_COLL_LIBRARIES})
endif()

###############################################################################
find_path(SCHEDULE_COLL_INCLUDE_DIR tuw_schedule_collectives.h PATHS ${PATH_SCHEDULE_COLL}/include)
find_library(SCHEDULE_COLL_LIBRARIES NAMES msc PATHS ${PATH_SCHEDULE_COLL}/lib)

if(NOT SCHEDULE_COLL_INCLUDE_DIR)
	message(STATUS "tuw_schedule_collectives.h not found. Please set PATH_SCHEDULE_COLL to enable this library.")
endif()

if(NOT SCHEDULE_COLL_LIBRARIES)
	message(STATUS "Library 'msc' not found. Please set PATH_SCHEDULE_COLL to enable this library.")
endif()

if(SCHEDULE_COLL_INCLUDE_DIR AND SCHEDULE_COLL_LIBRARIES)
	message(STATUS "Extension: schedule collectives enabled: ${SCHEDULE_COLL_LIBRARIES}")
	SET(MY_COMPILE_FLAGS "${MY_COMPILE_FLAGS} -DHAVE_SCHEDULE_COLL")
	SET(HAVE_SCHEDULE_COLL 1)

	SET(MY_EXTERNAL_LIBRARY_INCLUDES ${MY_EXTERNAL_LIBRARY_INCLUDES} ${SCHEDULE_COLL_INCLUDE_DIR})
	SET(MY_EXTERNAL_LIBRARY_LIBRARIES ${MY_EXTERNAL_LIBRARY_LIBRARIES} ${SCHEDULE_COLL_LIBRARIES})
endif()
###############################################################################

set(PGMPI_LIB_FILES
src/pgmpi_algid_store.c
src/pgmpi_tune.c
src/bufmanager/pgmpi_buf.c
src/collectives/collective_modules.c
src/config/pgmpi_config_reader.c
src/config/pgmpi_config.c
src/log/zf_log.c
src/map/hashtable_int.c
src/util/keyvalue_store.c
src/util/pgmpi_parse_cli.c
src/pgmpi_mpihook.c
)

add_library(pgmpicli
${PGMPI_COLL_FILES}
${PGMPI_LIB_FILES}
src/pgmpi_mpihook_cli.c
)

add_library(pgmpituned
${PGMPI_COLL_FILES}
${PGMPI_LIB_FILES}
src/tuning/pgmpi_function_replacer.c
src/tuning/pgmpi_profile.c
src/tuning/pgmpi_profile_reader.c
src/pgmpi_mpihook_tuned.c
)

#set(PGMPI_HEADERS
#		include/pgmpi_tune.h
#		src/collectives/collective_modules.h
#)
SET_TARGET_PROPERTIES(pgmpicli PROPERTIES COMPILE_FLAGS "${MY_COMPILE_FLAGS} -DUSE_PMPI")
SET_TARGET_PROPERTIES(pgmpicli PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
#SET_TARGET_PROPERTIES(pgmpicli PROPERTIES PUBLIC_HEADER "${PGMPI_HEADERS}")
target_include_directories(pgmpicli PRIVATE ${MY_EXTERNAL_LIBRARY_INCLUDES})
target_link_libraries(pgmpicli ${MY_EXTERNAL_LIBRARY_LIBRARIES} MPI::MPI_C)

#install(TARGETS pgmpicli DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
INSTALL(TARGETS pgmpicli
		LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
#		PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}/include
)

SET_TARGET_PROPERTIES(pgmpituned PROPERTIES COMPILE_FLAGS "${MY_COMPILE_FLAGS}")
SET_TARGET_PROPERTIES(pgmpituned PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
TARGET_LINK_LIBRARIES(pgmpituned MPI::MPI_C)

add_executable(pgmpi_info
src/pgmpi_info.c
)
TARGET_LINK_LIBRARIES(pgmpi_info pgmpicli MPI::MPI_C)

if(OPTION_ENABLE_TESTS)
    add_executable(test1
		${TEST_DIR}/libtest/test1.c
	)
	TARGET_LINK_LIBRARIES(test1 pgmpicli MPI::MPI_C)


	add_executable(test2
		${TEST_DIR}/perfmodels/test_allgather.c
	)
	TARGET_LINK_LIBRARIES(test2 pgmpituned MPI::MPI_C)
	
	add_executable(maptest1
		${TEST_DIR}/maptest/maptest1.c
	)
	TARGET_LINK_LIBRARIES(maptest1 pgmpituned MPI::MPI_C)

	add_executable(testcoll
		${TEST_DIR}/colltest/tests.c
		${TEST_DIR}/colltest/test_collectives.c
	)
	TARGET_LINK_LIBRARIES(testcoll pgmpicli MPI::MPI_C)

endif()




set(PGMPITUNE_NAME ${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION})
set(DIST_ARCHIVE "${PGMPITUNE_NAME}-dist")

set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PGMPITUNE_NAME}-src")
set(CPACK_SOURCE_GENERATOR "TBZ2")
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


