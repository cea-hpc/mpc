MPC_C_COMPILER_LIST=@MPC_C_COMPILER_LIST@
MPC_CPP_COMPILER_LIST=@MPC_CPP_COMPILER_LIST@
MPC_FORTRAN_COMPILER_LIST=@MPC_FORTRAN_COMPILER_LIST@

define Register_C_compiler
	echo "Registering $(1) C compiler";
	$(bindir)/mpc_compiler_manager --global add c $(1);
endef

define Register_CPP_compiler
	echo "Registering $(1) C compiler";
	$(bindir)/mpc_compiler_manager --global add cxx $(1);
endef

define Register_FORTRAN_compiler
	echo "Registering $(1) C compiler";
	$(bindir)/mpc_compiler_manager --global add fortran $(1);
endef

# Default configuration file and schema installation
install-data-local:
	cp -r $(abs_top_srcdir)/bin/fortran_gen $(pkgdatadir)/
	cp -r $(abs_top_srcdir)/MPC_MPI/include/mpc_mpi.h $(pkgdatadir)/fortran_gen/
	install -m 0755 -D $(abs_top_builddir)/bin/mpc_compiler_manager.sh $(bindir)/mpc_compiler_manager
	$(foreach ccomp,$(MPC_C_COMPILER_LIST), $(call Register_C_compiler, $(ccomp)) )
	$(foreach cppcomp,$(MPC_CPP_COMPILER_LIST), $(call Register_CPP_compiler, $(cppcomp)) )
	$(foreach fortrancomp,$(MPC_FORTRAN_COMPILER_LIST), $(call Register_FORTRAN_compiler, $(fortrancomp)) )
	install -m 0755 -D $(abs_top_builddir)/bin/mpc_cflags $(bindir)/mpc_cflags
	install -m 0755 -D $(abs_top_builddir)/bin/mpc_ldflags $(bindir)/mpc_ldflags
	install -m 0755 -D $(abs_top_builddir)/bin/mpc_cc.sh $(bindir)/mpc_cc
	install -m 0755 -D $(abs_top_builddir)/bin/mpc_cxx.sh $(bindir)/mpc_cxx
	install -m 0755 -D $(abs_top_builddir)/bin/mpc_f77.sh $(bindir)/mpc_f77
	install -m 0755 -D $(abs_top_builddir)/bin/mpc_f77.sh $(bindir)/mpc_f90
	install -m 0755 -D $(abs_top_builddir)/bin/mpc_f77.sh $(bindir)/mpc_f08
	cp -r $(abs_top_srcdir)/bin/mpcrun_opt $(pkgdatadir)/
	install -m 0755 -D $(abs_top_builddir)/bin/mpcrun.sh $(bindir)/mpcrun


