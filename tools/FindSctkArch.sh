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
SCTK_ARCH_DEPS=''
SCTK_ARCH_BUILD_PARAMETERS=''

######################################################
findSctkArch()
{
	#TODO

	#otherwise, set to ${INTERNAL_KEY}
	if [ -z "${SCTK_ARCH_PREFIX}" ]; then
		SCTK_ARCH_PREFIX="${INTERNAL_KEY}"
		SCTK_ARCH_REAL_PREFIX="${PREFIX}"
	else
		SCTK_ARCH_REAL_PREFIX="${SCTK_ARCH_PREFIX}"
	fi
}
