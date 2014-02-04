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
MPC_GDB_DEPS=''
MPC_GDB_BUILD_PARAMETERS=''

######################################################
findMPCGDB()
{
	#TODO

	#otherwise, set to ${INTERNAL_KEY}
	if [ -z "${MPC_GDB_PREFIX}" ]; then
		MPC_GDB_PREFIX="${INTERNAL_KEY}"
		MPC_GDB_REAL_PREFIX="${PREFIX}"
	else
		MPC_GDB_REAL_PREFIX="${MPC_GDB_PREFIX}"
	fi
}
