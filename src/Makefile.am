SUBDIRS=MPC_Doc MPC_Arch MPC_Common MPC_Config MPC_Topology

if MPC_THREAD_ENABLED
        SUBDIRS += MPC_Launch
endif

if MPC_THREAD_ENABLED
        SUBDIRS += MPC_Threads
endif

if MPC_OPENMP_ENABLED
        SUBDIRS += MPC_OpenMP
endif

if MPC_MESSAGE_PASSING_ENABLED
        SUBDIRS += MPC_Message_Passing
endif

if MPC_MPI_ENABLED
        SUBDIRS += MPC_MPI
endif

# Bin must be the last as it depends on libmpcframework.so being
# installed for Fortran generation
SUBDIRS += lib bin

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

#
# Print Config Program
#

AM_CPPFLAGS = -I$(abs_top_srcdir)/src/MPC_Common/include/ -I$(abs_top_srcdir)/src/include/ -I$(abs_top_srcdir)/src/MPC_Config/include/
AM_CPPFLAGS += -I$(abs_top_builddir)/src/MPC_Arch/include/  -I$(abs_top_srcdir)/src/MPC_Arch/include/ $(XML2_CFLAGS)
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Config/include/ -I$(abs_top_srcdir)/src/MPC_Config/src/ -I$(abs_top_srcdir)/src/MPC_Config/src/generated/


bin_PROGRAMS = mpc_print_config

mpc_print_config_SOURCES = ./MPC_Config/src/print_config/mpc_print_config.c ./MPC_Config/src/print_config/mpc_print_config_sh.c ./MPC_Config/src/print_config/mpc_print_config_sh.h
mpc_print_config_SOURCES += ./MPC_Config/src/print_config/mpc_print_config_xml.c ./MPC_Config/src/print_config/mpc_print_config_xml.h

mpc_print_config_LDADD = $(XML2_LIBS)

mpc_print_config_LDADD += @ALL_MPC_LIBS@