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

# Running from $SCRIPTPATH

FORTRAN_GEN_DIR="$SCRIPTPATH/fortran_gen"
MPI_H_PATH="$SCRIPTPATH/../MPC_MPI/include/mpc_mpi.h"

# Installing Fortran environment

cp -r $FORTRAN_GEN_DIR $SHARE_PREFIX/
cp -r $MPI_H_PATH $SHARE_PREFIX/fortran_gen/

# Installing Compiler Manager

install -m 0755 -D ${BUILD_BIN_PREFIX}/mpc_compiler_manager.sh ${BIN_PREFIX}/mpc_compiler_manager
COMPILER_MANAGER="${BIN_PREFIX}/mpc_compiler_manager"

# Installing Compiler Flags

echo "CP mpc_cflags"
install -m 0755 -D ${BUILD_BIN_PREFIX}/mpc_cflags ${BIN_PREFIX}/mpc_cflags
echo "CP mpc_ldflags"
install -m 0755 -D ${BUILD_BIN_PREFIX}/mpc_ldflags ${BIN_PREFIX}/mpc_ldflags

# Installing Compiler Wrappers

echo "CP mpc_cc"
install -m 0755 -D ${BUILD_BIN_PREFIX}/mpc_cc.sh ${BIN_PREFIX}/mpc_cc
echo "CP mpc_cxx"
install -m 0755 -D ${BUILD_BIN_PREFIX}/mpc_cxx.sh ${BIN_PREFIX}/mpc_cxx

for fc in mpc_f77 mpc_f90 mpc_f08
do
        echo "CP ${fc}"
        install -m 0755 -D ${BUILD_BIN_PREFIX}/mpc_f77.sh ${BIN_PREFIX}/${fc}
done

# Registering Compiler Alternatives


printf_per_compiler()
{
cat <<EOF > $1
#!/bin/sh

# This is the per compiler wrapper it simply forces
# the compilert to choose note that this is linked
# to the compiler manager order so that if it is
# GCC the first GCC will be chosen and so on

# Now invoke the compiler
$2 -cc=$3 "\$@"
EOF
}

for c in $C_COMPILER_LIST
do
	echo "REG $c C compiler";
	${COMPILER_MANAGER} --global add c $c;

        if test "x$?" = "x0"; then
                echo "GEN mpc_$c"
                printf_per_compiler ${BIN_PREFIX}/mpc_$c ${BIN_PREFIX}/mpc_cc $c
                chmod +x ${BIN_PREFIX}/mpc_$c
        fi
done

for c in $CXX_COMPILER_LIST
do
	echo "REG $c CXX compiler";
	${COMPILER_MANAGER} --global add cxx $c;

        if test "x$?" = "x0"; then
                echo "GEN mpc_$c"
                printf_per_compiler ${BIN_PREFIX}/mpc_$c ${BIN_PREFIX}/mpc_cxx $c
                chmod +x ${BIN_PREFIX}/mpc_$c
        fi
done

for c in $FC_COMPILER_LIST
do
	echo "REG $c FC compiler";
	${COMPILER_MANAGER} --global add fortran $c;

        if test "x$?" = "x0"; then
                echo "GEN mpc_$c"
                printf_per_compiler ${BIN_PREFIX}/mpc_$c ${BIN_PREFIX}/mpc_f77 $c
                chmod +x ${BIN_PREFIX}/mpc_$c
        fi
done

# Generating MPI standard compilers

printf_mpi_compiler()
{
cat <<EOF > $1
#!/bin/sh

# This is the MPI class compiler proxy
# to the MPC one, just passes the arguments
# through however note that by default
# this compiler does not privatize
# you can export MPI_PRIV to do it from here
# or just use their mpc_* counterpart

MPI_PRIV_ARG="-fnompc-privatize"

if test ! -z "\$MPI_PRIV"; then
	MPI_PRIV_ARG=""
fi

# Now invoke the compiler
$2 \$MPI_PRIV_ARG "\$@"
EOF
}

for i in cc cxx f77 f90 f08; do
        echo "GEN mpi$i"
	printf_mpi_compiler ${BIN_PREFIX}/mpi$i ${BIN_PREFIX}/mpc_$i
        chmod +x ${BIN_PREFIX}/mpi$i
done

# Point C++ to CXX
printf_mpi_compiler ${BIN_PREFIX}/mpic++ ${BIN_PREFIX}/mpc_cxx
chmod +x ${BIN_PREFIX}/mpic++