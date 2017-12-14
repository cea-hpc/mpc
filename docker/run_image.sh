#!/bin/sh

SRC="`readlink -e $0`"
SRC="`dirname $SRC`"
UNIQ=
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
			MPC_ENV=`read_param "$arg" "img="`
			;;
		list)
			printf "Available images:\n"
			docker images mpc-env
			exit 0
			;;
		--uniq)
			UNIQ="--rm"
			;;
		*)
			die "Unknown arg '$arg'"
			exit 2
			;;
	esac
done

test -n "`docker images -q mpc-env:$MPC_ENV`" || die "$MPC_ENV does not exist"

running_container="`docker ps -q -f "ancestor=mpc-env:$MPC_ENV"`"
container="`docker ps -q -a -f "ancestor=mpc-env:$MPC_ENV"`"
check_no_multiple_container "$container"

echo "Building MPC: /opt/src/mpc/installmpc --prefix=/home/mpcuser/install"

if test -n "$running_container"; then
	echo "Attaching to an already running container ($container)"
	docker attach $container

elif test -n "$container"; then
	echo "Starting from an existing container ($container)"
	docker start -ai $container
else
	echo "Running a new container for mpc-env:$MPC_ENV"
	test -n "$UNIQ" && echo "** Warning **: The container will be destroyed when exit (can take a while)"
	docker run $UNIQ -h $MPC_ENV -w /home/mpcuser/build -v $MPC_PATH:/opt/src/mpc -it mpc-env:$MPC_ENV
fi
