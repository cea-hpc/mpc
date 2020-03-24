AM_CPPFLAGS = -I$(abs_top_srcdir)/src/MPC_Common/include/ -I$(abs_top_srcdir)/src/include/ -I$(abs_top_builddir)/src/MPC_Arch/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Arch/include/ -I$(abs_top_srcdir)/src/MPC_Threads/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Config/include/ -I$(abs_top_srcdir)/src/MPC_Config/src/generated/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Topology/include/ -I$(abs_top_builddir)/src/MPC_Threads/include/

if MPC_MESSAGE_PASSING_ENABLED
        AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Message_Passing/src/ -I$(abs_top_srcdir)/src/MPC_Message_Passing/include/
endif

if MPC_MPI_ENABLED
        AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_MPI/include/ -I$(abs_top_srcdir)/src/MPC_MPI/src/
        AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Message_Passing/include/
endif


lib_LTLIBRARIES = libmpcthread.la

libmpcthread_la_SOURCES = sctk_context.c sctk_ethread.c sctk_ethread_mxn.c sctk_futex.c sctk_gen_thread.c
libmpcthread_la_SOURCES += sctk_kernel_thread.c sctk_posix_ethread.c sctk_posix_ethread_mxn.c sctk_posix_pthread.c sctk_posix_thread.c
libmpcthread_la_SOURCES += sctk_pthread.c sctk_thread_barrier.c sctk_thread.c sctk_thread_cond.c sctk_thread_generic.c threads_generic_kind.c
libmpcthread_la_SOURCES += sctk_thread_keys.c sctk_thread_mutex.c sctk_thread_rwlock.c sctk_thread_scheduler.c sctk_thread_sem.c sctk_thread_spinlock.c sctk_tls.c

libmpcthread_la_SOURCES += accelerator.c

if MPC_CUDA_ENABLED

libmpcthread_la_SOURCES +=  ./accelerator/cuda.c ./accelerator/cuda_wrap.c

lib_LTLIBRARIES += libmpc_cuda_addon.la

libmpc_cuda_addon_la_SOURCES = ./accelerator/cuda_lib/cuda_lib.c
libmpc_cuda_addon_la_LIBADD = -lcuda

endif


libmpcthread_la_LIBADD = ../../MPC_Launch/src/libmpclaunch.la  -lpthread
