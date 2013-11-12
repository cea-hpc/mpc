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
#Global setup
PROJECT(mpc_allocator)
cmake_minimum_required(VERSION 2.6)
enable_testing()

########################################################################
#Some global setup for debug
# add_definitions(-DSCTK_ALLOC_SPY)
# add_definitions(-DENABLE_TRACE)

########################################################################
#Use internal cmake files and load some files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/)
include(macros)

########################################################################
#set some global options
set(DISABLE_UNIT_TESTS no CACHE BOOL "Disable build of allocator unit tests.")

########################################################################
#Do some setup actions
mpc_enable_rpath()
mpc_check_stdbool()
mpc_check_valgrind()
mpc_check_hwloc()
mpc_check_openpa()

########################################################################
#Summary
message(STATUS "============== SUMMARY ==================")
message(STATUS "CMAKE_BUILD_TYPE : ${CMAKE_BUILD_TYPE}")
print_variable_status("ENABLE_RPATH")
print_variable_status("DISABLE_UNIT_TESTS")
print_variable_status("HWLOC_FOUND")
print_variable_status("OPENPA_FOUND")
message(STATUS "=========================================")

########################################################################
#Compile unit tests if not disabled
add_subdirectory(src)
if(NOT DISABLE_UNIT_TESTS)
	add_subdirectory(tests)
endif(NOT DISABLE_UNIT_TESTS)

########################################################################
#Files to install
install(FILES include/mpcalloc.h DESTINATION include/)
