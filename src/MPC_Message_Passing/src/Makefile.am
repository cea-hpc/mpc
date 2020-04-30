AM_CPPFLAGS = -I$(abs_top_srcdir)/src/MPC_Common/include/ -I$(abs_top_srcdir)/src/include/ -I$(abs_top_builddir)/src/MPC_Arch/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Arch/include/ -I$(abs_top_srcdir)/src/MPC_Threads/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Config/include/ -I$(abs_top_srcdir)/src/MPC_Config/src/generated/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Topology/include/ -I$(abs_top_srcdir)/src/MPC_Message_Passing/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Launch/include/  -I$(abs_top_builddir)/src/include/

AM_CPPFLAGS += -I$(abs_srcdir)/tcp/
AM_CPPFLAGS += -I$(abs_srcdir)/shm/

if MPC_THREAD_ENABLED
        AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Threads/src/ -I$(abs_top_srcdir)/src/MPC_Threads/include/ -I$(abs_top_srcdir)/src/MPC_Topology/include/
endif

lib_LTLIBRARIES = libmpclowcomm.la

# low-level communication #
libmpclowcomm_la_SOURCES = coll.c comm.c sctk_checksum.c sctk_communicator.c sctk_control_messages.c ft.c
libmpclowcomm_la_SOURCES += sctk_handle.c sctk_low_level_comm.c sctk_multirail.c sctk_net_tools.c sctk_net_topology.c
libmpclowcomm_la_SOURCES += sctk_polling.c sctk_rail.c sctk_reorder.c sctk_route.c sctk_topological_polling.c sctk_topological_rail.c sctk_window.c

# TCP driver #
libmpclowcomm_la_SOURCES += ./tcp/sctk_tcp.c  ./tcp/sctk_tcp_rdma.c  ./tcp/sctk_tcp_toolkit.c

# SHM driver #
libmpclowcomm_la_SOURCES += ./shm/sctk_handler_mpi.c  ./shm/sctk_shm.c      ./shm/sctk_shm_eager.c  ./shm/sctk_shm_mapper.c  ./shm/sctk_shm_raw_queues.c
libmpclowcomm_la_SOURCES += ./shm/sctk_handler_pmi.c  ./shm/sctk_shm_cma.c  ./shm/sctk_shm_frag.c   ./shm/sctk_shm_qemu.c    ./shm/sctk_shm_raw_queues_internals.c


# IB driver #
if INFINIBAND_DRIVER_ENABLED
AM_CPPFLAGS += -I$(abs_srcdir)/ib/
libmpclowcomm_la_SOURCES += ./ib/sctk_ib_async.c ./ib/sctk_ib_buffered.c ./ib/sctk_ib.c ./ib/sctk_ib_cm.c
libmpclowcomm_la_SOURCES += ./ib/sctk_ib_config.c ./ib/sctk_ib_cp.c ./ib/sctk_ib_device.c ./ib/sctk_ib_eager.c
libmpclowcomm_la_SOURCES += ./ib/sctk_ib_mmu.c ./ib/sctk_ib_mpi.c ./ib/sctk_ib_polling.c
libmpclowcomm_la_SOURCES += ./ib/sctk_ib_qp.c ./ib/sctk_ib_rdma.c ./ib/sctk_ib_toolkit.c ./ib/sctk_ib_topology.c
libmpclowcomm_la_SOURCES += ./ib/sctk_ibufs.c ./ib/sctk_ibufs_rdma.c
endif

# PTL driver #
if PORTALS4_DRIVER_ENABLED
AM_CPPFLAGS += -I$(abs_srcdir)/portals/
libmpclowcomm_la_SOURCES += ./portals/sctk_portals.c ./portals/sctk_ptl_cm.c ./portals/sctk_ptl_eager.c
libmpclowcomm_la_SOURCES += ./portals/sctk_ptl_iface.c ./portals/sctk_ptl_offcoll.c ./portals/sctk_ptl_rdma.c
libmpclowcomm_la_SOURCES += ./portals/sctk_ptl_rdv.c ./portals/sctk_ptl_toolkit.c

# PTL-AM driver #
libmpclowcomm_la_SOURCES += ./portals_am/sctk_ptl_am_iface.c  ./portals_am/sctk_ptl_am_toolkit.c ./portals_am/sctk_ptl_am_arpc_api.c ./portals_am/arpc_weak.c
endif

# library dependency #
libmpclowcomm_la_LIBADD = ../../MPC_Launch/src/libmpclaunch.la -lm
