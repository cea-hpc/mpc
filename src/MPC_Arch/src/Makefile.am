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
AM_CPPFLAGS=-I$(abs_top_srcdir)/src/MPC_Arch/include/ -I$(abs_top_builddir)/src/MPC_Arch/include/ $(OPENPA_CFLAGS)

lib_LTLIBRARIES = libmpcarch.la

libmpcarch_la_SOURCES = ./pause.c ./timer.c

libmpcarch_la_LIBADD = $(OPENPA_LIBS)

# Optimized Context Switches

if MPC_ARCH_X86_64_CONTEXT_SWITCH
        libmpcarch_la_SOURCES += ./context/x86_64/getcontext.S ./context/x86_64/makecontext.c
        libmpcarch_la_SOURCES += ./context/x86_64/setcontext.S ./context/x86_64/startcontext.S ./context/x86_64/swapcontext.S
endif

# Microstask Support

if MPC_ARCH_X86_64_MICROTASK
        libmpcarch_la_SOURCES += ./microtask/x86_64/z_Linux_asm.S
endif

if MPC_ARCH_GENERIC_MICROTASK
        libmpcarch_la_SOURCES += ./microtask/generic/microtask.c
endif
