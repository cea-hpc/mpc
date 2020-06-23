#
# Compiler Setup
#

bin_SCRIPTS = mpc_cc mpc_cxx mpc_cflags mpc_ldflags mpicc mpicxx

if MPC_FORTRAN_ENABLED
  bin_SCRIPTS += mpc_f77 mpif77
endif

#
# Other scripts
#
bin_SCRIPTS += mpc_status mpcvars.sh
pkgdata_SCRIPTS = mpc_compiler_common.sh

# Make sure the source script is at the root of the prefix for retro-compatibility
install-data-local:
	ln -sf $(bindir)/mpcvars.sh $(prefix)/mpcvars.sh

uninstall-local:
	rm -f $(prefix)/mpcvars.sh

#
# Cuda support
#

if MPC_CUDA_ENABLED
   bin_SCRIPTS += mpc_nvcc
endif

#
# mpirun
#

bin_PROGRAMS=mpirun
mpirun_SOURCES=./mpirun.c

#
# mpcrun
#
bin_SCRIPTS += mpcrun

mpcrun_plugindir = $(pkgdatadir)/mpcrun_opt/
mpcrun_plugin_SCRIPTS = mpcrun_opt/mpcrun_default mpcrun_opt/mpcrun_hydra mpcrun_opt/mpcrun_mpiexec mpcrun_opt/mpcrun_mpiexec_allinea mpcrun_opt/mpcrun_mpiexec_gdb mpcrun_opt/mpcrun_none mpcrun_opt/mpcrun_none_ddd mpcrun_opt/mpcrun_none_gdb mpcrun_opt/mpcrun_none_mpc-gdb mpcrun_opt/mpcrun_none_totalview mpcrun_opt/mpcrun_none_valgrind mpcrun_opt/mpcrun_propagate_env mpcrun_opt/mpcrun_remote mpcrun_opt/mpcrun_remote_launcher mpcrun_opt/mpcrun_salloc_hydra mpcrun_opt/mpcrun_srun mpcrun_opt/mpcrun_srun_gdb mpcrun_opt/mpcrun_srun_mpcdb mpcrun_opt/mpcrun_srun_totalview

#CLEANFILES += $(bin_SCRIPTS) $(mpcrun_plugin_SCRIPTS)
