cmake_minimum_required(VERSION 3.22)


set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")
   
project(pgmpitunelib)

set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(TEST_DIR "${CMAKE_SOURCE_DIR}/test")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}")

set(INCLUDE_PLATFORM_CONFIG_FILE "${CMAKE_SOURCE_DIR}/platform_files/default.cmake" 
    CACHE STRING "Configure project to use a specific platform file")
include(${INCLUDE_PLATFORM_CONFIG_FILE})

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} "include")
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} "src")

SET(MY_COMPILE_FLAGS "")

option(OPTION_ENABLE_LOGGING   "Enable logging" off)
option(OPTION_ENABLE_DEBUGGING "Enable debugging" off)
option(OPTION_ENABLE_TESTS "Enable tests" off)
option(OPTION_ENABLE_ALGID_STORING "Enable tracing algorithmic IDs" on)
option(OPTION_ENABLE_LANE_COLL "Enable lane collectives if available" on)
option(OPTION_ENABLE_CIRCULANTS "Enable circulant collectives if available" off)


set(OPTION_BUFFER_ALIGNMENT "" CACHE STRING "Alignment to use for buffer allocation [default: no alignment (allocation with calloc)]")

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
${SRC_DIR}/collectives/allgather.c
${SRC_DIR}/collectives/allgather_impl.c
${SRC_DIR}/collectives/allreduce.c
${SRC_DIR}/collectives/allreduce_impl.c
${SRC_DIR}/collectives/alltoall.c
${SRC_DIR}/collectives/alltoall_impl.c
${SRC_DIR}/collectives/bcast.c
${SRC_DIR}/collectives/bcast_impl.c
${SRC_DIR}/collectives/gather.c
${SRC_DIR}/collectives/gather_impl.c
${SRC_DIR}/collectives/reduce.c
${SRC_DIR}/collectives/reduce_impl.c
${SRC_DIR}/collectives/reduce_scatter_block.c
${SRC_DIR}/collectives/reduce_scatter_block_impl.c
${SRC_DIR}/collectives/scan.c
${SRC_DIR}/collectives/scan_impl.c
${SRC_DIR}/collectives/scatter.c
${SRC_DIR}/collectives/scatter_impl.c
)

if(OPTION_ENABLE_LANE_COLL)
	message(STATUS "Extension: Checking for lane collectives")
	SET(HAVE_LANE_COLL 0) # default
	if(EXISTS "${CMAKE_SOURCE_DIR}/submodules/mpi-lane-collectives/src/tuw_lanecoll.h")
		SET(MY_COMPILE_FLAGS "${MY_COMPILE_FLAGS} -DHAVE_LANE_COLL")
		SET(LANE_COLL_ROOT ${CMAKE_SOURCE_DIR}/submodules/mpi-lane-collectives)
		SET(HAVE_LANE_COLL 1)
	endif()
else()
	message(STATUS "Extension: hint you can enable lane collectives by using -DOPTION_ENABLE_LANE_COLL=ON")
endif()

if( HAVE_LANE_COLL )
	message(STATUS "Extension: Lane collectives available and enabled")

	set(PGMPI_COLL_FILES
			${PGMPI_COLL_FILES}
			${LANE_COLL_ROOT}/src/tuw_lanecoll.c
			${LANE_COLL_ROOT}/src/tuw_lane_bcast.c
	)

	INCLUDE_DIRECTORIES("${LANE_COLL_ROOT}/src")
else()
	message(STATUS "Extension: Lane collectives unavailable")
endif()

if(OPTION_ENABLE_CIRCULANTS)
	message(STATUS "Extension: Checking for circulants")
	SET(HAVE_CIRCULANTS 0) # default
	if(EXISTS "${CMAKE_SOURCE_DIR}/submodules/mpi_circulants/src/mpi_circulants.h")
		SET(MY_COMPILE_FLAGS "${MY_COMPILE_FLAGS} -DHAVE_CIRCULANTS")
		SET(CIRCULANTS_ROOT ${CMAKE_SOURCE_DIR}/submodules/mpi_circulants)
		SET(HAVE_CIRCULANTS 1)
	endif()
else()
	message(STATUS "Extension: hint you can enable circulants by using -DOPTION_ENABLE_CIRCULANTS=ON")
endif()

if( HAVE_CIRCULANTS )
	message(STATUS "Extension: Circulant collectives available and enabled")

	set(PGMPI_COLL_FILES
			${PGMPI_COLL_FILES}
			${CIRCULANTS_ROOT}/src/mpi_circulants.c)

	INCLUDE_DIRECTORIES("${CIRCULANTS_ROOT}/src")
else()
	message(STATUS "Extension: Circulant collectives unavailable")
endif()


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

SET_TARGET_PROPERTIES(pgmpicli PROPERTIES COMPILE_FLAGS "${MY_COMPILE_FLAGS} -DUSE_PMPI")
SET_TARGET_PROPERTIES(pgmpicli PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")

set(PGMPI_HEADERS
		include/pgmpi_tune.h
		src/collectives/collective_modules.h
)
SET_TARGET_PROPERTIES(pgmpicli PROPERTIES PUBLIC_HEADER "${PGMPI_HEADERS}")

#install(TARGETS pgmpicli DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
INSTALL(TARGETS pgmpicli
		LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
		PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}/include
		)

SET_TARGET_PROPERTIES(pgmpituned PROPERTIES COMPILE_FLAGS "${MY_COMPILE_FLAGS}")
SET_TARGET_PROPERTIES(pgmpituned PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")


add_executable(pgmpi_info
${SRC_DIR}/pgmpi_info.c
)
TARGET_LINK_LIBRARIES(pgmpi_info pgmpicli)

if(OPTION_ENABLE_TESTS)
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
endif()



set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "2")
set(CPACK_PACKAGE_VERSION_PATCH "0")
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


