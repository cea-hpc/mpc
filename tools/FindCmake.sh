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
#It will set CMAKE_PREFIX or just check if it can be used
CMAKE_DEPS=''
CMAKE_BUILD_PARAMETERS=''

######################################################
findCmakeOnSystem()
{
	#if not setup, need to find it, try by xml2-config in PATH
	if [ -z "${CMAKE_PREFIX}" ]; then
		getPrefixOfCmd 'CMAKE_PREFIX' 'cmake'
	fi
}

######################################################
findCmake()
{
	if [ "$ALL_INTERNALS" != 'true' ]; then
		findCmakeOnSystem
	fi

	#otherwise, set to ${INTERNAL_KEY}
	if [ -z "${CMAKE_PREFIX}" ]; then
		CMAKE_PREFIX="${INTERNAL_KEY}"
		CMAKE_REAL_PREFIX="${PREFIX}"
	else
		CMAKE_REAL_PREFIX="${CMAKE_PREFIX}"
	fi
}
