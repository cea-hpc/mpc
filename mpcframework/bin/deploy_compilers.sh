#!/bin/sh

INSTALL_PREFIX="$1"
SHARE_PREFIX="$1/share/mpc/"
BIN_PREFIX="$1/bin/"
BUILD_PREFIX="$2"
BUILD_BIN_PREFIX="$2/bin"
C_COMPILER_LIST="$3"
CXX_COMPILER_LIST="$4"
FC_COMPILER_LIST="$5"

if test "x$#" != "x4"; then
        echo "$0 [PREFIX] [BUILD PREFIX] [C COMP] [CXX COMP] [FC COMP]"
fi

SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

echo "# Running from $SCRIPTPATH"

FORTRAN_GEN_DIR="$SCRIPTPATH/fortran_gen"
MPI_H_PATH="$SCRIPTPATH/../MPC_MPI/include/mpc_mpi.h"

echo "# Installing Fortran environment"

cp -r $FORTRAN_GEN_DIR $SHARE_PREFIX/
cp -r $MPI_H_PATH $SHARE_PREFIX/fortran_gen/

echo "# Installing Compiler Manager"

install -m 0755 -D ${BUILD_BIN_PREFIX}/mpc_compiler_manager.sh ${BIN_PREFIX}/mpc_compiler_manager
COMPILER_MANAGER="${BIN_PREFIX}/mpc_compiler_manager"

echo "# Installing Compiler Flags"

echo "CP mpc_cflags"
install -m 0755 -D ${BUILD_BIN_PREFIX}/mpc_cflags ${BIN_PREFIX}/mpc_cflags
echo "CP mpc_ldflags"
install -m 0755 -D ${BUILD_BIN_PREFIX}/mpc_ldflags ${BIN_PREFIX}/mpc_ldflags

echo "# Installing Compiler Wrappers"

echo "CP mpc_cc"
install -m 0755 -D ${BUILD_BIN_PREFIX}/mpc_cc.sh ${BIN_PREFIX}/mpc_cc
echo "CP mpc_cxx"
install -m 0755 -D ${BUILD_BIN_PREFIX}/mpc_cxx.sh ${BIN_PREFIX}/mpc_cxx

for fc in mpc_f77 mpc_f90 mpc_f08
do
        echo "CP ${fc}"
        install -m 0755 -D ${BUILD_BIN_PREFIX}/mpc_f77.sh ${BIN_PREFIX}/${fc}
done

echo "# Registering Compiler Alternatives"

for c in $C_COMPILER_LIST
do
	echo "REG $c C compiler";
	${COMPILER_MANAGER} --global add c $c;
done

for c in $CXX_COMPILER_LIST
do
	echo "REG $c CXX compiler";
	${COMPILER_MANAGER} --global add c $c;
done

for c in $FC_COMPILER_LIST
do
	echo "REG $c FC compiler";
	${COMPILER_MANAGER} --global add fortran $c;
done