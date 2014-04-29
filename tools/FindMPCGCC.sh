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
MPC_GCC_DEPS=''
MPC_GCC_BUILD_PARAMETERS=''

######################################################
findMPCGCC()
{
	#TODO

	#otherwise, set to ${INTERNAL_KEY}
	if [ -z "${MPC_GCC_PREFIX}" ]; then
		MPC_GCC_PREFIX="${INTERNAL_KEY}"
		MPC_GCC_REAL_PREFIX="${PREFIX}"
	else
		MPC_GCC_REAL_PREFIX="${MPC_GCC_PREFIX}"
	fi
}
