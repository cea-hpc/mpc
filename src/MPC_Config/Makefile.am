mpc_headerdir=$(includedir)/mpc/
mpc_header_HEADERS=./include/mpc_runtime_config.h

# Editor installation

install-data-local:
	cp -r $(abs_top_srcdir)/src/MPC_Config/editor $(pkgdatadir)
	install -m 0644 -D $(abs_top_srcdir)/src/MPC_Config/src/generated/mpc_config_meta.js $(pkgdatadir)/editor/javascript/
	install -m 0644 -D $(abs_top_srcdir)/src/MPC_Config/src/generated/mpc_config_xsd.js $(pkgdatadir)/editor/javascript/


SUBDIRS=src
