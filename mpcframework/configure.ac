AC_INIT([mpc], [4.0], [france@paratools.com])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_LANG(C)
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_SRCDIR([MPC_Arch/src/timer.c])
AC_CONFIG_HEADERS([mpc_config.h])
AM_SILENT_RULES([yes])
LT_PREREQ([2.4.6])

LT_INIT
LT_LANG([C])
LT_INIT
AC_PROG_CXX

# Define MPC_VERSION

AC_DEFINE([SCTK_VERSION_MAJOR], [4], [MPC Major Version 3])
AC_DEFINE([SCTK_VERSION_MINOR], [0], [MPC Minor Version 3])
AC_DEFINE([SCTK_VERSION_PRE], [0], [MPC Pre Version 3])

AC_DEFINE_UNQUOTED([MPC_PREFIX_PATH], ["$prefix"], [Prefix Path for MPC])


# General Configuration

MPC_SUPPORTED_ARCHITECTURES_LIST="x86_64 ia64 i686 arm aarch64 powerpc64"

# Programs Checks
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_CC
AM_PROG_AS

AC_LANG_C

# Optionnal XML generators

AC_CHECK_PROG([xsltproc],[xsltproc],[yes],[no])
AC_CHECK_PROG([xmllint],[xmllint],[yes],[no])

AM_CONDITIONAL([CAN_GENERATE_CONFIG], [test "x$xsltproc" = xyes -o "x$xmllint" = xyes])

# Checks for libraries.

PKG_CHECK_MODULES([XML2], [libxml-2.0])

AC_SUBST([XML2_CFLAGS])
AC_SUBST([XML2_LIBS])

PKG_CHECK_MODULES([HWLOC], [hwloc < 2.0])

AC_SUBST([HWLOC_CFLAGS])
AC_SUBST([HWLOC_LIBS])

PKG_CHECK_MODULES([OPENPA], [openpa])

AC_SUBST([OPENPA_CFLAGS])
AC_SUBST([OPENPA_LIBS])

# Checks for header files.

AC_HEADER_STDC

AC_CHECK_HEADERS([aio.h])
AC_CHECK_HEADERS([asm/unistd.h])
AC_CHECK_HEADERS([assert.h])
AC_CHECK_HEADERS([ctype.h])
AC_CHECK_HEADERS([errno.h])
AC_CHECK_HEADERS([execinfo.h])
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([getopt.h])
AC_CHECK_HEADERS([hwloc.h])
AC_CHECK_HEADERS([immintrin.h])
AC_CHECK_HEADERS([libxml/parser.h])
AC_CHECK_HEADERS([libxml/tree.h])
AC_CHECK_HEADERS([libxml/xmlschemas.h])
AC_CHECK_HEADERS([limits.h])
AC_CHECK_HEADERS([linux/personality.h])
AC_CHECK_HEADERS([netdb.h])
AC_CHECK_HEADERS([netinet/in.h])
AC_CHECK_HEADERS([netinet/tcp.h])
AC_CHECK_HEADERS([regex.h])
AC_CHECK_HEADERS([sched.h])
AC_CHECK_HEADERS([semaphore.h])
AC_CHECK_HEADERS([setjmp.h])
AC_CHECK_HEADERS([signal.h])
AC_CHECK_HEADERS([stddef.h])
AC_CHECK_HEADERS([stdint.h])
AC_CHECK_HEADERS([sys/mman.h])
AC_CHECK_HEADERS([sys/param.h])
AC_CHECK_HEADERS([sys/pstat.h])
AC_CHECK_HEADERS([sys/socket.h])
AC_CHECK_HEADERS([sys/stat.h])
AC_CHECK_HEADERS([sys/syscall.h])
AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_HEADERS([sys/types.h])
AC_CHECK_HEADERS([sys/ucontext.h])
AC_CHECK_HEADERS([sys/un.h])
AC_CHECK_HEADERS([sys/utsname.h])
AC_CHECK_HEADERS([sys/wait.h])
AC_CHECK_HEADERS([time.h])
AC_CHECK_HEADERS([ucontext.h])
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([wchar.h])

# Checks for optionnal headers

#TODO CHECK IB
#TODO CHECK WHY libunwind.h
#TODO CHECK PMI
#TODO CHECK STDBOOL
#TODO CHECK VALGRIND
AC_CHECK_HEADERS([pmi.h])
AC_CHECK_HEADERS([valgrind/memcheck.h])
AC_CHECK_HEADERS([valgrind/valgrind.h])
AC_CHECK_HEADER_STDBOOL
AC_CHECK_HEADERS([libunwind.h])
AC_CHECK_HEADERS([infiniband/verbs.h])

# Resolve Host CPU

AC_CANONICAL_HOST
MPC_TARGET_ARCHITECTURE=$host_cpu
AC_SUBST([MPC_TARGET_ARCHITECTURE])

# Check if architecture is in support list

AC_MSG_CHECKING([Checking if target architecture is supported])

is_in_list()
{
    echo "$2" | grep -q "$1"
    return $?
}

if is_in_list "$MPC_TARGET_ARCHITECTURE" "$MPC_SUPPORTED_ARCHITECTURES_LIST"; then
        AC_MSG_RESULT($MPC_TARGET_ARCHITECTURE)
else
        AC_MSG_RESULT(Unsupported)
        AC_MSG_ERROR(Your architecture is unsupported)
fi


# Checks for typedefs, structures, and compiler characteristics.

AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_OFF_T
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for Macros

AC_MSG_CHECKING([macro __func__])

AC_COMPILE_IFELSE(AC_LANG_PROGRAM([#include <stdio.h>],
                                  [fprintf(stderr,"%s",__func__);
                                   return 0;]),
                res_macro=yes,res_macro=no)

if test "$res_macro" = "yes" ; then 
   MPC_ARCH_DETECTED_FUNCTION="__func__"
   AC_MSG_RESULT(yes)
   AC_SUBST(MPC_ARCH_DETECTED_FUNCTION)
else
   AC_MSG_RESULT(no)
   AC_MSG_ERROR([Unable to detect function name])
fi

AC_MSG_CHECKING([macro __STRING])

AC_COMPILE_IFELSE(AC_LANG_PROGRAM([#include <stdio.h>],
                                  [fprintf(stderr,"%s",__STRING(TOTO));
                                   return 0;]),
                res_macro=yes,res_macro=no)

if test "$res_macro" = "yes" ; then 
  MPC_ARCH_DETECTED_STRING="__STRING"
   AC_MSG_RESULT(yes)
   AC_SUBST(MPC_ARCH_DETECTED_STRING)
else
   AC_MSG_RESULT(no)
   AC_MSG_ERROR([Unable to convert to string])
fi

# Checking for builtin

AX_GCC_BUILTIN([__builtin_expect])

AC_C_CONST
AC_C_RESTRICT
AC_C_VOLATILE
AC_C_INLINE

AX_TLS([AC_DEFINE([TLS_SUPPORT], [1], [TLS extension supported by compiler])],
       [AC_DEFINE([TLS_SUPPORT], [0], [TLS extension NOT supported by compiler])])


# Checks for type sizes

AC_CHECK_SIZEOF(char)
AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(float)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(double)
AC_CHECK_SIZEOF(size_t)
AC_CHECK_SIZEOF(void*)
AC_CHECK_SIZEOF(long long)

# Checks for standard library functions.

AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memcpy memmove memset])
AC_CHECK_FUNCS([strcasecmp strchr strdup strerror strstr strtol])
AC_CHECK_FUNCS([gethostbyname gethostname])
AC_CHECK_FUNCS([gettimeofday])
AC_CHECK_FUNCS([munmap])
AC_CHECK_FUNCS([setenv getenv])
AC_CHECK_FUNCS([uname])
AC_CHECK_FUNCS([regcomp])
AC_CHECK_FUNCS([socket connect accept])
AC_CHECK_FUNCS([dup2 fdatasync])
AC_CHECK_FUNCS([makecontext setcontext getcontext swapcontext])

# Check for pThread

AC_MSG_NOTICE(Check Pthreads functions)
AC_CHECK_HEADER([pthread.h])
AC_CHECK_LIB(pthread,pthread_create,LIBS="$LIBS -lpthread")
AC_CHECK_SIZEOF(pthread_t,,[#include <pthread.h>])

AC_CHECK_FUNCS(pthread_yield)
AC_CHECK_FUNCS(pthread_atfork )
AC_CHECK_FUNCS(pthread_cancel )
AC_CHECK_FUNCS(pthread_create )
AC_CHECK_FUNCS(pthread_detach )
AC_CHECK_FUNCS(pthread_equal )
AC_CHECK_FUNCS(pthread_exit )
AC_CHECK_FUNCS(pthread_getconcurrency )
AC_CHECK_FUNCS(pthread_getcpuclockid )
AC_CHECK_FUNCS(pthread_getschedparam )
AC_CHECK_FUNCS(pthread_getspecific )
AC_CHECK_FUNCS(pthread_join )
AC_CHECK_FUNCS(pthread_key_create )
AC_CHECK_FUNCS(pthread_key_delete )
AC_CHECK_FUNCS(pthread_once )

AC_CHECK_TYPE(pthread_attr_t,is_thread=yes,is_thread=no,[#include <pthread.h>])

if test "$is_thread" = "yes" ; then
        AC_CHECK_SIZEOF(pthread_attr_t,,[#include <pthread.h>])
        AC_CHECK_FUNCS(pthread_attr_destroy )
        AC_CHECK_FUNCS(pthread_attr_getdetachstate )
        AC_CHECK_FUNCS(pthread_attr_getguardsize )
        AC_CHECK_FUNCS(pthread_attr_getinheritsched )
        AC_CHECK_FUNCS(pthread_attr_getschedparam )
        AC_CHECK_FUNCS(pthread_attr_getschedpolicy )
        AC_CHECK_FUNCS(pthread_attr_getscope )
        AC_CHECK_FUNCS(pthread_attr_getstack )
        AC_CHECK_FUNCS(pthread_attr_getstackaddr )
        AC_CHECK_FUNCS(pthread_attr_getstacksize )
        AC_CHECK_FUNCS(pthread_attr_init )
        AC_CHECK_FUNCS(pthread_attr_setdetachstate )
        AC_CHECK_FUNCS(pthread_attr_setguardsize )
        AC_CHECK_FUNCS(pthread_attr_setinheritsched )
        AC_CHECK_FUNCS(pthread_attr_setschedparam )
        AC_CHECK_FUNCS(pthread_attr_setschedpolicy )
        AC_CHECK_FUNCS(pthread_attr_setscope )
        AC_CHECK_FUNCS(pthread_attr_setstack )
        AC_CHECK_FUNCS(pthread_attr_setstackaddr )
        AC_CHECK_FUNCS(pthread_attr_setstacksize )
        AC_CHECK_FUNCS(pthread_attr_setaffinity_np )
fi

AC_CHECK_TYPE(pthread_barrier_t,is_barrier=yes,is_barrier=no,[#include <pthread.h>])

if test "$is_barrier" = "yes" ; then
        AC_CHECK_SIZEOF(pthread_barrier_t,,[#include <pthread.h>])
        AC_CHECK_SIZEOF(pthread_barrierattr_t,,[#include <pthread.h>])
        AC_CHECK_FUNCS(pthread_barrier_destroy )
        AC_CHECK_FUNCS(pthread_barrier_init )
        AC_CHECK_FUNCS(pthread_barrier_wait )
        AC_CHECK_FUNCS(pthread_barrierattr_destroy )
        AC_CHECK_FUNCS(pthread_barrierattr_getpshared )
        AC_CHECK_FUNCS(pthread_barrierattr_init )
        AC_CHECK_FUNCS(pthread_barrierattr_setpshared )
fi

AC_CHECK_TYPE(pthread_cond_t,is_cond=yes,is_cond=no,[#include <pthread.h>])
if test "$is_cond" = "yes" ; then
        AC_CHECK_SIZEOF(pthread_cond_t,,[#include <pthread.h>])
        AC_CHECK_SIZEOF(pthread_condattr_t,,[#include <pthread.h>])
        AC_CHECK_FUNCS(pthread_cond_broadcast )
        AC_CHECK_FUNCS(pthread_cond_destroy )
        AC_CHECK_FUNCS(pthread_cond_init )
        AC_CHECK_FUNCS(pthread_cond_signal )
        AC_CHECK_FUNCS(pthread_cond_timedwait )
        AC_CHECK_FUNCS(pthread_cond_wait )
        AC_CHECK_FUNCS(pthread_condattr_destroy )
        AC_CHECK_FUNCS(pthread_condattr_getclock )
        AC_CHECK_FUNCS(pthread_condattr_getpshared )
        AC_CHECK_FUNCS(pthread_condattr_init )
        AC_CHECK_FUNCS(pthread_condattr_setclock )
        AC_CHECK_FUNCS(pthread_condattr_setpshared )
fi

AC_CHECK_TYPE(pthread_mutex_t,is_mutex=yes,is_mutex=no,[#include <pthread.h>])

if test "$is_mutex" = "yes" ; then 
        AC_CHECK_SIZEOF(pthread_mutex_t,,[#include <pthread.h>])
        AC_CHECK_SIZEOF(pthread_mutexattr_t,,[#include <pthread.h>])
        AC_CHECK_FUNCS(pthread_mutex_destroy )
        AC_CHECK_FUNCS(pthread_mutex_getprioceiling )
        AC_CHECK_FUNCS(pthread_mutex_init )
        AC_CHECK_FUNCS(pthread_mutex_lock )
        AC_CHECK_FUNCS(pthread_mutex_setprioceiling )
        AC_CHECK_FUNCS(pthread_mutex_timedlock )
        AC_CHECK_FUNCS(pthread_mutex_trylock )
        AC_CHECK_FUNCS(pthread_mutex_unlock )
        AC_CHECK_FUNCS(pthread_mutexattr_destroy )
        AC_CHECK_FUNCS(pthread_mutexattr_getprioceiling )
        AC_CHECK_FUNCS(pthread_mutexattr_getprotocol )
        AC_CHECK_FUNCS(pthread_mutexattr_getpshared )
        AC_CHECK_FUNCS(pthread_mutexattr_gettype )
        AC_CHECK_FUNCS(pthread_mutexattr_init )
        AC_CHECK_FUNCS(pthread_mutexattr_setprioceiling )
        AC_CHECK_FUNCS(pthread_mutexattr_setprotocol )
        AC_CHECK_FUNCS(pthread_mutexattr_setpshared )
        AC_CHECK_FUNCS(pthread_mutexattr_settype )
fi

AC_CHECK_TYPE(pthread_rwlock_t,is_rwlock=yes,is_rwlock=no,[#include <pthread.h>])

if test "$is_rwlock" = "yes" ; then 
        AC_CHECK_SIZEOF(pthread_rwlock_t,,[#include <pthread.h>])
        AC_CHECK_SIZEOF(pthread_rwlockattr_t,,[#include <pthread.h>])
        AC_CHECK_FUNCS(pthread_rwlock_destroy )
        AC_CHECK_FUNCS(pthread_rwlock_init )
        AC_CHECK_FUNCS(pthread_rwlock_rdlock )
        AC_CHECK_FUNCS(pthread_rwlock_timedrdlock )
        AC_CHECK_FUNCS(pthread_rwlock_timedwrlock )
        AC_CHECK_FUNCS(pthread_rwlock_tryrdlock )
        AC_CHECK_FUNCS(pthread_rwlock_trywrlock )
        AC_CHECK_FUNCS(pthread_rwlock_unlock )
        AC_CHECK_FUNCS(pthread_rwlock_wrlock )
        AC_CHECK_FUNCS(pthread_rwlockattr_destroy )
        AC_CHECK_FUNCS(pthread_rwlockattr_getpshared )
        AC_CHECK_FUNCS(pthread_rwlockattr_init )
        AC_CHECK_FUNCS(pthread_rwlockattr_setpshared )
fi

AC_CHECK_FUNCS(pthread_self )
AC_CHECK_FUNCS(pthread_setcancelstate )
AC_CHECK_FUNCS(pthread_setcanceltype )
AC_CHECK_FUNCS(pthread_setconcurrency )
AC_CHECK_FUNCS(pthread_setschedparam )
AC_CHECK_FUNCS(pthread_setschedprio )
AC_CHECK_FUNCS(pthread_setspecific )

AC_CHECK_TYPE(pthread_spinlock_t,is_spinlock=yes,is_spinlock=no,[#include <pthread.h>])

if test "$is_spinlock" = "yes" ; then
        AC_CHECK_SIZEOF(pthread_spinlock_t)
        AC_CHECK_FUNCS(pthread_spin_destroy )
        AC_CHECK_FUNCS(pthread_spin_init )
        AC_CHECK_FUNCS(pthread_spin_lock )
        AC_CHECK_FUNCS(pthread_spin_trylock )
        AC_CHECK_FUNCS(pthread_spin_unlock )
fi

AC_CHECK_FUNCS(pthread_testcancel )

AC_CHECK_HEADER([semaphore.h])
AC_CHECK_TYPE(sem_t,is_semaphore=yes,is_semaphore=no,[#include <semaphore.h>])

if test "$is_semaphore" = "yes" ; then 
        AC_CHECK_SIZEOF(sem_t,,[#include <semaphore.h>])
        AC_CHECK_FUNCS(sem_init )
        AC_CHECK_FUNCS(sem_wait )
        AC_CHECK_FUNCS(sem_trywait )
        AC_CHECK_FUNCS(sem_post )
        AC_CHECK_FUNCS(sem_getvalue )
        AC_CHECK_FUNCS(sem_destroy )
        AC_CHECK_FUNCS(sem_open )
        AC_CHECK_FUNCS(sem_close )
        AC_CHECK_FUNCS(sem_unlink )
        AC_CHECK_FUNCS(sem_timedwait )
fi

AC_CHECK_FUNCS(pthread_sigmask )
AC_CHECK_FUNCS(pthread_kill )
AC_CHECK_FUNCS(sched_get_priority_max )
AC_CHECK_FUNCS(sched_get_priority_min )

#
# Configure MPC_Arch
#

MPC_CONTEXT_SWITCH_SUPPORTED_ARCHITECTURE="x86_64"

AC_MSG_CHECKING([Configuring optimized context switches])

MPC_ARCH_CONTEXT_SWITCH="generic"

if is_in_list "$MPC_TARGET_ARCHITECTURE" "$MPC_CONTEXT_SWITCH_SUPPORTED_ARCHITECTURE"; then
        MPC_ARCH_CONTEXT_SWITCH="$MPC_TARGET_ARCHITECTURE"
        AC_MSG_RESULT($MPC_TARGET_ARCHITECTURE)
else
        AC_MSG_RESULT(Using standard context switches)
fi

# Note that by construction the following conditionnals are multually exclusive
AM_CONDITIONAL([MPC_ARCH_X86_64_CONTEXT_SWITCH], [test "x${MPC_ARCH_CONTEXT_SWITCH}" = "xx86_64"])

MPC_MICROTASK_SUPPORTED_ARCHITECTURE="x86_64"

AC_MSG_CHECKING([Configuring optimized microtasks])

MPC_ARCH_MICROTASK_ARCH="generic"

if is_in_list "$MPC_TARGET_ARCHITECTURE" "$MPC_MICROTASK_SUPPORTED_ARCHITECTURE"; then
        MPC_ARCH_MICROTASK_ARCH="$MPC_TARGET_ARCHITECTURE"
        AC_MSG_RESULT($MPC_TARGET_ARCHITECTURE)
else
        AC_MSG_RESULT(Using standard microtasks)
fi

# Note that by construction the following conditionnals are multually exclusive
AM_CONDITIONAL([MPC_ARCH_X86_64_MICROTASK], [test "x${MPC_ARCH_MICROTASK_ARCH}" = "xx86_64"])
# Note that by construction the following conditionnals are multually exclusive
AM_CONDITIONAL([MPC_ARCH_GENERIC_MICROTASK], [test "x${MPC_ARCH_MICROTASK_ARCH}" = "xgeneric"])


#
# Modular MPC configuration
#

AC_ARG_WITH([threads],
[AS_HELP_STRING([--with-threads],
                [Compile MPC with pthread and user-level thread support])],
[mpc_thread_enabled=$withval],
[mpc_thread_enabled=yes])

if test "x${mpc_thread_enabled}" = "xyes"; then
        AC_DEFINE([MPC_THREADS], [1], [MPC_THREAD module was enabled])
fi

AM_CONDITIONAL([MPC_THREAD_ENABLED], [test "x${mpc_thread_enabled}" = "xyes"])

#
# Final Output
#

AC_OUTPUT([MPC_Arch/include/mpc_arch.h])

AC_OUTPUT([Makefile MPC_Arch/Makefile MPC_Arch/src/Makefile
           MPC_Common/Makefile MPC_Common/src/Makefile MPC_Config/Makefile
           MPC_Config/src/Makefile MPC_Topology/Makefile MPC_Topology/src/Makefile
           MPC_Threads/Makefile MPC_Threads/src/Makefile])

echo "========== MPC Configuration =========="
echo "PREFIX: $prefix"
echo "MPC_Thread : $mpc_thread_enabled"
echo "======================================="

