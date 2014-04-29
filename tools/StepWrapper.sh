#!/bin/bash
######################################################
#            PROJECT  : svMPC                        #
#            VERSION  : 0.0.0                        #
#            DATE     : 11/2013                      #
#            AUTHOR   : Valat Sébastien              #
#            LICENSE  : CeCILL-C                     #
######################################################

#extract args
stepinfo="$1"
shift

#setup colors
if [ "${ENABLE_COLOR}" = 'true' ]; then
	COLOR_START=`printf '\e[1m'`
	COLOR_RESET=`printf '\e[0m'`
	COLOR_ERROR=`printf '\e[91m'`
	COLOR_SUCCESS=`printf '\e[32m'`
	COLOR_WORKER=`printf '\e[90m'`
	COLOR_COMMAND=`printf '\e[94m'`
else
	COLOR_START=''
	COLOR_RESET=''
	COLOR_ERROR=''
	COLOR_SUCCESS=''
	COLOR_WORKER=''
fi

#Print start
echo "${COLOR_START} -> $stepinfo...${COLOR_RESET}" 1>&2

#Capture error when first process of pipe chain fail
#TODO Check compatibility with sh
set -o pipefail

#run
status=''
if [ "$STEP_WRAPPER_VERBOSE" = '2' ] || [ "$STEP_WRAPPER_VERBOSE" = '3' ]; then
	echo "${COLOR_COMMAND} -> $@${COLOR_RESET}" 1>&2
	"$@" 2>&1 | sed -e "s#^#${COLOR_WORKER}[$stepinfo]${COLOR_RESET} #" || exit $?
	status='ok'
elif [ "$STEP_WRAPPER_VERBOSE" = '1' ]; then
	echo "${COLOR_COMMAND} -> $@${COLOR_RESET}" 1>&2
	#oops may be limited to bash, need to find another way to capture error with this filtering way
	"$@" 2>&1 1> /dev/null | sed -e "s#^#${COLOR_WORKER}[$stepinfo]${COLOR_RESET} #" && status='ok'
else
	"$@" 1> out.log 2>&1  && status="ok"
fi

#Print end
if [ "$status" = 'ok' ]; then
	echo "${COLOR_SUCCESS} -> $stepinfo finished with success${COLOR_RESET}" 1>&2
else
	echo "${COLOR_ERROR}GET ERROR ON STEP $stepinfo ${COLOR_RESET}" 1>&2
	if [ -f 'out.log' ]; then
		cat 'out.log' | sed -e "s#^#${COLOR_WORKER}[$stepinfo]${COLOR_RESET} #" 1>&2
	fi
	exit 1
fi
