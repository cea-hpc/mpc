
mpc_headerdir=$(includedir)/mpc/
mpc_header_HEADERS= ./include/mpcthread.h ./include/sctk_posix_thread.h ./include/mpcthread_config.h ./include/sctk_thread_api.h ./src/sctk_futex_def.h


# Install the pthread.h interceptions header
install-data-local:
	install -m 0644 -D $(abs_top_srcdir)/src/MPC_Threads/src/sctk_pthread_api.h $(includedir)/mpc/pthread.h


SUBDIRS=src
