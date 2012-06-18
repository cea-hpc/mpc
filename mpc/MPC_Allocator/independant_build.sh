#!/bin/bash
############################# MPC License ##############################
# Wed Nov 19 15:19:19 CET 2008                                         #
# Copyright or (C) or Copr. Commissariat a l'Energie Atomique          #
#                                                                      #
# IDDN.FR.001.230040.000.S.P.2007.000.10000                            #
# This file is part of the MPC Runtime.                                #
#                                                                      #
# This software is governed by the CeCILL-C license under French law   #
# and abiding by the rules of distribution of free software.  You can  #
# use, modify and/ or redistribute the software under the terms of     #
# the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     #
# following URL http://www.cecill.info.                                #
#                                                                      #
# The fact that you are presently reading this means that you have     #
# had knowledge of the CeCILL-C license and that you accept its        #
# terms.                                                               #
#                                                                      #
# Authors:                                                             #
#   - Adam Julien julien.adam.ocre@cea.fr                              #
#                                                                      #
########################################################################
##################### VARIABLES #####################
# Variables need to be setted with Windows
#	- WINE_PATH 	--> path to wine64 executable (for tests)
#	- EXTERNAL_LIBS --> external libraries path

MODE="Debug"
EXIT_STATUS="0"
REBUILD="no"
OS="linux"
ROOT_FOLDER=""
CMAKE_ARGS=""
MAKE_J=1
EXTERNAL_LIBS="/home/julia/usr/lib"
print_help(){
	echo "#############################################################################"
	echo "#                      ./independant_build.sh USAGE                         #"
	echo "#############################################################################"
	echo "#                                                                           #"
	echo "#  --help, -h              Displays this help.                              #"
	echo "#  --mode=[Debug|Release]  Allows to run cmake with Debug or Release Mode.  #"
	echo "#  --setup                 Make some actions to prepare CMake execution     #"
	echo "#  --clean                 Clean build folder.                              #"
	echo "#  --rebuild               Execute CMake command after clean up build folder#"
	echo "#  --all                   Clean up and run Debug Mode.                     #"
	echo "#  --args=<args>           Arguments for CMake instructions (separated      #"
	echo "#                          by comma).                                       #"
	echo "#  -j <num>, -j=<num>      Specifies Make -j options.                       #"
	echo "#  --ext-libs=<path>        Specifies path to bind minGW with hers libraries #"
	echo "#                                                                           #"
	echo "#############################################################################"
}

read_param_value(){
	echo "$1" | sed -e "s/^$2=//g"
}

clean(){

	#clean up
	rm -rfv ${ROOT_FOLDER}/build/* 2> /dev/null
	delinker
}

linker(){
	#link
	echo "SymLink Creation"
	ln -s ${ROOT_FOLDER}/../../MPC_Test_Suite/UnitTests/Allocator tests 2> /dev/null
}

delinker(){

	#delink
	echo "SymLink Destruction..."
	rm ${ROOT_FOLDER}/tests 2> /dev/null
}

run(){
	if [ "$REBUILD" == "yes" ] ; then
		clean
	fi
	
	linker
	
	#execution
	cd ${ROOT_FOLDER}/build
	case $OS in
		"linux")
			run_linux;;
		"windows")
			run_win;;
	esac
	cd ..

	delinker
}

run_linux(){
	cmake -DCMAKE_BUILD_TYPE=$MODE .. ${CMAKE_ARGS} && make -j ${MAKE_J} && make test
}

run_win(){
	#some checks
	CHAIN="$(echo ${CMAKE_ARGS} | grep "\-DWINE_PATH=")"
	if [ "${CHAIN}" == "" ] ; then 
		echo "Error in argument to find Wine. You must add -DWINE_PATH=<path>"
		delinker
		exit 1
	fi
	
	cmake -DWIN32=yes -DCMAKE_TOOLCHAIN_FILE=${ROOT_FOLDER}/toolchain.cmake -DCMAKE_BUILD_TYPE=$MODE ${CMAKE_ARGS} .. && make -j 1
	if [ ! $? -eq 0 ] ; then
		echo "Error during Make command. Stop."
		exit 2
	fi
		cp ${EXTERNAL_LIBS}/* ${ROOT_FOLDER}/build/tests/
		cd tests
		make test
		cd ..
}

######################### MAIN ######################

#some check
if [ ! -f "${PWD}/independant_build.sh" ] ; then
	echo "Script must be executed like this:  ./test_allocator.sh."
	exit 1
fi

if [ ! -d "${PWD}/build" ] ; then
	mkdir -p ${PWD}/build
fi

ROOT_FOLDER=${PWD}

for arg in $*
do 
	case $arg in
		--linux)
			OS="linux"
			;;
		--win)
			OS="windows"
			;;
		--mode=*)
			MODE="$(read_param_value $arg --mode)"
			;;
		--setup)
			MODE="setup"
			;;
		--clean)
			MODE="clean"
			;;
		--help|-h)
			print_help
			exit 0
			;;
		--rebuild)
			REBUILD="yes"
			;;
		--all)
			MODE="Debug"
			REBUILD="yes"
			;;
		--args=*)
			CMAKE_ARGS="$(read_param_value $arg --args | sed -e 's/\"//g' -e 's/,/ /g')"
			;;
		-j=*)
			MAKE_J=$(read_param_value $arg -j)
			;;
		-j*)
			MAKE_J=$(echo $arg | sed -e "s/-j//g")
			;;
		--ext-libs=*)
			EXTERNAL_LIBS="$(read_param_value $arg --ext-libs)"
			;;
		*)
			echo "Error argument: \"$arg\""
			exit 1
			;;
	esac
done

case $MODE in
	"setup")
		clean
		linker
		;;
	"clean")
		clean
		;;
	"Debug"|"Release")
		run	
		;;
	*)
		echo "Error Mode: \"$MODE\""
		;;
esac

exit "${EXIT_STATUS}"
