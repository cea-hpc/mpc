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

deploy_compilers:
	$(abs_top_srcdir)/bin/deploy_compilers.sh "$(prefix)" "$(abs_top_builddir)" "$(MPC_C_COMPILER_LIST)" "$(MPC_CPP_COMPILER_LIST)" "$(MPC_FORTRAN_COMPILER_LIST)"


deploy_mpcrun:
	cp -r $(abs_top_srcdir)/bin/mpcrun_opt $(pkgdatadir)/
	install -m 0755 -D $(abs_top_builddir)/bin/mpcrun.sh $(bindir)/mpcrun

install-data-local: deploy_compilers deploy_mpcrun

