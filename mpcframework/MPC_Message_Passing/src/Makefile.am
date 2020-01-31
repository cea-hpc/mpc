AM_CPPFLAGS = -I$(abs_top_srcdir)/MPC_Common/include/ -I$(abs_top_srcdir)/include/ -I$(abs_top_builddir)/MPC_Arch/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/MPC_Arch/include/ -I$(abs_top_srcdir)/MPC_Threads/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/MPC_Config/include/ -I$(abs_top_srcdir)/MPC_Config/src/generated/
AM_CPPFLAGS += -I$(abs_top_srcdir)/MPC_Topology/include/ -I$(abs_top_srcdir)/MPC_Message_Passing/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/MPC_Launch/include/

AM_CPPFLAGS += -I$(abs_top_srcdir)/MPC_Message_Passing/src/tcp/
AM_CPPFLAGS += -I$(abs_top_srcdir)/MPC_Message_Passing/src/shm/

if MPC_THREAD_ENABLED
        AM_CPPFLAGS += -I$(abs_top_srcdir)/MPC_Threads/src/ -I$(abs_top_srcdir)/MPC_Threads/include/ -I$(abs_top_srcdir)/MPC_Topology/include/
endif

lib_LTLIBRARIES = libmpclowcomm.la

libmpclowcomm_la_SOURCES = coll.c comm.c sctk_checksum.c sctk_communicator.c sctk_control_messages.c
libmpclowcomm_la_SOURCES += sctk_handle.c sctk_low_level_comm.c sctk_multirail.c sctk_net_tools.c sctk_net_topology.c
libmpclowcomm_la_SOURCES += sctk_polling.c sctk_rail.c sctk_reorder.c sctk_route.c sctk_topological_polling.c sctk_topological_rail.c sctk_window.c


libmpclowcomm_la_SOURCES += ./tcp/sctk_tcp.c  ./tcp/sctk_tcp_rdma.c  ./tcp/sctk_tcp_toolkit.c

libmpclowcomm_la_SOURCES += ./shm/sctk_handler_mpi.c  ./shm/sctk_shm.c      ./shm/sctk_shm_eager.c  ./shm/sctk_shm_mapper.c  ./shm/sctk_shm_raw_queues.c
libmpclowcomm_la_SOURCES += ./shm/sctk_handler_pmi.c  ./shm/sctk_shm_cma.c  ./shm/sctk_shm_frag.c   ./shm/sctk_shm_qemu.c    ./shm/sctk_shm_raw_queues_internals.c

libmpclowcomm_la_LIBADD = ../../MPC_Launch/src/libmpclaunch.la  -lpthread
