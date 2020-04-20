#
# Compiler Setup
#

MPC_C_COMPILER_LIST=@MPC_C_COMPILER_LIST@
MPC_CPP_COMPILER_LIST=@MPC_CPP_COMPILER_LIST@
MPC_FORTRAN_COMPILER_LIST=@MPC_FORTRAN_COMPILER_LIST@
MPC_SHARE_DIR="@PKG_DATADIR@"

install_build_env:
	install -m 0755 -D $(abs_builddir)/mpc_build_env.sh $(bindir)/mpc_build_env.sh


install_compilers: install_build_env
	$(abs_srcdir)/deploy_compilers.sh "$(prefix)" "$(abs_top_builddir)/src" "$(MPC_SHARE_DIR)" "$(abs_top_srcdir)" "$(MPC_C_COMPILER_LIST)" "$(MPC_CPP_COMPILER_LIST)" "$(MPC_FORTRAN_COMPILER_LIST)"

#
# mpcrun setup
#

deploy_mpcrun:
	cp -r $(abs_srcdir)/mpcrun_opt $(pkgdatadir)/
	install -m 0755 -D $(abs_builddir)/mpcrun.sh $(bindir)/mpcrun
	install -m 0755 -D $(abs_builddir)/mpc_status.sh $(bindir)/mpc_status
	install -m 0755 -D $(abs_builddir)/mpcvars.sh $(bindir)/mpcvars.sh
	ln -sf $(bindir)/mpcvars.sh $(prefix)/mpcvars.sh

#
# mpirun
#

bin_PROGRAMS=mpirun
mpirun_SOURCES=./mpirun.c

#
# Cuda support
#

if MPC_CUDA_ENABLED
deploy_mpc_nvcc:
	install -m 0755 -D $(abs_srcdir)/mpc_nvcc.sh $(bindir)/mpc_nvcc
else
deploy_mpc_nvcc:
	@echo ""
endif

#
# All install time targets
#

install-data-local: install_build_env install_compilers deploy_mpcrun deploy_mpc_nvcc
