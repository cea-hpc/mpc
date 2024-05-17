#!/bin/bash

SRC="`readlink -e $0`"
SRC="`dirname $SRC`"
CUR=$PWD
MPC_ENV="debian-stretch centos-7"
MPC_PATH=$SRC/..
MPC_VERSION=3.3.1

function die()
{
	printf "Error: $@\n" 1>&2
	exit 42
}

function read_param()
{
	echo "xxA$1" | sed -e "s@xxA$2@@"
}

for arg in "$@"
do
	case $arg in
		img=*)
			MPC_ENV=`read_param "$arg" "img="`
			;;
		list)
			printf "Available images:\n"
			\ls -d */
			exit 0
			;;
		*)
			die "Unknown arg: '$arg'"
			exit 4
			;;
	esac
done

docker version > /dev/null || die "Docker not available on this system"

for img in $MPC_ENV
do
	dir=${img//-*/}
	tag=${img//*-/}
	test -d "$SRC/$dir" || die "$SRC/$dir image does not exist yet"
	echo "- Building image for $img:"
	for step in env inst demo;
	do
		echo "   * $step layer"
		docker build --build-arg img_tag=$tag --build-arg mpc_version=$MPC_VERSION --target $step --rm -f "docker/$dir/Dockerfile" -t paratoolsfrance/mpc-$step:$img $SRC/$dir
	done
done
echo "Done !"
