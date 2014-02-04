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
GMP_DEPS=''
GMP_BUILD_PARAMETERS=''

######################################################
findGmp()
{
	#TODO

	#otherwise, set to ${INTERNAL_KEY}
	if [ -z "${GMP_PREFIX}" ]; then
		GMP_PREFIX="${INTERNAL_KEY}"
		GMP_REAL_PREFIX="${PREFIX}"
	else
		GMP_REAL_PREFIX="${GMP_PREFIX}";
	fi
}
