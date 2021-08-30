
mpc_headerdir=$(pkgincludedir)

mpc_header_HEADERS = $(abs_top_builddir)/src/include/mpc.h $(abs_top_srcdir)/src/include/mpc_main.h

if MPC_THREAD_ENABLED
  mpc_header_HEADERS +=  $(abs_top_builddir)/src/include/mpc_threads_config.h
endif

if MPC_MPI_ENABLED
  mpc_header_HEADERS += $(abs_top_srcdir)/src/include/mpi.h
endif
#
# Define the MPC Library
#

lib_LTLIBRARIES = libmpcframework.la

libmpcframework_la_LIBADD = @ALL_MPC_LIBS@

libmpcframework_la_SOURCES=
