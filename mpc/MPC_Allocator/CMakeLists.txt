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
cmake_minimum_required(VERSION 2.8)

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

####################### DEPS ############################
if (NOT DISABLE_LIBNUMA)
	find_package(Hwloc)
endif(NOT DISABLE_LIBNUMA)

if (HWLOC_FOUND)
	add_definitions(-DHAVE_LIBNUMA=yes)
else (HWLOC_FOUND)
	if(EXISTS ${CMAKE_SOURCE_DIR}/hwloc)
		Message("Hwloc not found by cmake, use the one in SOURCE_DIR/hwloc")
		add_definitions(-DHAVE_LIBNUMA=yes)
		set(HWLOC_LIBRARY ${CMAKE_SOURCE_DIR}/hwloc/lib/libhwloc.lib)
		set(HWLOC_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/hwloc/include)
		set(HWLOC_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/hwloc/include)
		set(HWLOC_FOUND yes)
		include_directories(${HWLOC_INCLUDE_DIRS})
	else()
		Message("Hwloc not found, disable NUMA")
		set(HWLOC_LIBRARY "")
	endif()
endif (HWLOC_FOUND)

add_subdirectory(src)
IF(EXISTS ${CMAKE_SOURCE_DIR}/tests)
	add_subdirectory(tests)
ENDIF(EXISTS ${CMAKE_SOURCE_DIR}/tests)

