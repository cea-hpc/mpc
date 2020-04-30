AM_CPPFLAGS = -I$(abs_top_srcdir)/src/MPC_Common/include/ -I$(abs_top_srcdir)/src/MPC_Topology/include/ -I$(abs_top_srcdir)/src/MPC_Launch/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Arch/include/ -I$(abs_top_builddir)/src/MPC_Arch/include/ -I$(abs_top_srcdir)/src/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Config/include/ -I$(abs_top_srcdir)/src/MPC_Config/src/generated/  -I$(abs_top_builddir)/src/include/

if MPC_THREAD_ENABLED
        AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Threads/src/ -I$(abs_top_srcdir)/src/MPC_Threads/include/ -I$(abs_top_srcdir)/src/MPC_Topology/include/
endif

if HYDRA_CLIENT_LIB_ENABED
        AM_CPPFLAGS += -I$(abs_srcdir)/hydra_client
endif



mpc_headerdir=$(pkgincludedir)

lib_LTLIBRARIES = libmpclaunch.la

libmpclaunch_la_SOURCES = main.c  libc_main.c  mpc_launch.c pmi.c

if HYDRA_CLIENT_LIB_ENABED
        libmpclaunch_la_SOURCES += ./hydra_client/safestr2.c  ./hydra_client/simple_pmi.c  ./hydra_client/simple_pmiutil.c
endif


libmpclaunch_la_LIBADD = ../../MPC_Topology/src/libmpctopology.la -ldl
