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

########################################################################
#Some deps
include(CheckIncludeFiles)

########################################################################
#Use rpath to find deps on exec
MACRO(mpc_enable_rpath)
	#setup the global entry with doc
	set(ENABLE_RPATH no CACHE BOOL "Force to use rpath for all libs when installing.")

	#do it if enabled
	if(ENABLE_RPATH)
		set(CMAKE_SKIP_BUILD_RPATH FALSE)
		set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
		set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}")
		set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
	endif(ENABLE_RPATH)
ENDMACRO(mpc_enable_rpath)

########################################################################
#check for stdbool.h
MACRO(mpc_check_stdbool)
	check_include_files(stdbool.h HAVE_STDBOOL_H)
	if(HAVE_STDBOOL_H)
		add_definitions(-DHAVE_STDBOOL_H)
	endif(HAVE_STDBOOL_H)
ENDMACRO(mpc_check_stdbool)

########################################################################
#Check for valgrind
MACRO(mpc_check_valgrind)
	#setup the global cache entry
	set(DISABLE_VALGRIND no CACHE BOOL "Disable usage of valgrind support if present.")

	#check header
	check_include_files(valgrind/memcheck.h HAVE_MEMCHECK_H)

	#enable if presend an not disabled
	if(HAVE_MEMCHECK_H AND NOT DISABLE_VALGRIND)
		add_definitions(-DHAVE_MEMCHECK_H)
		message(STATUS "Support memcheck : yes")
	else(HAVE_MEMCHECK_H AND NOT DISABLE_VALGRIND)
		message(STATUS "Support memcheck : no")
	endif(HAVE_MEMCHECK_H AND NOT DISABLE_VALGRIND)
ENDMACRO(mpc_check_valgrind)

########################################################################
#Check for hwloc
MACRO(mpc_check_hwloc)
	#set cache entry
	set(DISABLE_LIBNUMA no CACHE BOOL "Ignore presence of hwloc and do not try to build NUMA support.")

	# Search for hwloc for NUMA support
	if (NOT DISABLE_LIBNUMA)
		find_package(Hwloc QUIET)
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
ENDMACRO(mpc_check_hwloc)

########################################################################
MACRO(mpc_check_openpa)
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
ENDMACRO(mpc_check_openpa)

########################################################################
#Display info on current configuration
macro (print_variable_status var_name)
	if (${var_name})
		message(STATUS "${var_name} : yes")
	else (${var_value})
		message(STATUS "${var_name} : no")
	endif (${var_name})
endmacro (print_variable_status)

#check the presence of svUnitTest
find_package(svUnitTest)

########################################################################
macro(mpc_setup_testing)
	enable_testing()

	if (NOT WIN32)
		find_package(OpenMP REQUIRED)
	endif(NOT WIN32)

	if(SVUNITTEST_FOUND)
		set(UNITTEST_INCLUDE_DIR ${SVUNITTEST_INCLUDE_DIR})
		set(UNITTEST_LIBRARY ${SVUNITTEST_LIBRARY})
	else(SVUNITTEST_FOUND)
		#add_subdirectory(svUnitTest_fake)
		set(UNITTEST_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/svUnitTest_fake/)
		set(UNITTEST_LIBRARY "")
	endif(SVUNITTEST_FOUND)
endmacro(mpc_setup_testing)
