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
#   - Adam Julien julien.adam@cea.fr                                   #
#                                                                      #
########################################################################
##################### VARIABLES #####################
# Variables need to be setted with Windows
#	- WINE_PATH 	--> path to wine64 executable (for tests)
#	- EXTERNAL_LIBS --> external libraries path

MODE="all"
LAUNCH="Debug"
EXIT_STATUS="0"
REBUILD="no"
OS="linux"
ROOT_FOLDER=""
CMAKE_ARGS=""
MAKE_J=1
EXTERNAL_LIBS="libgcc_s_sjlj-1.dll libgfortran-3.dll libobjc-4.dll libquadmath-0.dll libssp-0.dll libstdc++-6.dll"
MINGW_PATH="${HOME}/usr/minGW"
WINE_PATH="${HOME}/usr/wine/bin/wine64"
TARGET_ENV_PATH=""
INTERNALS_ARGS=""

print_help(){
	echo "##############################################################################"
	echo "#                       ./independant_build.sh USAGE                         #"
	echo "##############################################################################"
	echo "#                                                                            #"
	echo "# COMMON OPTIONS:                                                            #"
	echo "#                                                                            #"
	echo "#  --help, -h              Displays this help.                               #"
	echo "#  --mode=[Debug|Release]  Allows to run cmake with Debug or Release Mode.   #"
	echo "#  --clean                 Clean build folder.                               #"
	echo "#  --rebuild               Allows to clean build folder before execution.    #"
	echo "#  --all                   Execute CMake, make and test conmmand.            #"
	echo "#  --cmake                 Execute only CMake command.                       #"
	echo "#  --make                  Execute only make command.                        #"
	echo "#  --test                  Execute only test command.                        #"
	echo "#  --args=<args>           Arguments for CMake (separated by comma).         #"
	echo "#  --linux                 Run on Unix systems (default).                    #"
	echo "#  --win                   Run on Microsoft systems.                         #"
	echo "#  --target-path           Specifies where CMake have to search .CMake files.#"  
	echo "#                                                                            #"
	echo "# ONLY LINUX OPTIONS:                                                        #" 
	echo "#                                                                            #"
	echo "#  -j <num>, -j=<num>      Specifies Make -j options.                        #"
	echo "#                                                                            #"
	echo "# ONLY WINDOWS OPTIONS:                                                      #"
	echo "#                                                                            #"
	echo "#  --mingw-path=<path>     Specifies minGW prefix (bind libraries or exe).   #"
	echo "#                                                                            #"
	echo "##############################################################################"
}

read_param_value(){
	echo "$1" | sed -e "s/^$2=//g"
}

clean(){

	#clean up
	rm -rfv ${ROOT_FOLDER}/build/* 2> /dev/null
}

config(){
	if [ "$REBUILD" == "yes" ] ; then
		clean
	fi
	
	#execution
	cd ${ROOT_FOLDER}/build
	
	if [ "$OS" == "windows" ] ; then
		CHAIN="$(echo ${CMAKE_ARGS} | grep "\-DWINE_PATH=")"
		if [ "${CHAIN}" == "" ] ; then 
			echo "Error in argument to find Wine. You must add -DWINE_PATH=<path>"
			exit 1
		fi

		INTERNALS_ARGS="-DTARGET_ENV_PREFIX="${TARGET_ENV_PATH}" -DMINGW_PREFIX="${MINGW_PATH}" -DWIN32=yes -DCMAKE_TOOLCHAIN_FILE=${ROOT_FOLDER}/toolchain.cmake"
	fi

	cmake ${INTERNALS_ARGS} -DCMAKE_BUILD_TYPE=$LAUNCH ${CMAKE_ARGS} ..
	
	#if make fails
	if [ ! $? -eq 0 ] ; then
		echo "Error during Make command. Stop."
		exit 2
	fi
}

compil(){
	
	if [ "$OS" == "windows" ] ; then
		MAKE_J=1
	fi

	cd ${ROOT_FOLDER}/build
	make -j ${MAKE_J}
}

testing(){
	
	if [ "$OS" == "windows" ] ; then
		for dll in ${EXTERNAL_LIBS}
		do
			cp ${MINGW_PATH}/mingw/lib/$dll ${ROOT_FOLDER}/build/tests/
		done
	fi

	cd ${ROOT_FOLDER}/build
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
			LAUNCH="$(read_param_value $arg --mode)"
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
			MODE="all"
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
		--mingw-path=*)
			MINGW_PATH="$(read_param_value $arg --win-path)"
			;;
		--cmake)
			MODE="config"
			;;
		--make)
			MODE="compil"
			;;
		--test)
			MODE="test"
			;;
		--target-path=*)
			TARGET_ENV_PATH="$(read_param_value $arg --target-path)"
			;;
		*)
			echo "Error argument: \"$arg\""
			exit 1
			;;
	esac
done

case $MODE in
	"clean")
		clean
		;;
	"config")
		config
		;;
	"compil")
		compil
		;;
	"test")
		testing
		;;
	"all")
		config
		compil
		testing
		;;
	*)
		echo "Error Mode: \"$MODE\""
		;;
esac

exit "${EXIT_STATUS}"
