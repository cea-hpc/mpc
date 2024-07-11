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
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Config/include/ -I$(abs_top_srcdir)/src/MPC_Config/src/generated/ -I$(abs_top_builddir)/src/MPC_IO/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Topology/include/ -I$(abs_top_srcdir)/src/MPC_Lowcomm/include/ -I$(abs_top_srcdir)/src/MPC_Lowcomm/src/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Launch/include/ -I$(abs_top_srcdir)/src/MPC_MPI/include/  -I$(abs_top_builddir)/src/include/

if MPC_THREAD_ENABLED
        AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Threads/src/ -I$(abs_top_srcdir)/src/MPC_Threads/include/ -I$(abs_top_srcdir)/src/MPC_Topology/include/
endif

if PORTALS4_DRIVER_ENABLED
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Lowcomm/src/portals/
endif

if OFI_DRIVER_ENABLED
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Lowcomm/src/ofi/
endif

if MPC_OPENMP_ENABLED
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_OpenMP/include/
endif

#Yes drivers are leaking out

AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Lowcomm/src/tcp/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Lowcomm/src/shm/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Lowcomm/src/lcr/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Lowcomm/src/lcp/

AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_MPI/src/osc/

lib_LTLIBRARIES = libmpcmpi.la

libmpcmpi_la_SOURCES = ./osc/osc_win.c ./osc/osc_active_target.c ./osc/osc_comm.c ./osc/osc_module.c ./osc/osc_passive_target.c ./osc/osc_win_attr.c ./osc/osc_common.c
libmpcmpi_la_SOURCES += comm_lib.c egreq_nbc.c mpc_extern32.c mpc_mpi_halo.c mpc_reduction.c mpit.c
libmpcmpi_la_SOURCES += datatype.c egreq_progress.c mpc_info.c mpc_nbc.c mpi_conf.c mpc_coll.c session.c errh.c mgroup.c
libmpcmpi_la_SOURCES += egreq_classes.c mpc_cs.c mpc_mpi.c mpc_nbc_progress_thread_binding.c 

if MPC_FORTRAN_ENABLED
   libmpcmpi_la_SOURCES += fortran.c
endif

# MPITypes
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_MPI/src/dataloop/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_MPI/src/mpitypes/

libmpcmpi_la_SOURCES += dataloop/contents_support.c\
			dataloop/darray_support.c\
			dataloop/dataloop.c\
			dataloop/dataloop.h\
			dataloop/dataloop_create.c\
			dataloop/dataloop_create.h\
			dataloop/dataloop_create_blockindexed.c\
			dataloop/dataloop_create_contig.c\
			dataloop/dataloop_create_indexed.c\
			dataloop/dataloop_create_pairtype.c\
			dataloop/dataloop_create_struct.c\
			dataloop/dataloop_create_vector.c\
			dataloop/dataloop_parts.h\
			dataloop/segment.c\
			dataloop/segment_count.c\
			dataloop/segment_flatten.c\
			dataloop/segment_packunpack.c\
			dataloop/subarray_support.c\
			dataloop/typesize_support.c\
			dataloop/typesize_support.h

libmpcmpi_la_SOURCES += mpitypes/mpitypes.c\
			mpitypes/mpitypes.h\
			mpitypes/mpitypes_dataloop.c\
			mpitypes/mpitypes_dataloop.h\
			mpitypes/mpitypes_transpack_flatten.c

libmpcmpi_la_LIBADD = ../../MPC_Lowcomm/src/libmpclowcomm.la -lrt
