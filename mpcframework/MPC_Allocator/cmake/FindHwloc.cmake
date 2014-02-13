# Find the shelltoolit library
#
#  This module defines the following variables:
#     HWLOC_FOUND       - True if HWLOC_INCLUDE_DIR & svUnitTest_LIBRARY are found
#     HWLOC_LIBRARIES   - Set when HWLOC_LIBRARY is found
#     HWLOC_INCLUDE_DIRS - Set when HWLOC_INCLUDE_DIR is found
#
#     HWLOC_INCLUDE_DIR - where to find shelltoolkit/shtkFrontEnd.h, etc.
#     HWLOC_LIBRARY     - the svUnitTest library
#

find_path(HWLOC_INCLUDE_DIR
	NAMES hwloc.h
	PATHS ${HWLOC_PREFIX}/include ${CMAKE_INSTALL_PREFIX}/include
	DOC "The HWLOC include directory"
)

find_library(HWLOC_LIBRARY
	NAMES hwloc
	PATHS ${HWLOC_PREFIX}/lib ${CMAKE_INSTALL_PREFIX}/lib
	DOC "The NUMA library"
)

# handle the QUIETLY and REQUIRED arguments and set ShellToolKit_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(HWLOC DEFAULT_MSG HWLOC_LIBRARY HWLOC_INCLUDE_DIR)

if(HWLOC_FOUND)
  set( HWLOC_LIBRARIES ${HWLOC_LIBRARY} )
  set( HWLOC_INCLUDE_DIRS ${HWLOC_INCLUDE_DIR} )
endif()

mark_as_advanced(HWLOC_INCLUDE_DIR HWLOC_LIBRARY)
