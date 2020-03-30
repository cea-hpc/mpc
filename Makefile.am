SUBDIRS=src

lint:
	$(abs_top_srcdir)/utils/lint.sh $(abs_top_builddir)

#
# Config generation helper
#

if CAN_GENERATE_CONFIG
# Generate and copy back to sources to ensure transparent update on success
config:
	$(abs_top_srcdir)/src/MPC_Config/generators/mpc_gen_runtime_config $(abs_top_srcdir)/src $(abs_top_srcdir)/src/MPC_Config/generators/ $(abs_top_srcdir)/src/MPC_Config/src/generated/
else
config:
	@echo "ERROR: Both 'xsltproc' and 'xmllint' are needed to generate the configuration"
	@echo "ERROR: Install them and reconfigure the project"
endif
