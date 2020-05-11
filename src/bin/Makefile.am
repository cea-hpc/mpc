#
# Compiler Setup
#

bin_SCRIPTS = mpc_cc mpc_cxx mpc_cflags mpc_ldflags

if MPC_FORTRAN_ENABLED
  bin_SCRIPTS += mpc_f77
endif

#
# Other scripts
#
bin_SCRIPTS += mpc_status mpcrun mpcvars.sh
pkgdata_SCRIPTS = mpc_compiler_common.sh

#
# Cuda support
#

if MPC_CUDA_ENABLED
   bin_SCRIPTS += mpc_nvcc
endif

CLEANFILES = $(bin_SCRIPTS) $(pkgdata_SCRIPTS)

#
# mpirun
#

bin_PROGRAMS=mpirun
mpirun_SOURCES=./mpirun.c

