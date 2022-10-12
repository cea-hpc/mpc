#!/bin/bash

# set to directory of bash script
dir=$(cd -P -- "$(dirname -- "$0")" && pwd -P)
cd $dir

if [ "$1" == "localhost" ];
then
	make all || exit 1
elif [ "$1" == "inti" ];
then
	ccc_mprun -v -N 1 -x -p $1 make all || exit 1
fi
