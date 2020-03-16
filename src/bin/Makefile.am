#
# Compiler Setup
#

MPC_C_COMPILER_LIST=@MPC_C_COMPILER_LIST@
MPC_CPP_COMPILER_LIST=@MPC_CPP_COMPILER_LIST@
MPC_FORTRAN_COMPILER_LIST=@MPC_FORTRAN_COMPILER_LIST@

install_build_env:
	install -m 0755 -D $(abs_top_builddir)/src/bin/mpc_build_env.sh $(bindir)/mpc_build_env.sh


install_compilers: install_build_env
	$(abs_top_srcdir)/src/bin/deploy_compilers.sh "$(prefix)" "$(abs_top_builddir)/src" "$(MPC_C_COMPILER_LIST)" "$(MPC_CPP_COMPILER_LIST)" "$(MPC_FORTRAN_COMPILER_LIST)"

if MPC_HAVE_AUTOPRIV
deploy_compilers: install_compilers
	$(prefix)/bin/mpc_compiler_manager --global set_default c apcc
	$(prefix)/bin/mpc_compiler_manager --global set_default cxx ap++
	$(prefix)/bin/mpc_compiler_manager --global set_default fortran apfortran
else
deploy_compilers: install_compilers
endif

#
# mpcrun setup
#

deploy_mpcrun:
	cp -r $(abs_top_srcdir)/src/bin/mpcrun_opt $(pkgdatadir)/
	install -m 0755 -D $(abs_top_builddir)/src/bin/mpcrun.sh $(bindir)/mpcrun
	install -m 0755 -D $(abs_top_builddir)/src/bin/mpc_status.sh $(bindir)/mpc_status
	install -m 0755 -D $(abs_top_builddir)/src/bin/mpcvars.sh $(bindir)/mpcvars.sh
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
	install -m 0755 -D $(abs_top_builddir)/src/bin/mpc_nvcc.sh $(bindir)/mpc_nvcc
else
deploy_mpc_nvcc:
	@echo ""
endif

#
# All install time targets
#

install-data-local: install_build_env deploy_compilers deploy_mpcrun deploy_mpc_nvcc
