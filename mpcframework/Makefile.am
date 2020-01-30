mpc_headerdir=$(includedir)/mpc/
mpc_header_HEADERS=$(abs_top_builddir)/mpc_config.h $(abs_top_srcdir)/include/mpc.h  $(abs_top_srcdir)/include/mpc_keywords.h  $(abs_top_srcdir)/include/mpc_main.h

SUBDIRS=MPC_Arch MPC_Common MPC_Config MPC_Topology

if MPC_THREAD_ENABLED
        SUBDIRS += MPC_Threads
endif


if CAN_GENERATE_CONFIG
# Generate and copy back to sources to ensure transparent update on success
config:
	$(abs_top_srcdir)/MPC_Config/generators/mpc_gen_runtime_config $(abs_top_srcdir) $(abs_top_srcdir)/MPC_Config/generators/ $(abs_top_srcdir)/MPC_Config/src/generated/
else
config:
	@echo "ERROR: Both 'xsltproc' and 'xmllint' are needed to generate the configuration"
	@echo "ERROR: Install them and reconfigure the project"
endif


#
# Print Config Program
#

bin_PROGRAMS = mpc_print_config

mpc_print_config_SOURCES = ./MPC_Config/src/print_config/mpc_print_config.c ./MPC_Config/src/print_config/mpc_print_config_sh.c ./MPC_Config/src/print_config/mpc_print_config_sh.h
mpc_print_config_SOURCES += ./MPC_Config/src/print_config/mpc_print_config_xml.c ./MPC_Config/src/print_config/mpc_print_config_xml.h
mpc_print_config_LDADD = ./MPC_Common/src/libmpccommon.la .MPC_Config/MPC_Thread/libmpcthread.la $(XML2_LIBS)