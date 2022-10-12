#!/bin/bash

function print_help(){
	echo "Utility to run mpi tests in log/gdb/valgrind configuration."
	echo "  -n/--nodes [n]: number of nodes;"
	echo "  -mp/--mpc-install-path [p]: path to mpc install directory;"
	echo "  -t/--run-type [0|1|2]: 0=log, 1=gdb, 2=valgrind;"
	echo "  -pc/--portals-count [n]: number of Portals interface;"
	echo "  -tc/--tcp-count [n]: number of TCP interface;"
	echo "  -p/--program [p]: path to executable"
}

POSITIONAL_ARGS=()

while [[ $# -gt 0 ]]; do
  case $1 in
    -n|--nodes)
      NODES="$2"
      shift # past argument
      shift # past value
      ;;
    -mp|--mpc-install-path)
      INSTALLPATH="$2"
      shift # past argument
      shift # past value
      ;;
    -t|--run-type)
      RUNTYPE=$2
      shift # past argument
      shift # past value 
      ;;
    -pc|--portals-count)
      PORTALSCOUNT=$2
      shift # past argument
      shift # past value 
      ;;
    -tc|--tcp-count)
      TCPCOUNT=$2
      shift # past argument
      shift # past value 
      ;;
    -p|--program)
      PROGRAM=$2
      shift # past argument
      shift # past value 
      ;;
    -h|--help)
      print_help
      exit 0
      ;;
    -*|--*)
      echo "Unknown option $1"
      exit 1
      ;;
    *)
      POSITIONAL_ARGS+=("$1") # save positional arg
      shift # past argument
      ;;
  esac
done

set -- "${POSITIONAL_ARGS[@]}" # restore positional parameters

# set directory of bash script
dir=$(cd -P -- "$(dirname -- "$0")" && pwd -P)
cd $dir

# source specific version
echo "MPC install path: ${INSTALLPATH}"
source ${INSTALLPATH}/mpcvars.sh

echo "Spack load openpa..."
spack load openpa@1.0.4

echo "Setting pcvs build dir..."
PCVSBUILDDIR=$dir/../.pcvs-build/test_suite/mpi
PROGRAM=$PCVSBUILDDIR/$PROGRAM

MPCFRAMEWORK_LOWCOMM_PROTOCOL_LCRLIST_PORTALSMPI_COUNT=${PORTALSCOUNT}
export MPCFRAMEWORK_LOWCOMM_PROTOCOL_LCRLIST_PORTALSMPI_COUNT
MPCFRAMEWORK_LOWCOMM_PROTOCOL_LCRLIST_TCPMPI_COUNT=${TCPCOUNT}
export MPCFRAMEWORK_LOWCOMM_PROTOCOL_LCRLIST_TCPMPI_COUNT

NCORE=$NODES
NNODE=$NODES
NPROC=$NODES

if [ $PORTALSCOUNT -ge 1 ]; then
	export PTL_IFACE_NAME=lo
	echo "Portal interface name: $PTL_IFACE_NAME"
fi

case $RUNTYPE in
0)
	mpcrun -vvv -c=1 --net=lcp -m=pthread -n=$NNODE -N=$NPROC -p=$NCORE \
		./out.sh $PROGRAM;;
1)
	# xterm debug
	mpcrun -vvv -c=1 --net=lcp -m=pthread -n=$NNODE -N=$NPROC -p=$NCORE \
		xterm -e gdb --command=./gdbscript.gdb -e $PROGRAM;;
2)
	# valgrind
	# %p print current process id, see 
	# https://valgrind.org/docs/manual/manual-core.html#manual-core.basicopts
	mpcrun -c=1 --net=lcp -m=pthread -n=$NNODE -N=$NPROC -p=$NCORE  \
		valgrind --log-file="vg-lcp-%p.out" --leak-check=full \
		--show-leak-kinds=all --tool=memcheck --leak-check=yes  \
		$PROGRAM;;
*)
	echo "Unknown run type";;
esac
