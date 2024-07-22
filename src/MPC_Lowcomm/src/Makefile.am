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
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Launch/include/  -I$(abs_top_builddir)/src/include/

AM_CPPFLAGS += -I$(abs_srcdir)/tcp/
AM_CPPFLAGS += -I$(abs_srcdir)/shm/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Lowcomm/src/cplane/

if MPC_BUILD_OPENPA
AM_CPPFLAGS += -I$(abs_top_srcdir)/contrib/openpa/src -I$(abs_top_builddir)/contrib/openpa/src
else
AM_CPPFLAGS+=$(OPA_CFLAGS)
endif

if MPC_THREAD_ENABLED
        AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Threads/src/ -I$(abs_top_srcdir)/src/MPC_Threads/include/ -I$(abs_top_srcdir)/src/MPC_Topology/include/
endif

if MPC_MPI_ENABLED
        AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_MPI/include/
endif

lib_LTLIBRARIES = libmpclowcomm.la

# low-level communication #
libmpclowcomm_la_SOURCES = coll.c comm.c checksum.c ft.c group.c communicator.c shm_coll.c lowcomm_config.c alloc_mem.c
libmpclowcomm_la_SOURCES += lowcomm.c msg_cpy.c lowcomm_thread.c mqd.c datatypes_common.c
libmpclowcomm_la_SOURCES += rail.c reorder.c endpoint.c pset.c handle_ctx.c

if ENABLE_WORKSHARE
libmpclowcomm_la_SOURCES += workshare.c
endif


libmpclowcomm_la_SOURCES += ./cplane/auth.c ./cplane/peer.c  ./cplane/proto.c ./cplane/port.c
libmpclowcomm_la_SOURCES += ./cplane/monitor.c  ./cplane/set.c ./cplane/uid.c ./cplane/name.c

# library dependency #
libmpclowcomm_la_LIBADD =  $(ALLOC_LIBS) ../../MPC_Launch/src/libmpclaunch.la -lm -lrt

#
# All the components below
#

# LCP #
AM_CPPFLAGS += -I$(abs_srcdir)/tbsm/
AM_CPPFLAGS += -I$(abs_srcdir)/lcp/ -I$(abs_srcdir)/lcr/
libmpclowcomm_la_SOURCES += ./tbsm/tbsm.c
# LCP core files
libmpclowcomm_la_SOURCES += ./lcp/core/lcp_context.c ./lcp/core/lcp_ep.c ./lcp/core/lcp_progress.c \
							./lcp/core/lcp_datatype.c ./lcp/core/lcp_manager.c ./lcp/core/lcp_task.c \
							./lcp/core/lcp_request.c ./lcp/core/lcp_mem.c 
# LCP tag files
libmpclowcomm_la_SOURCES += ./lcp/tag/lcp_tag.c ./lcp/tag/lcp_send.c ./lcp/tag/lcp_tag_offload.c \
							./lcp/tag/lcp_tag_match.c ./lcp/tag/lcp_pending.c ./lcp/tag/lcp_recv.c \
							./lcp/tag/lcp_probe.c 
# LCP rndv and active message files
libmpclowcomm_la_SOURCES += ./lcp/rndv/lcp_rndv.c ./lcp/am/lcp_eager.c ./lcp/am/lcp_am.c
# LCP rma files
libmpclowcomm_la_SOURCES += ./lcp/rma/lcp_rma.c ./lcp/rma/lcp_ato.c ./lcp/rma/lcp_ato_sw.c 
libmpclowcomm_la_SOURCES += ./lcr/lcr_component.c ./shm/mpc_shm.c

#AM Extensions #
if MPC_HAVE_AM
 # Active message support (ffi)
 libmpclowcomm_la_SOURCES += am.c
 libmpclowcomm_la_LIBADD += $(FFI_LIBS)
endif

# TCP driver #
libmpclowcomm_la_SOURCES += ./tcp/tcp.c ./tcp/tcp_toolkit.c

# PTL driver #
if PORTALS4_DRIVER_ENABLED
AM_CPPFLAGS += -I$(abs_srcdir)/ptl/
libmpclowcomm_la_SOURCES += ./ptl/ptl_ep.c ./ptl/ptl_iface.c ./ptl/ptl_recv.c ./ptl/ptl_rd.c ./ptl/ptl_rma.c
if PORTALS4_CONTROL_FLOW_ENABLED
libmpclowcomm_la_SOURCES += ./ptl/ptl_tk.c
endif
endif

# OFI Driver #
if OFI_DRIVER_ENABLED
AM_CPPFLAGS += -I$(abs_srcdir)/ofi/ $(OFI_CFLAGS)
libmpclowcomm_la_SOURCES += ./ofi/mpc_ofi_request.c ./ofi/mpc_ofi_helpers.c ./ofi/mpc_ofi_dns.c ./ofi/mpc_ofi_domain.c ./ofi/mpc_ofi_context.c ./ofi/mpc_ofi.c
libmpclowcomm_la_LIBADD += $(OFI_LIBS)
endif
