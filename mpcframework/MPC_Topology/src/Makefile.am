AM_CPPFLAGS = -I$(abs_top_srcdir)/MPC_Common/include/ -I$(abs_top_srcdir)/MPC_Topology/include/ -I$(abs_top_srcdir)/MPC_Arch/include/ -I$(abs_top_builddir)/MPC_Arch/include/ -I$(abs_top_srcdir)/include/ $(HWLOC_CFLAGS)

if MPC_THREAD_ENABLED
        AM_CPPFLAGS += -I$(abs_top_srcdir)/MPC_Threads/src/ -I$(abs_top_srcdir)/MPC_Threads/include/ -I$(abs_top_srcdir)/MPC_Topology/include/
endif

mpc_headerdir=$(includedir)/mpc/

noinst_LTLIBRARIES = libmpctopology.la

libmpctopology_la_SOURCES = topology.c  topology_device.c  topology_render.c

libmpctopology_la_LIBADD = $(abs_top_builddir)/MPC_Common/src/libmpccommon.la $(HWLOC_LIBS)

