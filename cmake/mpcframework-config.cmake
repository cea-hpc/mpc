############################# MPC License ##############################
# Wed Nov 19 15:19:19 CET 2008                                         #
# Copyright or (C) or Copr. Commissariat a l Energie Atomique          #
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
#   - BESNARD Jean-Baptiste  jbbesnard@paratools.fr                    #
#                                                                      #
########################################################################
#[=======================================================================[.rst:
FindMPCFRAMEWORK
-------

Finds the MPCFRAMEWORK library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``MPCFRAMEWORK::MPCFRAMEWORK``
  The MPCFRAMEWORK library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``MPCFRAMEWORK_FOUND``
  True if the system has the MPCFRAMEWORK library.
``MPCFRAMEWORK_VERSION``
  The version of the MPCFRAMEWORK library which was found.
``MPCFRAMEWORK_INCLUDE_DIRS``
  Include directories needed to use MPCFRAMEWORK.
``MPCFRAMEWORK_LIBRARIES``
  Libraries needed to link to MPCFRAMEWORK.

FindMPI Like
^^^^^^^^^^^^

Note that for now we do not support fortran in variables below yet.
<lang> is in (C, CXX)

``MPI_FOUND``
  Variable indicating that MPI settings for C/CXX was found

``MPI_VERSION``
  Minimal version of MPI detected among the requested languages, or all enabled languages if no components were specified.

This module will set the following variables per language in your project, where <lang> is one of C, CXX, or Fortran:

``MPI_<lang>_FOUND``
  Variable indicating the MPI settings for <lang> were found and that simple MPI test programs compile with the provided settings.

``MPI_<lang>_COMPILER``
  MPI compiler for <lang> if such a program exists.

``MPI_<lang>_INCLUDE_DIRS``
  Include path(s) for MPI header.

``MPI_<lang>_LIBRARIES``
   All libraries to link MPI programs against.


Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``MPCFRAMEWORK_INCLUDE_DIR``
  The directory containing ``mpc.h`` and ``mpi.h``.
``MPCFRAMEWORK_LIBRARY``
  The path to the MPCFRAMEWORK library.

#]=======================================================================]

# First Proceed To the Basic PKG-CONFIG search

find_package(PkgConfig)
pkg_check_modules(MPCFRAMEWORK QUIET libmpcmpi)


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MPCFRAMEWORK
  FOUND_VAR MPCFRAMEWORK_FOUND
  REQUIRED_VARS
    MPCFRAMEWORK_LIBRARIES
    MPCFRAMEWORK_INCLUDE_DIRS
  VERSION_VAR MPCFRAMEWORK_VERSION
)

Message(STATUS "MPCFRAMEWORK version ${MPCFRAMEWORK_VERSION} found in ${MPCFRAMEWORK_PREFIX}")


mark_as_advanced(
  MPCFRAMEWORK_INCLUDE_DIR
  MPCFRAMEWORK_LIBRARY
)

# Not try to mimic some of the FindMPI things


if (MPCFRAMEWORK_FOUND)
    set(MPI_FOUND TRUE)
    # Yes we'll need to detect..
    set(MPI_VERSION_MAJOR "3" CACHE STRING "Major MPI version")
    set(MPI_VERSION_MINOR "1" CACHE STRING "Minor MPI version")
    set(MPI_VERSION "${MPI_VERSION_MAJOR}.${MPI_VERSION_MINOR}" CACHE STRING "MPI version")

    set(MPI_C_FOUND TRUE CACHE BOOL "C Interface for MPI was found")
    set(MPI_C_INCLUDE_DIRS "${MPCFRAMEWORK_INCLUDE_DIRS}" CACHE PATH "Include Directories for C")
    set(MPI_C_LIBRARIES "${MPCFRAMEWORK_INCLUDE_DIRS}" CACHE STRING "Libraries Directories for C")
    find_program(MPI_C_COMPILER mpc_cc PATHS ${MPCFRAMEWORK_PREFIX}/bin/ REQUIRED)

    set(MPI_CXX_FOUND TRUE CACHE BOOL "CXX Interface for MPI was found")
    set(MPI_CXX_INCLUDE_DIRS "${MPCFRAMEWORK_INCLUDE_DIRS}" CACHE PATH "Include Directories for CXX")
    set(MPI_CXX_LIBRARIES "${MPCFRAMEWORK_LIBRARIES}" CACHE STRING "Libraries Directories for CXX")
    find_program(MPI_CXX_COMPILER mpc_cxx PATHS ${MPCFRAMEWORK_PREFIX}/bin/ REQUIRED)


else()
    set(MPI_FOUND FALSE)
endif()
