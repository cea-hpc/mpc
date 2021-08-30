#
# Compiler Setup
#

bin_SCRIPTS = mpc_cc mpc_cxx mpc_cflags mpc_ldflags

if MPC_MPI_ENABLED
  bin_SCRIPTS += mpicc mpicxx
endif

if MPC_FORTRAN_ENABLED
  bin_SCRIPTS += mpc_f77

if MPC_MPI_ENABLED
    bin_SCRIPTS += mpif77
endif

endif

#
# Other scripts
#
bin_SCRIPTS += mpc_status mpcvars.sh
pkgdata_SCRIPTS = mpc_compiler_common.sh

# It duplicates mpcvarsh.sh but is needed for retrocompat
prefix_bindir=$(prefix)
prefix_bin_SCRIPTS=mpcvars.sh

#
# Cuda support
#

if MPC_CUDA_ENABLED
   bin_SCRIPTS += mpc_nvcc
endif

#
# mpirun
#


if PRRTE_LAUNCHER_ENABLED
install-exec-hook:
	$(MKDIR_P) $(prefix)/bin
	rm -fr  $(prefix)/bin/mpirun; $(LN_S) @PRTERUN@ $(prefix)/bin/mpirun
else
bin_PROGRAMS=mpirun
mpirun_SOURCES=./mpirun.c
endif

#
# mpcrun
#
bin_SCRIPTS += mpcrun

mpcrun_plugindir = $(pkgdatadir)/mpcrun_opt/
mpcrun_plugin_SCRIPTS = mpcrun_opt/mpcrun_none mpcrun_opt/mpcrun_none_mpc-gdb

if MPC_HAS_SRUN_SUPPORT
mpcrun_plugin_SCRIPTS += mpcrun_opt/mpcrun_srun
endif

if HYDRA_LAUNCHER_ENABLED
mpcrun_plugin_SCRIPTS += mpcrun_opt/mpcrun_hydra mpcrun_opt/mpcrun_salloc_hydra 
endif

if PRRTE_LAUNCHER_ENABLED
mpcrun_plugin_SCRIPTS +=  mpcrun_opt/mpcrun_prrte
endif

#CLEANFILES += $(bin_SCRIPTS) $(mpcrun_plugin_SCRIPTS)
