#!/bin/bash
#set -x
function print_help(){
	echo "Utility to build clean test environment and run unit test in {log/gdb/valgrind/callgrind} configuration."
	echo "  --build: import scripts and set test environment;"
	echo "  -n/--nodes [n]: number of nodes;"
	echo "  -t/--run-type {0,1,2,3}: 0=log, 1=gdb, 2=valgrind, 3=callgrind;"
	echo "  -c/--config [file]: path to config path;"
	echo "  -m/--machine [name]: name of machine to use;"
	echo "  -ts/--test-suite [name]: name of test suite {nas,lcp,mpi,imb};"
	echo "  -id/--mpc-install-dir [dir]: mpc install directory name (base from machines.[].workdir in conf file);"
	echo "  -bd/--pcvs-build-dir [dir]: pcvs build directory name (base from machines.[].testsuites[].dir in conf file);"
	echo "  -p/--program [p]: path to lcp executable."
	echo "  -pa/--args [p]: program arguments."
}

POSITIONAL_ARGS=()

VERBOSE=""
PROGRAMARGS=""
while [[ $# -gt 0 ]]; do
  case $1 in
    --build)
      break
      ;;
    -n|--nodes)
      NODES="$2"
      shift # past argument
      shift # past value
      ;;
    -t|--run-type)
      RUNTYPE=$2
      shift # past argument
      shift # past value 
      ;;
    -m|--machine)
      MACHINE=$2
      shift # past argument
      shift # past value 
      ;;
    -ts|--test-suite)
      TESTSUITE=$2
      shift # past argument
      shift # past value 
      ;;
    -id|--mpc-install-dir)
      MPCINSTALLDIR=$2
      shift # past argument
      shift # past value 
      ;;
    -bd|--pcvs-build-dir)
      PCVSBUILDDIR=$2
      shift # past argument
      shift # past value 
      ;;
    -p|--program)
      PROGRAM=$2
      FOUTPUT=$2
      shift # past argument
      shift # past value 
      ;;
    -pa|--args)
      PROGRAMARGS=$2
      shift # past argument
      shift # past value 
      ;;
    -c|--config)
      CONFIGFILE=$2
      shift # past argument
      shift # past value
      ;;
    -v|--verbose)
      VERBOSE="-vv"
      shift
      ;;
    -vv)
      VERBOSE="-vvv"
      shift
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

# check config file
if [ ! -f "$CONFIGFILE" ]; then
	echo "$CONFIGFILE does not exist. Did you use absolute path ? Stop..."
	exit 1
fi

## set up configuration
# check partition
PARTITION=$(cat $CONFIGFILE | jq -r --arg MACHINE "$MACHINE" '.machines[] | select(.name == $MACHINE) | .partition')
if [ "$PARTITION" = "" ]; then 
	echo "${MACHINE} does not exists in config file ${CONFIGFILE}. Stop..."
	exit 1
fi
# additionnal slurm args
SLURMARGS=$(cat $CONFIGFILE | jq -r --arg MACHINE "$MACHINE" '.machines[] | select(.name == $MACHINE) | .args')

# openpa architecture
OPENPAARCH=$(cat $CONFIGFILE | jq -r --arg MACHINE "$MACHINE" '.machines[] | select(.name == $MACHINE) | .openpa')

# check mpc install path
INSTALLPATH=$(cat $CONFIGFILE | jq -r --arg MACHINE "$MACHINE" --arg TESTSUITE "$TESTSUITE" \
	'.machines[] | select(.name == $MACHINE) | .workdir')
INSTALLPATH+=$MPCINSTALLDIR
if [ ! -d "$INSTALLPATH" ]; then
	echo "MPC install directory $INSTALLPATH does not exist. Stop..."
	exit 1
fi
# load mpc env 
source ${INSTALLPATH}/mpcvars.sh

# check pcvs build dir
TESTBUILDDIR=$(cat $CONFIGFILE | jq -r --arg MACHINE "$MACHINE" --arg TESTSUITE "$TESTSUITE" \
	'.machines[] | select(.name == $MACHINE) | .testsuites[] | select(.name == $TESTSUITE) | .dir')
TESTBUILDDIR+=$PCVSBUILDDIR
if [ ! -d "$TESTBUILDDIR" ]; then
	echo "PCVS build directory $INSTALLPATH does not exist. Stop..."
	exit 1
fi

# check program
PROGRAM=$TESTBUILDDIR/$PROGRAM
if [ ! -f "$PROGRAM" ]; then
	echo "$PROGRAM does not exist. Stop..."
	exit 1
fi

if [ "$TESTSUITE" = "lcp" ]; then
	echo "Spack load openpa..."
	spack load openpa@1.0.4 arch=$OPENPAARCH
fi

echo "Setting mpc configuration..."
# get run config 
NPTL=$(cat $CONFIGFILE | jq -r '.run.n_ptl')
NTCP=$(cat $CONFIGFILE | jq -r '.run.n_tcp')
FLEN=$(cat $CONFIGFILE | jq -r '.run.f_len')
FSIZE=$(cat $CONFIGFILE | jq -r '.run.f_size')

MPCFRAMEWORK_LOWCOMM_PROTOCOL_LCRLIST_PORTALSMPI_COUNT=${NPTL}
export MPCFRAMEWORK_LOWCOMM_PROTOCOL_LCRLIST_PORTALSMPI_COUNT
MPCFRAMEWORK_LOWCOMM_PROTOCOL_LCRLIST_TCPMPI_COUNT=${NTCP}
export MPCFRAMEWORK_LOWCOMM_PROTOCOL_LCRLIST_TCPMPI_COUNT
export MPCFRAMEWORK_LOWCOMM_PROTOCOL_LCRLIST_PORTALSMPI_MAX=4

export MPCFRAMEWORK_LOWCOMM_PROTOCOL_FRAGLENGTH=${FLEN}
export MPCFRAMEWORK_LOWCOMM_PROTOCOL_FRAGSIZE=${FSIZE}

echo "Setting cluster configuration..."
CLUSTERARG=""
if [ ! "$PARTITION" = "null" ]; then
	CLUSTERARG="--opt=-p $PARTITION $SLURMARGS"
	if [ $RUNTYPE -eq 1 ];then
		CLUSTERARG+=" --x11=all"
	fi
	# export iface of login node, otherwise mpc_print_config fails
	export PTL_IFACE_NAME=$(ip -o -4 route show to default | awk 'END{print $5}')
else
	export PTL_IFACE_NAME=lo
fi

NCORE=$NODES
NNODE=$NODES
NPROC=$NODES

case $RUNTYPE in
0)
	mpcrun $VERBOSE -c=1 --net=lcp -m=pthread -n=$NNODE -N=$NPROC -p=$NCORE \
		"$CLUSTERARG" ./set_compute_env.sh ./out.sh $PROGRAM $PROGRAMARGS;;
1)
	# xterm debug
	mpcrun $VERBOSE -c=1 --net=lcp -m=pthread -n=$NNODE -N=$NPROC -p=$NCORE \
		"$CLUSTERARG" xterm -e ./set_compute_env.sh gdb --command=./gdbscript.gdb -e \
		$PROGRAM $PROGRAMARGS;;
2)
	# valgrind
	mpcrun $VERBOSE -c=1 --net=lcp -m=pthread -n=$NNODE -N=$NPROC -p=$NCORE  \
		"$CLUSTERARG" ./set_compute_env.sh valgrind --log-file="vg-${FOUTPUT}-%p.out" \
		--leak-check=full --show-leak-kinds=all --tool=memcheck --leak-check=yes \
		$PROGRAM $PROGRAMARGS;;
3)
	# perf 
	mpcrun $VERBOSE -c=1 --net=lcp -m=pthread -n=$NNODE -N=$NPROC -p=$NCORE  \
		"$CLUSTERARG" ./set_compute_env.sh valgrind --tool=callgrind \
		--log-file="cg-${FOUTPUT}-%p.out" $PROGRAM $PROGRAMARGS;;
*)
	echo "Unknown run type";;
esac
