#!/bin/sh

FORTRAN_COMPILERS=$1
MPC_CFLAGS_BIN=$2
MPC_COMPILE_MOD_BIN=$3

#Extract compiler configuration and flags
# - MPC_INSTALL_PREFIX
# - MPC_SHARE_DIR
# - MPC_DEFAULT_FC_COMPILER
# - MPC_DEFAULT_C_COMPILER
# - MPC_GENERATION_DATE
# - CFLAGS
eval $(sh "$MPC_CFLAGS_BIN" -sh -cc)

echo "C: $MPC_DEFAULT_C_COMPILER   FC: $FORTRAN_COMPILERS"

for fc in $FORTRAN_COMPILERS
do
	if test "x$(basename $fc)" = "xapfortran"; then
		$MPC_COMPILE_MOD_BIN -cc="$MPC_DEFAULT_C_COMPILER" -fc="$fc" -cf="${MPC_CFLAGS_BIN}" --use-install --nopriv
		$MPC_COMPILE_MOD_BIN -cc="$MPC_DEFAULT_C_COMPILER" -fc="$fc" -cf="${MPC_CFLAGS_BIN}" --use-install
	else
		$MPC_COMPILE_MOD_BIN -cc="$MPC_DEFAULT_C_COMPILER" -fc="$fc" -cf="${MPC_CFLAGS_BIN}" --use-install --nopriv
	fi
done