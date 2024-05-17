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

AM_CPPFLAGS = -I$(abs_top_srcdir)/src/MPC_Common/include/ -I$(abs_top_srcdir)/src/include/ -I$(abs_top_builddir)/src/MPC_Arch/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Arch/include/ -I$(abs_top_srcdir)/src/MPC_IO/include/ -I$(abs_top_srcdir)/src/MPC_MPI/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Lowcomm/include/ -I$(abs_top_srcdir)/src/MPC_Threads/include/ -I$(abs_top_builddir)/src/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Launch/include/ -I$(abs_top_srcdir)/src/MPC_Topology/include/ -I$(abs_top_builddir)/src/MPC_Common/include/
AM_CPPFLAGS += -I$(abs_top_builddir)/src/MPC_IO/include/ -I$(abs_top_srcdir)/src/MPC_Config/include/

if MPC_BUILD_OPENPA
AM_CPPFLAGS += -I$(abs_top_srcdir)/contrib/openpa/src -I$(abs_top_builddir)/contrib/openpa/src
else
AM_CPPFLAGS+=$(OPA_CFLAGS)
endif

if MPC_FORTRAN_ENABLED
     AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Fortran/include/
endif


SUBDIRS=romio

lib_LTLIBRARIES = libmpcio.la

libmpcio_la_SOURCES = mpl.c mpir_ext.c

libmpcio_la_LIBADD = ../../MPC_MPI/src/libmpcmpi.la ./romio/libromio.la  ./romio/libpromio.la
