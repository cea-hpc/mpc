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

if MPC_FORTRAN_ENABLED
        SUBDIRS += MPC_Fortran
endif

if MPC_MPIIO_ENABLED
        SUBDIRS += MPC_IO
endif

# Bin must be the last as it depends on libmpcframework.so being
# installed for Fortran generation
SUBDIRS += lib bin


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
