AM_CPPFLAGS = -I$(abs_top_srcdir)/MPC_Common/include/ -I$(abs_top_srcdir)/include/ -I$(abs_top_builddir)/MPC_Arch/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/MPC_Arch/include/ $(OPENPA_CFLAGS)

noinst_LTLIBRARIES = libmpccommon.la

libmpccommon_la_SOURCES = aio.c bit_array.c fifo.c hashtable.c io_helper.c rank.c sctk_debug.c sctk_no_alloc.c sctk_pm_json.c flags.c
libmpccommon_la_LIBADD = $(abs_top_builddir)/MPC_Arch/src/libmpcarch.la $(OPENPA_LIBS)