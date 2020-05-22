############################# MPC License ##############################
# Wed Nov 19 15:19:19 CET 2008                                         #
# Copyright or (C) or Copr. Commissariat a l'Energie Atomique          #
#                                                                      #
# IDDN.FR.001.230040.000.S.P.2007.000.10000                            #
# This file is part of the MPC Runtime.                                #
#                                                                      #
# This software is governed by the CeCILL-C license under French law   #
# and abiding by the rules of distribution of free software.  You can  #
# use, modify and/ or redistribute the software under the terms of     #
# the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     #
# following URL http://www.cecill.info.                                #
#                                                                      #
# The fact that you are presently reading this means that you have     #
# had knowledge of the CeCILL-C license and that you accept its        #
# terms.                                                               #
#                                                                      #
# Authors:                                                             #
#   - CARRIBAULT Patrick patrick.carribault@cea.fr                     #
#   - PERACHE Marc marc.perache@cea.fr                                 #
#   - JAEGER Julien julien.jaeger@cea.fr                               #
#                                                                      #
########################################################################
AM_CPPFLAGS = -I$(abs_top_srcdir)/src/MPC_Common/include/ -I$(abs_top_srcdir)/src/include/ -I$(abs_top_srcdir)/src/MPC_Config/include/ -I$(abs_top_builddir)/src/MPC_Common/include/
AM_CPPFLAGS += -I$(abs_top_builddir)/src/MPC_Arch/include/  -I$(abs_top_srcdir)/src/MPC_Arch/include/ $(XML2_CFLAGS)


if MPC_MPI_ENABLED
        AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_MPI/include/ -I$(abs_top_srcdir)/src/MPC_MPI/src/
        AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Lowcomm/include/ 
endif


#
# MPC_Config Library
#

lib_LTLIBRARIES = libmpcconfig.la

libmpcconfig_la_SOURCES = libxml_helper.c runtime_config_mapper.c runtime_config_selectors.c runtime_config_validation.c
libmpcconfig_la_SOURCES += runtime_config.c runtime_config_printer.c runtime_config_sources.c runtime_config_walk.c

AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Config/src/generated/

# Use pre-generated files
noinst_HEADERS= ./generated/mpc_config_struct_defaults.h ./generated/mpc_config_struct.h

libmpcconfig_la_SOURCES += ./generated/mpc_config_struct_defaults.c
libmpcconfig_la_SOURCES += ./generated/mpc_config_struct_meta.c


libmpcconfig_la_LIBADD = $(abs_top_builddir)/src/MPC_Common/src/libmpccommon.la $(XML2_LIBS) -ldl
