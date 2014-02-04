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
#It will set LIBXML2_PREFIX or just check if it can be used
LIBXML2_DEPS=''
LIBXML2_BUILD_PARAMETERS=''

######################################################
findLibXML2OnSystem()
{
	#if not setup, need to find it, try by xml2-config in PATH
	if [ -z "${LIBXML2_PREFIX}" ]; then
		getPrefixOfCmd 'LIBXML2_PREFIX' 'xml2-config'
	fi

	#if still not found, search with pkg-config
	if [ -z "${LIBXML2_PREFIX}" ]; then
		getPrefixWithPkgConfig 'LIBXML2_PREFIX' 'libxml-2.0'
	fi
}

######################################################
findLibXML2()
{
	if [ "$ALL_INTERNALS" != 'true' ]; then
		findLibXML2OnSystem
	fi

	#otherwise, set to ${INTERNAL_KEY}
	if [ -z "${LIBXML2_PREFIX}" ]; then
		LIBXML2_PREFIX="${INTERNAL_KEY}"
		LIBXML2_REAL_PREFIX="${PREFIX}"
	else
		LIBXML2_REAL_PREFIX="${LIBXML2_PREFIX}"
	fi
}
