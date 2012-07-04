PROJECT(mpc_allocator_v1)
cmake_minimum_required(VERSION 2.8)

enable_testing()

# add_definitions(-DSCTK_ALLOC_SPY)
# add_definitions(-DENABLE_TRACE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/)

if(WIN32)
	SET(EXECUTOR_PATH ${WINE_PATH})
endif(WIN32)

####################### DEPS ############################
if (NOT DISABLE_LIBNUMA)
	find_package(Hwloc)
endif(NOT DISABLE_LIBNUMA)

if (HWLOC_FOUND)
	add_definitions(-DHAVE_LIBNUMA=yes)
else()
	set(HWLOC_LIBRARY "")
endif (HWLOC_FOUND)

add_subdirectory(src)
IF(EXISTS tests)
	add_subdirectory(tests)
ENDIF(EXISTS tests)

