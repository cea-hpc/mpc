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
AM_CPPFLAGS = -I$(abs_top_srcdir)/src/MPC_Common/include/ -I$(abs_top_srcdir)/src/include/ -I$(abs_top_builddir)/src/MPC_Arch/include/ -I$(abs_top_builddir)/src/MPC_Common/include/ -I$(abs_top_srcdir)/src/MPC_Config/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Arch/include/ -I$(abs_top_builddir)/src/include/ $(UNW_CFLAGS)

if MPC_BUILD_OPENPA
AM_CPPFLAGS += -I$(abs_top_srcdir)/contrib/openpa/src -I$(abs_top_builddir)/contrib/openpa/src
else
AM_CPPFLAGS+=$(OPA_CFLAGS)
endif

if MPC_THREAD_ENABLED
        AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Threads/src/ -I$(abs_top_srcdir)/src/MPC_Threads/include/ -I$(abs_top_srcdir)/src/MPC_Topology/include/
endif

noinst_LTLIBRARIES = libmpccommon.la

elf_64.c: elf_32.c
	$(abs_srcdir)/gen_elf64

CLEANFILES=elf_64.c

libmpccommon_la_SOURCES = aio.c bit_array.c fifo.c hashtable.c io_helper.c rank.c debug.c sctk_no_alloc.c sctk_pm_json.c flags.c debugger.c elf_32.c elf_64.c  spinlock.c base64.c recycler.c mpc_mempool.c


if MPC_PROFILER_ENABLED
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_MPI/src/profiler/

libmpccommon_la_SOURCES += ./profiler/sctk_internal_profiler.c ./profiler/sctk_performance_tree.c
libmpccommon_la_SOURCES += ./profiler/sctk_profile_meta.c ./profiler/sctk_profiler_array.c
libmpccommon_la_SOURCES += ./profiler/sctk_profile_render.c ./profiler/sctk_profile_render_html.c
libmpccommon_la_SOURCES += ./profiler/sctk_profile_render_tex.c ./profiler/sctk_profile_render_text.c
libmpccommon_la_SOURCES += ./profiler/sctk_profile_render_xml.c
endif

libmpccommon_la_LIBADD = $(abs_top_builddir)/src/MPC_Arch/src/libmpcarch.la $(ALLOC_LIBS) $(UNW_LIBS)

if MPC_BUILD_OPENPA
libmpccommon_la_LIBADD += $(abs_top_builddir)/contrib/openpa/src/libopa.la
else
libmpccommon_la_LIBADD += $(OPA_LIBS)
endif
