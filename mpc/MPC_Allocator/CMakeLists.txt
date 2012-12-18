############################# MPC License ##############################
# Wed Nov 19 15:19:19 CET 2008                                         #
# Copyright or (C) or Copr. Commissariat a l'Energie Atomique          #
#                                                                      #
# IDDN.FR.001.230040.000.S.P.2007.000.10000                            #
# This file is part of the MPC Runtime.                                #
#                                                                      #
# This software is governed by the CeCILL-C license under French law   #
# and abiding by the rules of distribution of free software.  You can  #
# use, modify and/ or redistribute the software under the terms of     #
# the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     #
# following URL http://www.cecill.info.                                #
#                                                                      #
# The fact that you are presently reading this means that you have     #
# had knowledge of the CeCILL-C license and that you accept its        #
# terms.                                                               #
#                                                                      #
# Authors:                                                             #
#   - ADAM Julien julien.adam@cea.fr                                   #
#   - VALAT Sebastien sebastien.valat@cea.fr                           #
#                                                                      #
########################################################################

PROJECT(mpc_allocator)
cmake_minimum_required(VERSION 2.6)

enable_testing()

# add_definitions(-DSCTK_ALLOC_SPY)
# add_definitions(-DENABLE_TRACE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/)

if(WIN32)
	SET(EXECUTOR_PATH ${WINE_PATH})
endif(WIN32)

####################  CMAKE MODULES #####################
INCLUDE (CheckIncludeFiles)

###################### CHECK HEADER #####################
CHECK_INCLUDE_FILES(stdbool.h HAVE_STDBOOL_H)
if(HAVE_STDBOOL_H)
	add_definitions(-DHAVE_STDBOOL_H)
endif(HAVE_STDBOOL_H)
add_definitions(-DENABLE_ALLOC_HOOKS)

################## VALGRIND SUPPORT #####################
CHECK_INCLUDE_FILES(valgrind/memcheck.h HAVE_MEMCHECK_H)
if(HAVE_MEMCHECK_H AND NOT DISABLE_VALGRIND)
    add_definitions(-DHAVE_MEMCHECK_H)
    message(STATUS "Support memcheck : yes")
else(HAVE_MEMCHECK_H AND NOT DISABLE_VALGRIND)
    message(STATUS "Support memcheck : no")
endif(HAVE_MEMCHECK_H AND NOT DISABLE_VALGRIND)

####################### DEPS ############################
if (NOT DISABLE_LIBNUMA)
	find_package(Hwloc)
endif(NOT DISABLE_LIBNUMA)

if (HWLOC_FOUND)
	add_definitions(-DHAVE_HWLOC=yes)
	include_directories(${HWLOC_INCLUDE_DIRS})
	message(STATUS "Support NUMA : yes")
else (HWLOC_FOUND)
	set(hwloc_inner_path ${CMAKE_SOURCE_DIR}/hwloc)
	#try some other dirs
	if(EXISTS ${CMAKE_SOURCE_DIR}/hwloc-win64-build-1.4.2/)
		set(hwloc_inner_path ${CMAKE_SOURCE_DIR}/hwloc-win64-build-1.4.2)
	endif()
	#real check
	if(EXISTS ${hwloc_inner_path})
		Message("Hwloc not found by cmake, use the one in ${hwloc_inner_path}")
		add_definitions(-DHAVE_HWLOC=yes)
		set(HWLOC_LIBRARY ${hwloc_inner_path}/lib/libhwloc.lib)
		set(HWLOC_INCLUDE_DIRS ${hwloc_inner_path}/include)
		set(HWLOC_INCLUDE_DIR ${hwloc_inner_path}/include)
		set(HWLOC_FOUND yes)
		include_directories(${HWLOC_INCLUDE_DIRS})
		message(STATUS "Support NUMA : yes")
	else()
		Message(WARNING "Hwloc not found, disable NUMA")
		message(STATUS "Support NUMA : no")
		set(HWLOC_LIBRARY "")
	endif()
endif (HWLOC_FOUND)

find_package(OpenPA REQUIRED)
if (OPENPA_FOUND)
    include_directories(${OPENPA_INCLUDE_DIRS})
    message(STATUS "Use OpenPA : yes")
else (OPENPA_FOUND)
    message(STATUS "Use OpenPA : no")
    message(FATAL_ERROR "Cannot find OpenPA, please provide the path with -DOPENPA_PREFIX=...")
endif (OPENPA_FOUND)

add_subdirectory(src)
IF(EXISTS ${CMAKE_SOURCE_DIR}/tests)
	add_subdirectory(tests)
ENDIF(EXISTS ${CMAKE_SOURCE_DIR}/tests)

