AM_CPPFLAGS = -I$(abs_top_srcdir)/src/MPC_Common/include/ -I$(abs_top_srcdir)/src/include/ -I$(abs_top_builddir)/src/MPC_Arch/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Arch/include/ -I$(abs_top_srcdir)/src/MPC_Threads/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Config/include/ -I$(abs_top_srcdir)/src/MPC_Config/src/generated/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Topology/include/ -I$(abs_top_builddir)/src/include/

if MPC_MESSAGE_PASSING_ENABLED
        AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Message_Passing/src/ -I$(abs_top_srcdir)/src/MPC_Message_Passing/include/
endif

if MPC_MPI_ENABLED
        AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_MPI/include/ -I$(abs_top_srcdir)/src/MPC_MPI/src/
        AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Message_Passing/include/
endif


lib_LTLIBRARIES = libmpcthread.la

libmpcthread_la_SOURCES = sctk_context.c sctk_ethread.c sctk_ethread_mxn.c sctk_futex.c thread_ptr.c
libmpcthread_la_SOURCES += kthread.c ethread_posix.c
libmpcthread_la_SOURCES += pthread_engine.c thread.c  ng_engine.c 
libmpcthread_la_SOURCES += ng_engine_scheduler.c sctk_tls.c

libmpcthread_la_SOURCES += accelerator.c

if MPC_CUDA_ENABLED

libmpcthread_la_SOURCES +=  ./accelerator/cuda.c ./accelerator/cuda_wrap.c

lib_LTLIBRARIES += libmpc_cuda_addon.la

libmpc_cuda_addon_la_SOURCES = ./accelerator/cuda_lib/cuda_lib.c
libmpc_cuda_addon_la_LIBADD = -lcuda

endif


libmpcthread_la_LIBADD = ../../MPC_Launch/src/libmpclaunch.la  -lpthread
