AM_CPPFLAGS = -I$(abs_top_srcdir)/MPC_Common/include/ -I$(abs_top_srcdir)/include/ -I$(abs_top_srcdir)/MPC_Config/include/
AM_CPPFLAGS += -I$(abs_top_builddir)/MPC_Arch/include/  -I$(abs_top_srcdir)/MPC_Arch/include/ $(XML2_CFLAGS)

#
# MPC_Config Library
#

mpc_headerdir=$(includedir)/mpc/

lib_LTLIBRARIES = libmpcconfig.la

libmpcconfig_la_SOURCES = libxml_helper.c runtime_config_mapper.c runtime_config_selectors.c runtime_config_validation.c
libmpcconfig_la_SOURCES += runtime_config.c runtime_config_printer.c runtime_config_sources.c runtime_config_walk.c

AM_CPPFLAGS += -I$(abs_top_srcdir)/MPC_Config/src/generated/

# Use pre-generated files
mpc_header_HEADERS= ./generated/mpc_config_struct_defaults.h ./generated/mpc_config_struct.h

libmpcconfig_la_SOURCES += ./generated/mpc_config_struct_defaults.c
libmpcconfig_la_SOURCES += ./generated/mpc_config_struct_meta.c


libmpcconfig_la_LIBADD = $(abs_top_builddir)/MPC_Common/src/libmpccommon.la $(XML2_LIBS)

# Default configuration file and schema installation
install-data-local:
	install -m 0644 -D $(abs_top_srcdir)/MPC_Config/extra/config.xml.example $(pkgdatadir)/config.xml
	install -m 0644 -D $(abs_top_srcdir)/MPC_Config/src/generated/mpc-config.xsd $(pkgdatadir)/mpc-config.xsd

#
# Print Config Program
#

bin_PROGRAMS = mpc_print_config

mpc_print_config_SOURCES = ./print_config/mpc_print_config.c ./print_config/mpc_print_config_sh.c ./print_config/mpc_print_config_sh.h
mpc_print_config_SOURCES += ./print_config/mpc_print_config_xml.c ./print_config/mpc_print_config_xml.h
mpc_print_config_LDADD = ../../MPC_Common/src/libmpccommon.la ./libmpcconfig.la $(XML2_LIBS)