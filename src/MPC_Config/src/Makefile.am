AM_CPPFLAGS = -I$(abs_top_srcdir)/src/MPC_Common/include/ -I$(abs_top_srcdir)/src/include/ -I$(abs_top_srcdir)/src/MPC_Config/include/
AM_CPPFLAGS += -I$(abs_top_builddir)/src/MPC_Arch/include/  -I$(abs_top_srcdir)/src/MPC_Arch/include/ $(XML2_CFLAGS)


if MPC_MPI_ENABLED
        AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_MPI/include/ -I$(abs_top_srcdir)/src/MPC_MPI/src/
        AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Message_Passing/include/ 
endif


#
# MPC_Config Library
#

mpc_headerdir=$(pkgincludedir)

lib_LTLIBRARIES = libmpcconfig.la

libmpcconfig_la_SOURCES = libxml_helper.c runtime_config_mapper.c runtime_config_selectors.c runtime_config_validation.c
libmpcconfig_la_SOURCES += runtime_config.c runtime_config_printer.c runtime_config_sources.c runtime_config_walk.c

AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Config/src/generated/

# Use pre-generated files
mpc_header_HEADERS= ./generated/mpc_config_struct_defaults.h ./generated/mpc_config_struct.h

libmpcconfig_la_SOURCES += ./generated/mpc_config_struct_defaults.c
libmpcconfig_la_SOURCES += ./generated/mpc_config_struct_meta.c


libmpcconfig_la_LIBADD = $(abs_top_builddir)/src/MPC_Common/src/libmpccommon.la $(XML2_LIBS) -ldl


# Default configuration file and schema installation
install-data-local:
	install -m 0644 -D $(abs_top_srcdir)/src/MPC_Config/extra/config.xml.example $(pkgdatadir)/config.xml
	install -m 0644 -D $(abs_top_srcdir)/src/MPC_Config/src/generated/mpc-config.xsd $(pkgdatadir)/mpc-config.xsd

