#!/bin/sh

SRC="`readlink -e $0`"
SRC="`dirname $SRC`"
UNIQ=
FROM_NET=no
MPC_ENV=alpine
MPC_PATH="$SRC/.."

function check_no_multiple_container()
{
	n="`echo "$1" | wc -l`"
	test "$n" != "1" && die "Multiple containers already exist. Please handle this by yourself \n$1"
}

function die()
{
	printf "Error: $@\n" 1>&2
	exit 42
}

function read_param()
{
	echo "xxA$1" | sed -e "s@xxA$2@@"
}

docker version > /dev/null || die "Docker not available on this system"

for arg in "$@"
do
	case $arg in
		img=*)
			os=`read_param "$arg" "img="`
			MPC_ENV="mpc-env:$os"
			;;
		list)
			printf "Available images:\n"
			docker images mpc-env
			exit 0
			;;
		--uniq)
			UNIQ="--rm"
			;;
		--net)
			FROM_NET="yes"
			;;
		*)
			die "Unknown arg '$arg'"
			exit 2
			;;
	esac
done


image=`docker images -q $MPC_ENV`
if test -z "$image"; then
	if test "$FROM_NET" = "yes"; then
		echo "Try to download the latest $MPC_ENV from hub.docker.com repository"
		docker pull paratoolsfrance/$MPC_ENV || die "$MPC_ENV does not remotely exist"
		MPC_ENV="paratoolsfrance/$MPC_ENV"
	else
		die "Unable to find a proper Docker image. Try to run ./build_image.sh [img=$os | --net ] first"
	fi
fi

running_container="`docker ps -q -f "ancestor=$MPC_ENV"`"
container="`docker ps -q -a -f "ancestor=$MPC_ENV"`"
check_no_multiple_container "$container"

echo "Building MPC: /opt/src/mpc/installmpc --prefix=/home/mpcuser/install"

if test -n "$running_container"; then
	echo "Attaching to an already running container ($container)"
	docker attach $container

elif test -n "$container"; then
	echo "Starting from an existing container ($container)"
	docker start -ai $container
else
	echo "Running a new container for $MPC_ENV"
	test -n "$UNIQ" && echo "** Warning **: The container will be destroyed when exit (can take a while)"
	docker run $UNIQ -h $os -w /home/mpcuser/build -v $MPC_PATH:/opt/src/mpc -it $MPC_ENV
fi
