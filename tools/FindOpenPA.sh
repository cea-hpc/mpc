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
OPENPA_DEPS=''
OPENPA_BUILD_PARAMETERS=''

######################################################
findOpenPA()
{
	#TODO

	#otherwise, set to ${INTERNAL_KEY}
	if [ -z "${OPENPA_PREFIX}" ]; then
		OPENPA_PREFIX="${INTERNAL_KEY}"
		OPENPA_REAL_PREFIX="${PREFIX}"
	else
		OPENPA_REAL_PREFIX="${OPENPA_PREFIX}"
	fi
}
