MPC_VERSION_STRING=@MPC_VERSION_STRING@

if CAN_USE_PANDOC
all:
	@$(abs_top_srcdir)/MPC_Doc/build_manpage.sh "$(MPC_VERSION_STRING)"
endif

install-data-local:
	@$(abs_top_srcdir)/MPC_Doc/install_all_manpages.sh "$(MPC_VERSION_STRING)" "$(mandir)"