PROJECT(mpc_allocator_v1)
cmake_minimum_required(VERSION 2.8)

enable_testing()

# add_definitions(-DSCTK_ALLOC_SPY)
# add_definitions(-DENABLE_TRACE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/)

if (WIN32)
	SET(EXECUTOR_PATH /home/julia/usr/wine/bin/wine64)
	# the name of the target operating system
	SET(CMAKE_SYSTEM_NAME Windows)

	# which compilers to use for C and C++
	SET(CMAKE_C_COMPILER /home/julia/usr/minGW/bin/x86_64-w64-mingw32-gcc)
	SET(CMAKE_CXX_COMPILER /home/julia/usr/minGW/bin/x86_64-w64-mingw32-g++)
	SET(CMAKE_RC_COMPILER /home/julia/usr/minGW/bin/x86_64-w64-mingw32-windres)

	# here is the target environment located
	SET(CMAKE_FIND_ROOT_PATH   /home/julia/usr/targets/ )

	# adjust the default behaviour of the FIND_XXX() commands:
	# search headers and libraries in the target environment, search 
	# programs in the host environment
	set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
	set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
	set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
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
add_subdirectory(tests)
