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
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Topology/include/ -I$(abs_top_builddir)/src/include/

if MPC_LOWCOMM_ENABLED
        AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Lowcomm/src/ -I$(abs_top_srcdir)/src/MPC_Lowcomm/include/
endif

if MPC_MPI_ENABLED
        AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_MPI/include/ -I$(abs_top_srcdir)/src/MPC_MPI/src/
        AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Lowcomm/include/
endif

if MPC_OPENMP_ENABLED
        AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_OpenMP/include/ \
                       -I$(abs_top_srcdir)/src/MPC_OpenMP/src/ \
                       -I$(abs_top_srcdir)/src/MPC_OpenMP/src/tools/src/ \
                       -I$(abs_top_srcdir)/src/MPC_OpenMP/src/tools/callback \
                       -I$(abs_top_srcdir)/src/MPC_OpenMP/src/tools/trace
endif

lib_LTLIBRARIES = libmpcthread.la

libmpcthread_la_SOURCES = sctk_context.c futex.c thread_ptr.c thread.c tls.c
libmpcthread_la_SOURCES += kthread.c microthread.c romio_ctx.c
libmpcthread_la_SOURCES += pthread_engine.c
libmpcthread_la_SOURCES += ng_engine.c ng_engine_scheduler.c
libmpcthread_la_SOURCES += ethread_engine.c ethread_mxn_engine.c ethread_posix.c

libmpcthread_la_SOURCES += accelerator.c

if MPC_CUDA_ENABLED

libmpcthread_la_SOURCES +=  ./accelerator/cuda.c ./accelerator/cuda_wrap.c

lib_LTLIBRARIES += libmpc_cuda_addon.la

libmpc_cuda_addon_la_SOURCES = ./accelerator/cuda_lib/cuda_lib.c
libmpc_cuda_addon_la_LIBADD = -lcuda

endif


libmpcthread_la_LIBADD = ../../MPC_Launch/src/libmpclaunch.la  -lpthread
