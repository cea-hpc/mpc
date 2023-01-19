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
libmpclowcomm_la_SOURCES = coll.c comm.c sctk_checksum.c ft.c group.c communicator.c shm_coll.c lowcomm_config.c
libmpclowcomm_la_SOURCES += sctk_low_level_comm.c multirail.c sctk_net_tools.c sctk_net_topology.c lowcomm_thread.c mqd.c datatypes_common.c
libmpclowcomm_la_SOURCES += sctk_rail.c reorder.c endpoint.c sctk_topological_polling.c sctk_topological_rail.c sctk_window.c workshare.c pset.c handle_ctx.c alloc_mem.c

# TCP driver #
libmpclowcomm_la_SOURCES += ./tcp/tcp.c ./tcp/tcp_toolkit.c

# SHM driver #
libmpclowcomm_la_SOURCES += ./shm/sctk_shm.c      ./shm/sctk_shm_eager.c  ./shm/sctk_shm_raw_queues.c
libmpclowcomm_la_SOURCES += ./shm/sctk_shm_cma.c  ./shm/sctk_shm_frag.c   ./shm/sctk_shm_qemu.c    ./shm/sctk_shm_raw_queues_internals.c


# IB driver #
if INFINIBAND_DRIVER_ENABLED
AM_CPPFLAGS += -I$(abs_srcdir)/ib/
libmpclowcomm_la_SOURCES += ./ib/async.c ./ib/buffered.c ./ib/ib.c ./ib/cm.c
libmpclowcomm_la_SOURCES += ./ib/ibconfig.c ./ib/cp.c ./ib/ibdevice.c ./ib/ibeager.c
libmpclowcomm_la_SOURCES += ./ib/ibmmu.c ./ib/ibmpi.c ./ib/ibpolling.c
libmpclowcomm_la_SOURCES += ./ib/qp.c ./ib/ibrdma.c ./ib/ibtoolkit.c ./ib/ibtopology.c
libmpclowcomm_la_SOURCES += ./ib/ibufs.c ./ib/ibufs_rdma.c
endif

# PTL driver #
if PORTALS4_DRIVER_ENABLED
AM_CPPFLAGS += -I$(abs_srcdir)/portals/
libmpclowcomm_la_SOURCES += ./portals/sctk_portals.c ./portals/sctk_ptl_cm.c ./portals/sctk_ptl_eager.c
libmpclowcomm_la_SOURCES += ./portals/sctk_ptl_iface.c ./portals/sctk_ptl_offcoll.c ./portals/sctk_ptl_rdma.c
libmpclowcomm_la_SOURCES += ./portals/sctk_ptl_rdv.c ./portals/sctk_ptl_toolkit.c
if LCP_ENABLED
libmpclowcomm_la_SOURCES += ./portals/lcr_ptl.c ./portals/lcr_ptl_recv.c ./portals/lcr_ptl_iface.c
endif

# PTL-AM driver #
libmpclowcomm_la_SOURCES += ./portals_am/sctk_ptl_am_iface.c  ./portals_am/sctk_ptl_am_toolkit.c ./portals_am/sctk_ptl_am_arpc_api.c ./portals_am/arpc_weak.c
endif

if OFI_DRIVER_ENABLED
AM_CPPFLAGS += -I$(abs_srcdir)/ofi/
libmpclowcomm_la_SOURCES +=./ofi/ofi.c ./ofi/ofi_msg.c ./ofi/ofi_rdma.c ./ofi/ofi_toolkit.c
endif

if LCP_ENABLED
AM_CPPFLAGS += -I$(abs_src_dir)/lcp/
AM_CPPFLAGS += -I$(abs_src_dir)/lcr/
libmpclowcomm_la_SOURCES += ./lcp/lcp_context.c ./lcp/lcp_ep.c ./lcp/lcp_pending.c ./lcp/lcp_progress.c ./lcp/lcp_recv.c ./lcp/lcp_send.c ./lcp/lcp_tag_lists.c 
libmpclowcomm_la_SOURCES += ./lcp/lcp_tag_matching.c ./lcp/lcp_tag.c ./lcp/lcp_request.c ./lcp/lcp_common.c ./lcp/lcp_rndv.c ./lcp/lcp_tag_offload.c ./lcp/lcp_mem.c ./lcp/lcp_rma.c

libmpclowcomm_la_SOURCES += ./lcr/lcr_component.c
endif

libmpclowcomm_la_SOURCES += ./cplane/auth.c ./cplane/peer.c  ./cplane/proto.c ./cplane/port.c
libmpclowcomm_la_SOURCES += ./cplane/monitor.c  ./cplane/set.c ./cplane/uid.c ./cplane/name.c

# library dependency #
libmpclowcomm_la_LIBADD =  $(ALLOC_LIBS) ../../MPC_Launch/src/libmpclaunch.la -lm -lrt

if MPC_HAVE_AM
 # Active message support (ffi)
 libmpclowcomm_la_SOURCES += am.c
 libmpclowcomm_la_LIBADD += $(FFI_LIBS)
endif
