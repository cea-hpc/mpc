AM_CPPFLAGS = -I$(abs_top_srcdir)/src/MPC_Common/include/ -I$(abs_top_srcdir)/src/include/ -I$(abs_top_builddir)/src/MPC_Arch/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Arch/include/ -I$(abs_top_srcdir)/src/MPC_IO/include/ -I$(abs_top_srcdir)/src/MPC_MPI/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Message_Passing/include/ -I$(abs_top_srcdir)/src/MPC_Threads/include/ -I$(abs_top_builddir)/src/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Launch/include/ -I$(abs_top_srcdir)/src/MPC_Topology/include/

SUBDIRS=romio

lib_LTLIBRARIES = libmpcio.la

libmpcio_la_SOURCES = mpl.c mpir_ext.c

libmpcio_la_LIBADD = ../../MPC_MPI/src/libmpcmpi.la ./romio/libromio.la  ./romio/libpromio.la 

