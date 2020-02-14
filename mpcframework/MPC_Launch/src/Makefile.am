AM_CPPFLAGS = -I$(abs_top_srcdir)/MPC_Common/include/ -I$(abs_top_srcdir)/MPC_Topology/include/ -I$(abs_top_srcdir)/MPC_Launch/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/MPC_Arch/include/ -I$(abs_top_builddir)/MPC_Arch/include/ -I$(abs_top_srcdir)/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/MPC_Config/include/ -I$(abs_top_srcdir)/MPC_Config/src/generated/

if MPC_THREAD_ENABLED
        AM_CPPFLAGS += -I$(abs_top_srcdir)/MPC_Threads/src/ -I$(abs_top_srcdir)/MPC_Threads/include/ -I$(abs_top_srcdir)/MPC_Topology/include/
endif

if MPC_MESSAGE_PASSING_ENABLED
        AM_CPPFLAGS += -I$(abs_top_srcdir)/MPC_Message_Passing/src/ -I$(abs_top_srcdir)/MPC_Message_Passing/include/
endif

if HYDRA_CLIENT_LIB_ENABED
        AM_CPPFLAGS += -I$(abs_top_srcdir)/MPC_Launch/src/hydra_client
endif

AM_CPPFLAGS += -I$(abs_top_srcdir)/MPC_Message_Passing/src/tcp/
AM_CPPFLAGS += -I$(abs_top_srcdir)/MPC_Message_Passing/src/shm/

if INFINIBAND_DRIVER_ENABLED
        AM_CPPFLAGS += -I$(abs_top_srcdir)/MPC_Message_Passing/src/ib/
endif

mpc_headerdir=$(includedir)/mpc/

noinst_LTLIBRARIES = libmpclaunch.la

libmpclaunch_la_SOURCES = main.c  mpcc.c  mpc_libc_main.c  sctk_launch.c pmi.c

if HYDRA_CLIENT_LIB_ENABED
        libmpclaunch_la_SOURCES += ./hydra_client/safestr2.c  ./hydra_client/simple_pmi.c  ./hydra_client/simple_pmiutil.c
endif


libmpclaunch_la_LIBADD = ../../MPC_Topology/src/libmpctopology.la -ldl
