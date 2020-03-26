AM_CPPFLAGS = -I$(abs_top_srcdir)/src/MPC_Common/include/ -I$(abs_top_srcdir)/src/include/ -I$(abs_top_builddir)/src/MPC_Arch/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Arch/include/ -I$(abs_top_srcdir)/src/MPC_Threads/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Config/include/ -I$(abs_top_srcdir)/src/MPC_Config/src/generated/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Topology/include/ -I$(abs_top_srcdir)/src/MPC_Message_Passing/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Launch/include/ -I$(abs_top_builddir)/src/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Threads/src/ -I$(abs_top_srcdir)/src/MPC_Threads/include/

AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_OpenMP/include/

AM_LDFLAGS = lol

lib_LTLIBRARIES = libmpcomp.la

libmpcomp_la_SOURCES = mpcomp_api.c mpcomp_core.c mpcomp_fortran.c mpcomp_loop.c
libmpcomp_la_SOURCES += mpcomp_parallel_region.c mpcomp_places_env.c mpcomp_spinning_core.c
libmpcomp_la_SOURCES += mpcomp_stack.c mpcomp_stack_node_leaf.c mpcomp_sync.c mpcomp_task.c
libmpcomp_la_SOURCES += mpcomp_tree.c omp_gomp.c omp_intel_atomics.c omp_intel.c omp_ompt.c

libmpcomp_la_LDFLAGS = -Wl,--version-script=$(abs_top_srcdir)/src/MPC_OpenMP/version_gomp.txt
libmpcomp_la_LIBADD = ../../MPC_Launch/src/libmpclaunch.la

lib_LTLIBRARIES += libiomp5.la libgomp.la

libiomp5_la_SOURCES = ./fakelib/fakelib.c
libgomp_la_SOURCES = ./fakelib/fakelib.c

