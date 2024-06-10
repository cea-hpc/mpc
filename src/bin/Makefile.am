#
# Compiler Setup
#

bin_SCRIPTS = mpc_cc mpc_cxx mpc_cflags mpc_ldflags

if MPC_MPI_ENABLED
  bin_SCRIPTS += mpicc mpicxx mpic++
endif

if MPC_FORTRAN_ENABLED
  bin_SCRIPTS += mpc_f77

if MPC_MPI_ENABLED
    bin_SCRIPTS += mpif77 mpif90
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

bin_PROGRAMS=

if MPC_LOWCOMM_ENABLED

bin_PROGRAMS+=mpc_sets

mpc_sets_SOURCES= ./mpc_sets.c
mpc_sets_CFLAGS= -I$(abs_top_srcdir)/src/MPC_Lowcomm/include/ -I$(abs_top_srcdir)/src/MPC_Common/include/ -I$(abs_top_builddir)/src/MPC_Common/include/ -I$(abs_top_srcdir)/src/include/  -I$(abs_top_builddir)/src/include/ -I$(abs_top_srcdir)/src/MPC_MPI/include/
mpc_sets_LDADD = $(ALLOC_LIBS)
mpc_sets_LDADD += @ALL_MPC_LIBS@

endif

#
# mpirun
#



if PRRTE_LAUNCHER_ENABLED
install-exec-hook:
	$(MKDIR_P) $(prefix)/bin
	rm -fr  $(prefix)/bin/mpirun; $(LN_S) @PRTERUN@ $(prefix)/bin/mpirun
else
bin_SCRIPTS += mpirun
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
