cmake_minimum_required(VERSION 3.22)

# Prevent CMake from searching the install prefix during the find_* commands
set(CMAKE_FIND_NO_INSTALL_PREFIX TRUE)

project(
		pgmpitunelib
		VERSION 0.3.0
		LANGUAGES C
)

include(FetchContent)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(TEST_DIR "${CMAKE_SOURCE_DIR}/test")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
#set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}")

list(APPEND CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

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

#set(PATH_LANE_COLL "" CACHE STRING "Path to lane collectives")
#set(PATH_CIRCULANTS "" CACHE STRING "Path to circulant collectives")
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
#find_path(CIRC_INCLUDE_DIR mpi_circulants.h PATHS ${PATH_LANE_COLL}/include)
#find_library(CIRC_LIBRARIES NAMES mpicirculants PATHS ${PATH_LANE_COLL}/lib)
#
#if(NOT CIRC_INCLUDE_DIR)
#	message(STATUS "mpi_circulants.h not found. Please set PATH_CIRCULANTS to enable this library.")
#endif()
#
#if(NOT CIRC_LIBRARIES)
#	message(STATUS "Library 'mpicirculants' not found. Please set PATH_CIRCULANTS to enable this library.")
#endif()
#
#if(CIRC_INCLUDE_DIR AND CIRC_LIBRARIES)
#	message(STATUS "Extension: mpi-circulant collectives enabled: ${CIRC_LIBRARIES}")
#	SET(MY_COMPILE_FLAGS "${MY_COMPILE_FLAGS} -DHAVE_CIRCULANTS")
#	SET(HAVE_LANE_COLL 1)
#
#	SET(MY_EXTERNAL_LIBRARY_INCLUDES ${MY_EXTERNAL_LIBRARY_INCLUDES} ${CIRC_INCLUDE_DIR})
#	SET(MY_EXTERNAL_LIBRARY_LIBRARIES ${MY_EXTERNAL_LIBRARY_LIBRARIES} ${CIRC_LIBRARIES})
#endif()

###############################################################################

set(PGMPI_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR})
set(PGMPI_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR})
set(PGMPI_INSTALL_BINDIR ${CMAKE_INSTALL_BINDIR})
set(PGMPI_INSTALL_LIBEXECDIR ${CMAKE_INSTALL_LIBEXECDIR})
set(PGMPI_INSTALL_DATADIR ${CMAKE_INSTALL_DATADIR})
set(PGMPI_INSTALL_CMAKEDIR ${CMAKE_INSTALL_DATADIR}/cmake/pgmpi)

STRING(COMPARE EQUAL ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} PGMPI_BUILD_INPLACE)
if(NOT PGMPI_BUILD_INPLACE)
	include_directories(BEFORE "${CMAKE_CURRENT_SOURCE_DIR}")
	include_directories(BEFORE "${CMAKE_CURRENT_SOURCE_DIR}/include")
	include_directories(BEFORE "${CMAKE_CURRENT_SOURCE_DIR}/src")
endif(NOT PGMPI_BUILD_INPLACE)
include_directories(BEFORE "${CMAKE_CURRENT_BINARY_DIR}")
include_directories(BEFORE "${CMAKE_CURRENT_BINARY_DIR}/include")
include_directories(BEFORE "${CMAKE_CURRENT_BINARY_DIR}/src")


FetchContent_Declare(
		MPI_LANE_COLLECTIVES
		GIT_REPOSITORY https://github.com/parlab-tuwien/mpi-lane-collectives.git
		GIT_TAG main
)
SET(MY_COMPILE_FLAGS "${MY_COMPILE_FLAGS} -DHAVE_LANE_COLL")

FetchContent_MakeAvailable(MPI_LANE_COLLECTIVES)

FetchContent_Declare(
		MPI_CIRCULANTS
		GIT_REPOSITORY git@github.com:parlab-tuwien/mpi-circulant-collectives.git
		GIT_TAG main
)
FetchContent_MakeAvailable(MPI_CIRCULANTS)

SET(MY_COMPILE_FLAGS "${MY_COMPILE_FLAGS} -DHAVE_CIRCULANTS")

#find_path(LANE_COLL_INCLUDE_DIR tuw_lanecoll.h PATHS ${PATH_LANE_COLL}/include)
#find_library(LANE_COLL_LIBRARIES NAMES lanec PATHS ${PATH_LANE_COLL}/lib)
#
#if(NOT LANE_COLL_INCLUDE_DIR)
#	message(STATUS "tuw_lanecoll.h not found. Please set PATH_LANE_COLL to enable this library.")
#endif()
#
#if(NOT LANE_COLL_LIBRARIES)
#	message(STATUS "Library 'lanec' not found. Please set PATH_LANE_COLL to enable this library.")
#endif()

#if(LANE_COLL_INCLUDE_DIR AND LANE_COLL_LIBRARIES)
#	message(STATUS "Extension: lane collectives enabled: ${LANE_COLL_LIBRARIES}")
#	SET(MY_COMPILE_FLAGS "${MY_COMPILE_FLAGS} -DHAVE_LANE_COLL")
#	SET(HAVE_LANE_COLL 1)
#
#	SET(MY_EXTERNAL_LIBRARY_INCLUDES ${MY_EXTERNAL_LIBRARY_INCLUDES} ${LANE_COLL_INCLUDE_DIR})
#	SET(MY_EXTERNAL_LIBRARY_LIBRARIES ${MY_EXTERNAL_LIBRARY_LIBRARIES} ${LANE_COLL_LIBRARIES})
#endif()

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
SET_TARGET_PROPERTIES(pgmpicli PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set_target_properties(pgmpicli PROPERTIES PUBLIC_HEADER include/pgmpi_tune.h)

#SET_TARGET_PROPERTIES(pgmpicli PROPERTIES PUBLIC_HEADER "${PGMPI_HEADERS}")
#target_include_directories(pgmpicli
#		PUBLIC
#		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>  # When building the project
#		$<INSTALL_INTERFACE:include>                            # When installing the project
#)

target_include_directories(pgmpicli
		INTERFACE
		$<INSTALL_INTERFACE:${PGMPI_INSTALL_INCLUDEDIR}>
		$<$<NOT:${PGMPI_BUILD_INPLACE}>:
		$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
		$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>>
		$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)


target_link_libraries(pgmpicli ${MY_EXTERNAL_LIBRARY_LIBRARIES} mpicirculants lanec MPI::MPI_C)


install(TARGETS pgmpicli
		EXPORT pgmpicli-targets
		LIBRARY DESTINATION ${PGMPI_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${PGMPI_INSTALL_LIBDIR}
		RUNTIME DESTINATION ${PGMPI_INSTALL_BINDIR}
		PUBLIC_HEADER DESTINATION ${PGMPI_INSTALL_INCLUDEDIR})

install(EXPORT pgmpicli-targets # rules to export the targets built here
		FILE PGMPITargets.cmake
		NAMESPACE PGMPI::
		DESTINATION ${PGMPI_INSTALL_CMAKEDIR})

#install(TARGETS pgmpicli DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
#INSTALL(TARGETS pgmpicli
#		LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
##		PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}/include
#)

SET_TARGET_PROPERTIES(pgmpituned PROPERTIES COMPILE_FLAGS "${MY_COMPILE_FLAGS}")
SET_TARGET_PROPERTIES(pgmpituned PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
target_include_directories(pgmpituned PRIVATE ${MY_EXTERNAL_LIBRARY_INCLUDES})
target_link_libraries(pgmpituned ${MY_EXTERNAL_LIBRARY_LIBRARIES} mpicirculants lanec MPI::MPI_C)


add_executable(pgmpi_info
src/pgmpi_info.c
)
TARGET_LINK_LIBRARIES(pgmpi_info pgmpicli MPI::MPI_C)

install(TARGETS pgmpi_info
		RUNTIME DESTINATION bin  # For executables
)

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



