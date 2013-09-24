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

################### PROJECT SETUP #######################
PROJECT(mpc_allocator)
cmake_minimum_required(VERSION 2.6)

enable_testing()

#For debug
# add_definitions(-DSCTK_ALLOC_SPY)
# add_definitions(-DENABLE_TRACE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/)

#Old testing througt mingw+wine, but now the windows version built with VisualStudio.
#if(WIN32)
#	SET(EXECUTOR_PATH ${WINE_PATH})
#endif(WIN32)

####################  CMAKE MODULES #####################
INCLUDE (CheckIncludeFiles)

######################  SET RPATHS ######################
if(ENABLE_RPATH)
	set(CMAKE_SKIP_BUILD_RPATH FALSE)
	set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
	set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}")
	set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif(ENABLE_RPATH)

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
# Search for hwloc for NUMA support
if (NOT DISABLE_LIBNUMA)
	find_package(Hwloc)
endif(NOT DISABLE_LIBNUMA)

#On windows, we help build by placing hwloc in hwloc/ sudirectory of allocator sources.
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

#On windows we help by copying openpa/ in allocator sources
if(WIN32)
	set(OPENPA_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/openpa/src")
endif(WIN32)

#Search for OpenPA
find_package(OpenPA REQUIRED)
if (OPENPA_FOUND)
    include_directories(${OPENPA_INCLUDE_DIRS})
    message(STATUS "Use OpenPA : yes")
else (OPENPA_FOUND)
    message(STATUS "Use OpenPA : no")
    message(FATAL_ERROR "Cannot find OpenPA, please provide the path with -DOPENPA_PREFIX=...")
endif (OPENPA_FOUND)

#Compile unit tests if not disabled
add_subdirectory(src)
IF(EXISTS ${CMAKE_SOURCE_DIR}/tests AND NOT DISABLE_UNIT_TESTS)
	add_subdirectory(tests)
ENDIF(EXISTS ${CMAKE_SOURCE_DIR}/tests AND NOT DISABLE_UNIT_TESTS)

#Files to install
INSTALL(FILES include/mpcalloc.h DESTINATION include/)

######################################################
#Display info on current configuration
macro (print_variable_status var_name)
	if (${var_name})
		message("${var_name} : yes")
	else (${var_value})
		message("${var_name} : no")
	endif (${var_name})
endmacro (print_variable_status)

######################################################
#Summary
message("============== SUMMARY ==================")
message("CMAKE_BUILD_TYPE : ${CMAKE_BUILD_TYPE}")
print_variable_status("ENABLE_GCC_COVERAGE")
print_variable_status("ENABLE_RPATH")
print_variable_status("DISABLE_UNIT_TESTS")
print_variable_status("HWLOC_FOUND")
print_variable_status("OPENPA_FOUND")
message("=========================================")
