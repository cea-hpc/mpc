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
HWLOC_DEPS=''
HWLOC_BUILD_PARAMETERS=''

######################################################
findHwlocOnSystem()
{
	#if still not found, search with pkg-config
	if [ -z "${HWLOC_PREFIX}" ]; then
		getPrefixWithPkgConfig 'HWLOC_PREFIX' 'hwloc'
	fi
}

######################################################
findHwloc()
{
	if [ "$ALL_INTERNALS" != 'true' ]; then
		findHwlocOnSystem
	fi

	#otherwise, set to ${INTERNAL_KEY}
	if [ -z "${HWLOC_PREFIX}" ]; then
		HWLOC_PREFIX="${INTERNAL_KEY}"
		HWLOC_REAL_PREFIX="${PREFIX}"
	else
		HWLOC_REAL_PREFIX="${HWLOC_PREFIX}"
	fi
}
