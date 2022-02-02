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
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Arch/include/ -I$(abs_top_srcdir)/src/MPC_Threads/include/ -I$(abs_top_builddir)/src/MPC_Common/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Config/include/ -I$(abs_top_srcdir)/src/MPC_Config/src/generated/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Topology/include/ -I$(abs_top_srcdir)/src/MPC_Lowcomm/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Launch/include/ -I$(abs_top_builddir)/src/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Threads/src/ -I$(abs_top_srcdir)/src/MPC_Threads/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_MPI/src/ -I$(abs_top_srcdir)/src/MPC_MPI/include/

AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_OpenMP/include/ $(VMEM_CFLAGS) $(OPA_CFLAGS)

tools_inc = -I$(abs_top_srcdir)/src/MPC_OpenMP/src/tools/src/ \
            -I$(abs_top_srcdir)/src/MPC_OpenMP/src/tools/callback/ \
            -I$(abs_top_srcdir)/src/MPC_OpenMP/src/tools/trace/

AM_CPPFLAGS += $(tools_inc)

lib_LTLIBRARIES = libmpcomp.la

libmpcomp_la_SOURCES = mpcomp_api.c mpcomp_core.c mpcomp_fortran.c mpcomp_loop.c
libmpcomp_la_SOURCES += mpcomp_parallel_region.c mpcomp_places_env.c mpcomp_spinning_core.c
libmpcomp_la_SOURCES += mpcomp_sync.c mpcomp_task.c
libmpcomp_la_SOURCES += mpcomp_tree.c mpcomp_alloc.c omp_gomp.c omp_intel_atomics.c omp_intel.c
libmpcomp_la_SOURCES += mpcomp_task_trace.c mpcomp_callback.c

tools_src = tools/src/mpcompt_entryPoints.c tools/src/mpcompt_init.c tools/src/mpcompt_instance.c \
            tools/callback/mpcompt_cancel.c tools/callback/mpcompt_control_tool.c \
            tools/callback/mpcompt_device.c tools/callback/mpcompt_dispatch.c \
            tools/callback/mpcompt_flush.c tools/callback/mpcompt_mutex.c \
            tools/callback/mpcompt_parallel.c tools/callback/mpcompt_sync.c \
            tools/callback/mpcompt_target.c tools/callback/mpcompt_task.c \
            tools/callback/mpcompt_thread.c tools/callback/mpcompt_workShare.c

libmpcomp_la_SOURCES += $(tools_src)

libmpcomp_la_LDFLAGS = -Wl,--version-script=$(abs_top_srcdir)/src/MPC_OpenMP/version_gomp.txt
libmpcomp_la_LIBADD = ../../MPC_Launch/src/libmpclaunch.la $(VMEM_LIBS) $(OPA_LIBS)

lib_LTLIBRARIES += libiomp5.la libgomp.la

libiomp5_la_SOURCES = ./fakelib/fakelib.c
libgomp_la_SOURCES = ./fakelib/fakelib.c

