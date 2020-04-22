MPC_VERSION_MAJOR=@MPC_VERSION_MAJOR@
MPC_VERSION_MINOR=@MPC_VERSION_MINOR@
MPC_VERSION_PATCH=@MPC_VERSION_PATCH@

MPC_VERSION="${MPC_VERSION_MAJOR}.${MPC_VERSION_MINOR}.${MPC_VERSION_PATCH}"

PACKAGE_SRING="@PACKAGE_STRING@"

if CAN_USE_PANDOC
all:
	@$(abs_top_srcdir)/src/MPC_Doc/build_manpage.sh "$(MPC_VERSION)"
endif

install-data-local:
	@$(abs_top_srcdir)/src/MPC_Doc/install_all_manpages.sh "$(MPC_VERSION)" "$PACKAGE_SRING" "$(mandir)"