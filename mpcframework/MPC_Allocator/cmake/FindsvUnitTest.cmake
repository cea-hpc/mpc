######################################################
#            PROJECT  : svUnitTest                   #
#            VERSION  : 0.3.0                        #
#            DATE     : 05/2011                      #
#            AUTHOR   : Valat Sébastien              #
#            LICENSE  : CeCILL-C                     #
######################################################

# Base source copied from cmake usr/share/cmake/Modules/FindsvUnitTest.cmake
# Thanks to them.

# Find the svUnitTest library
#
#  This module defines the following variables:
#     SVUNITTEST_FOUND       - True if SVUNITTEST_INCLUDE_DIR & svUnitTest_LIBRARY are found
#     SVUNITTEST_LIBRARIES   - Set when SVUNITTEST_LIBRARY is found
#     SVUNITTEST_INCLUDE_DIRS - Set when SVUNITTEST_INCLUDE_DIR is found
#
#     SVUNITTEST_INCLUDE_DIR - where to find svUnitTest/svutTestCase.h, etc.
#     SVUNITTEST_LIBRARY     - the svUnitTest library
#

find_path(SVUNITTEST_INCLUDE_DIR
	NAMES svUnitTest/svutTestCase.h
	PATHS ${SVUNITTEST_PREFIX}/include
	DOC "The svUnitTest include directory"
)

find_library(SVUNITTEST_LIBRARY
	NAMES svunittest
	PATHS ${SVUNITTEST_PREFIX}/lib
	DOC "The svUnitTest library"
)

# handle the QUIETLY and REQUIRED arguments and set svUnitTest_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(svUnitTest DEFAULT_MSG SVUNITTEST_LIBRARY SVUNITTEST_INCLUDE_DIR)

if(SVUNITTEST_FOUND)
  set( SVUNITTEST_LIBRARIES ${SVUNITTEST_LIBRARY} )
  set( SVUNITTEST_INCLUDE_DIRS ${SVUNITTEST_INCLUDE_DIR} )
endif()

mark_as_advanced(SVUNITTEST_INCLUDE_DIR SVUNITTEST_LIBRARY)
