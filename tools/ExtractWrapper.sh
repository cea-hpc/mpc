#!/bin/bash
######################################################
#            PROJECT  : svMPC                        #
#            VERSION  : 0.0.0                        #
#            DATE     : 11/2013                      #
#            AUTHOR   : Valat Sébastien              #
#            LICENSE  : CeCILL-C                     #
######################################################

#extract args
file="$1"

if [ ! -z "$STEP_WRAPPER_VERBOSE" ] && [ "$STEP_WRAPPER_VERBOSE" != '0' ]; then
	set -x
fi

#select extract mode
case "$file" in
	*.tar.bz2)
		tar -xjf "$file"
		exit $?
		;;
	*.tar.gz)
		tar -xzf "$file"
		exit $?
		;;
	*)
		echo "Unknown archive format to extract : $file"
		exit $?
		;;
esac
