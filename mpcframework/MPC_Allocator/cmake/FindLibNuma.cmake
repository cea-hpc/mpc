# Find the shelltoolit library
#
#  This module defines the following variables:
#     LIBNUMA_FOUND       - True if LIBNUMA_INCLUDE_DIR & svUnitTest_LIBRARY are found
#     LIBNUMA_LIBRARIES   - Set when LIBNUMA_LIBRARY is found
#     LIBNUMA_INCLUDE_DIRS - Set when LIBNUMA_INCLUDE_DIR is found
#
#     LIBNUMA_INCLUDE_DIR - where to find shelltoolkit/shtkFrontEnd.h, etc.
#     LIBNUMA_LIBRARY     - the svUnitTest library
#

find_path(LIBNUMA_INCLUDE_DIR
	NAMES numa.h
	PATHS ${LIBNUMA_PREFIX}/include ${CMAKE_INSTALL_PREFIX}/include
	DOC "The libnuma include directory"
)

find_library(LIBNUMA_LIBRARY
	NAMES numa
	PATHS ${LIBNUMA_PREFIX}/lib ${CMAKE_INSTALL_PREFIX}/lib
	DOC "The NUMA library"
)

# handle the QUIETLY and REQUIRED arguments and set ShellToolKit_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibNuma DEFAULT_MSG LIBNUMA_LIBRARY LIBNUMA_INCLUDE_DIR)

if(LIBNUMA_FOUND)
  set( LIBNUMA_LIBRARIES ${LIBNUMA_LIBRARY} )
  set( LIBNUMA_INCLUDE_DIRS ${LIBNUMA_INCLUDE_DIR} )
endif()

mark_as_advanced(LIBNUMA_INCLUDE_DIR LIBNUMA_LIBRARY)
