AM_CPPFLAGS = -I$(abs_top_srcdir)/MPC_Common/include/ -I$(abs_top_srcdir)/include/ -I$(abs_top_builddir)/MPC_Arch/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/MPC_Arch/include/ -I$(abs_top_srcdir)/MPC_Threads/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/MPC_Config/include/ -I$(abs_top_srcdir)/MPC_Config/src/generated/
AM_CPPFLAGS += -I$(abs_top_srcdir)/MPC_Topology/include/ -I$(abs_top_srcdir)/MPC_Message_Passing/include/ -I$(abs_top_srcdir)/MPC_Message_Passing/src/
AM_CPPFLAGS += -I$(abs_top_srcdir)/MPC_Launch/include/ -I$(abs_top_srcdir)/MPC_MPI/include/

if MPC_THREAD_ENABLED
        AM_CPPFLAGS += -I$(abs_top_srcdir)/MPC_Threads/src/ -I$(abs_top_srcdir)/MPC_Threads/include/ -I$(abs_top_srcdir)/MPC_Topology/include/
endif

if INFINIBAND_DRIVER_ENABLED
AM_CPPFLAGS += -I$(abs_top_srcdir)/MPC_Message_Passing/src/ib/
endif

if PORTALS4_DRIVER_ENABLED
        AM_CPPFLAGS += -I$(abs_top_srcdir)/MPC_Message_Passing/src/portals/
endif


#Yes drivers are leaking out

AM_CPPFLAGS += -I$(abs_top_srcdir)/MPC_Message_Passing/src/tcp/
AM_CPPFLAGS += -I$(abs_top_srcdir)/MPC_Message_Passing/src/shm/


lib_LTLIBRARIES = libmpcmpi.la

libmpcmpi_la_SOURCES = comm_lib.c egreq_nbc.c mpc_extern32.c mpc_mpi_halo.c mpc_reduction.c mpi_rma.c mpi_rma_win.c
libmpcmpi_la_SOURCES += datatype.c egreq_progress.c mpc_info.c mpc_nbc.c mpc_romio_glue.c mpi_rma_ctrl_msg.c mpit.c
libmpcmpi_la_SOURCES += egreq_classes.c mpc_cs.c mpc_mpi.c mpc_nbc_progress_thread_binding.c mpi_alloc_mem.c mpi_rma_epoch.c

libmpcmpi_la_LIBADD = ../../MPC_Message_Passing/src/libmpclowcomm.la -lrt
