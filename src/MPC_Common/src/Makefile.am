AM_CPPFLAGS = -I$(abs_top_srcdir)/src/MPC_Common/include/ -I$(abs_top_srcdir)/src/include/ -I$(abs_top_builddir)/src/MPC_Arch/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Arch/include/ -I$(abs_top_builddir)/src/include/ $(OPENPA_CFLAGS)

if MPC_THREAD_ENABLED
        AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Threads/src/ -I$(abs_top_srcdir)/src/MPC_Threads/include/ -I$(abs_top_srcdir)/src/MPC_Topology/include/
endif

noinst_LTLIBRARIES = libmpccommon.la

elf_64.c: elf_32.c
	$(abs_srcdir)/gen_elf64

libmpccommon_la_SOURCES = aio.c bit_array.c fifo.c hashtable.c io_helper.c rank.c sctk_debug.c sctk_no_alloc.c sctk_pm_json.c flags.c debugger.c elf_32.c elf_64.c


if MPC_PROFILER_ENABLED
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_MPI/src/profiler/

libmpccommon_la_SOURCES += ./profiler/sctk_internal_profiler.c ./profiler/sctk_performance_tree.c
libmpccommon_la_SOURCES += ./profiler/sctk_profile_meta.c ./profiler/sctk_profiler_array.c
libmpccommon_la_SOURCES += ./profiler/sctk_profile_render.c ./profiler/sctk_profile_render_html.c
libmpccommon_la_SOURCES += ./profiler/sctk_profile_render_tex.c ./profiler/sctk_profile_render_text.c
libmpccommon_la_SOURCES += ./profiler/sctk_profile_render_xml.c
endif

libmpccommon_la_LIBADD = $(abs_top_builddir)/src/MPC_Arch/src/libmpcarch.la $(OPENPA_LIBS) $(ALLOC_LIBS)
