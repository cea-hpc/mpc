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
MPFR_DEPS=''
MPFR_BUILD_PARAMETERS=''

######################################################
findMPFR()
{
	#TODO

	#otherwise, set to ${INTERNAL_KEY}
	if [ -z "${MPFR_PREFIX}" ]; then
		MPFR_PREFIX="${INTERNAL_KEY}"
		MPFR_REAL_PREFIX="${PREFIX}"
	else
		MPFR_REAL_PREFIX="${MPFR_PREFIX}"
	fi
}
