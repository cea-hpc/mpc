# Find the shelltoolit library
#
#  This module defines the following variables:
#     OPENPA_FOUND       - True if OPENPA_INCLUDE_DIR & svUnitTest_LIBRARY are found
#     OPENPA_LIBRARIES   - Set when OPENPA_LIBRARY is found
#     OPENPA_INCLUDE_DIRS - Set when OPENPA_INCLUDE_DIR is found
#
#     OPENPA_INCLUDE_DIR - where to find shelltoolkit/shtkFrontEnd.h, etc.
#     OPENPA_LIBRARY     - the svUnitTest library
#

find_path(OPENPA_INCLUDE_DIR
	NAMES opa_primitives.h
	PATHS ${OPENPA_PREFIX}/include ${CMAKE_INSTALL_PREFIX}/include
	DOC "The OPENPA include directory"
)

# handle the QUIETLY and REQUIRED arguments and set ShellToolKit_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OPENPA DEFAULT_MSG OPENPA_INCLUDE_DIR)

if(OPENPA_FOUND)
  set( OPENPA_LIBRARIES ${OPENPA_LIBRARY} )
  set( OPENPA_INCLUDE_DIRS ${OPENPA_INCLUDE_DIR} )
endif()

mark_as_advanced(OPENPA_INCLUDE_DIR OPENPA_LIBRARY)
