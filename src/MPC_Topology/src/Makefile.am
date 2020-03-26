AM_CPPFLAGS = -I$(abs_top_srcdir)/src/MPC_Common/include/ -I$(abs_top_srcdir)/src/MPC_Topology/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Arch/include/ -I$(abs_top_builddir)/src/MPC_Arch/include/ 
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/include/ -I$(abs_top_builddir)/src/include/ $(HWLOC_CFLAGS)

if MPC_THREAD_ENABLED
        AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Threads/src/ -I$(abs_top_srcdir)/src/MPC_Threads/include/ -I$(abs_top_srcdir)/src/MPC_Topology/include/
endif

mpc_headerdir=$(includedir)/mpc/

noinst_LTLIBRARIES = libmpctopology.la

libmpctopology_la_SOURCES = topology.c  topology_device.c  topology_render.c

libmpctopology_la_LIBADD = $(abs_top_builddir)/src/MPC_Common/src/libmpccommon.la $(HWLOC_LIBS)

