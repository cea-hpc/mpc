#!/bin/bash
######################################################
#            PROJECT  : svMPC                        #
#            VERSION  : 0.0.0                        #
#            DATE     : 11/2013                      #
#            AUTHOR   : Valat Sébastien              #
#            LICENSE  : CeCILL-C                     #
######################################################

######################################################
#This file search libXML2 in system and provided prefix
#It will set HWLOC_PREFIX or just check if it can be used
BINUTILS_DEPS=''
BINUTILS_BUILD_PARAMETERS=''

######################################################
findBinutils()
{
	#binutils are built by default except if manually defined
	#as pre-installed

	#otherwise, set to ${INTERNAL_KEY}
	if [ -z "${BINUTILS_PREFIX}" ]; then
		BINUTILS_PREFIX="${INTERNAL_KEY}"
		BINUTILS_REAL_PREFIX="${PREFIX}"
	else
		BINUTILS_REAL_PREFIX="${BINUTILS_PREFIX}"
	fi
}
