mpc_headerdir=$(includedir)/mpc/
mpc_header_HEADERS=$(abs_top_builddir)/mpc_config.h $(abs_top_srcdir)/include/mpc.h $(abs_top_srcdir)/include/mpc_main.h

SUBDIRS=MPC_Arch MPC_Common MPC_Config MPC_Topology

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

SUBDIRS += bin

if CAN_GENERATE_CONFIG
# Generate and copy back to sources to ensure transparent update on success
config:
	$(abs_top_srcdir)/MPC_Config/generators/mpc_gen_runtime_config $(abs_top_srcdir) $(abs_top_srcdir)/MPC_Config/generators/ $(abs_top_srcdir)/MPC_Config/src/generated/
else
config:
	@echo "ERROR: Both 'xsltproc' and 'xmllint' are needed to generate the configuration"
	@echo "ERROR: Install them and reconfigure the project"
endif


ALL_MPC_LIBS=./MPC_Config/src/libmpcconfig.la

if MPC_THREAD_ENABLED
ALL_MPC_LIBS+=./MPC_Launch/src/libmpclaunch.la
ALL_MPC_LIBS+=./MPC_Threads/src/libmpcthread.la
endif

if MPC_OPENMP_ENABLED
ALL_MPC_LIBS+=./MPC_OpenMP/src/libmpcomp.la
endif

if MPC_MESSAGE_PASSING_ENABLED
ALL_MPC_LIBS+=./MPC_Message_Passing/src/libmpclowcomm.la
endif

if MPC_MPI_ENABLED
ALL_MPC_LIBS+=./MPC_MPI/src/libmpcmpi.la
endif

#
# Define the MPC Library
#

lib_LTLIBRARIES = libmpcframework.la

libmpcframework_la_LIBADD =  $(ALL_MPC_LIBS)

libmpcframework_la_SOURCES=


#
# Print Config Program
#

AM_CPPFLAGS = -I$(abs_top_srcdir)/MPC_Common/include/ -I$(abs_top_srcdir)/include/ -I$(abs_top_srcdir)/MPC_Config/include/
AM_CPPFLAGS += -I$(abs_top_builddir)/MPC_Arch/include/  -I$(abs_top_srcdir)/MPC_Arch/include/ $(XML2_CFLAGS)
AM_CPPFLAGS += -I$(abs_top_srcdir)/MPC_Config/include/ -I$(abs_top_srcdir)/MPC_Config/src/ -I$(abs_top_srcdir)/MPC_Config/src/generated/


bin_PROGRAMS = mpc_print_config

mpc_print_config_SOURCES = ./MPC_Config/src/print_config/mpc_print_config.c ./MPC_Config/src/print_config/mpc_print_config_sh.c ./MPC_Config/src/print_config/mpc_print_config_sh.h
mpc_print_config_SOURCES += ./MPC_Config/src/print_config/mpc_print_config_xml.c ./MPC_Config/src/print_config/mpc_print_config_xml.h

mpc_print_config_LDADD = $(XML2_LIBS)

mpc_print_config_LDADD += $(ALL_MPC_LIBS)
