#!/bin/sh

SRC="`readlink -e $0`"
SRC="`dirname $SRC`"
CUR=$PWD
MPC_ENV=alpine
MPC_HEAD=HEAD
MPC_PATH=$SRC/..

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
		head=*)
			MPC_HEAD="`read_param "$arg" "head="`"
			;;
		*)
			die "Unknown arg: '$arg'"
			exit 4
			;;
	esac
done

docker version > /dev/null || die "Docker not available on this system"
test -d "$SRC/$MPC_ENV" || die "$SRC/$MPC_ENV image does not exist yet"

echo "Building $MPC_ENV image. Please wait..."
docker build -t mpc-env:$MPC_ENV $SRC/$MPC_ENV
echo "Done !"

