mpc_headerdir=$(pkgincludedir)
mpc_header_HEADERS= ./include/mpc_thread.h ./include/mpc_thread_engines.h  ./include/mpc_microthread.h


# Install the pthread.h interceptions header
install-data-local:
	install -m 0644 -D $(abs_top_srcdir)/src/MPC_Threads/include/mpc_pthread.h $(pkgincludedir)/pthread.h


SUBDIRS=src
