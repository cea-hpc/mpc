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
#   - Julien Adam : julien.adam.ocre@cea.fr                            #
#                                                                      #
########################################################################
MODE="Debug"
EXIT_STATUS="0"
REBUILD="no"
OS="linux"

print_help(){
	echo "###########################################################################"
	echo "#                      ./independant_build.sh USAGE                       #"
	echo "###########################################################################"
	echo "#                                                                         #"
	echo "#  --help, -h              Displays this help.                            #"
	echo "#  --mode=[Debug|Release]  Allows to run cmake with Debug or Release Mode.#"
	echo "#  --setup                 Make some actions to prepare CMake execution   #"
	echo "#  --clean                 Clean build folder.                            #"
	echo "#  --rebuild               Execute CMake command after clean up build     #"
	echo "#                          folder.                                        #"
	echo "#  --all                   Clean up and run Debug Mode.                   #"
	echo "#  --lin                   Select Linux platform (by default).            #"
	echo "#  --win                   Select minGW platform.                         #"
	echo "#                                                                         #"
	echo "###########################################################################"
}

read_param_value(){
	echo "$1" | sed -e "s/^$2=//g"
}


clean(){

	#clean up
	rm -rfv build/* 2> /dev/null
	delinker
}

linker(){
	#link
	echo "SymLink Creation"
	ln -s ../../MPC_Test_Suite/UnitTests/Allocator tests 2> /dev/null
}

delinker(){

	#delink
	echo "SymLink Destruction..."
	rm tests 2> /dev/null
}

run(){
	if [ "$REBUILD" == "yes" ] ; then
		clean
	fi
	
	linker
	
	#execution
	cd build

	case $OS in
		"linux")
			run_linux
			;;
	
		"windows")
			run_win
			;;
	esac
	cd ..
}


run_linux(){
	cmake -DCMAKE_BUILD_TYPE=$MODE .. && make && make test
}

run_win(){
	echo "nothing to do here..."
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

for arg in $*
do 
	case $arg in 
		--lin)
			OS="linux"
			;;

		--win)
			OS="windows"
			;;
		--mode=*)
			MODE=$(read_param_value $arg --mode)
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

		
		



