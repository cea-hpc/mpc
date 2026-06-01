# ==================================================================
# CompilerChecks.cmake — Header, function, and type size checks
# ==================================================================

# ------------------------------------------------------------------
# Header file checks
# ------------------------------------------------------------------
check_include_file(stdbool.h HAVE_STDBOOL_H)
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(wchar.h HAVE_WCHAR_H)
check_include_file(semaphore.h HAVE_SEMAPHORE_H)
check_include_file(pthread.h HAVE_PTHREAD_H)
check_include_file(sys/uio.h HAVE_UIO_H)
check_include_file(string.h HAVE_STRING_H)
check_include_file(fcntl.h HAVE_FCNTL_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(sys/vfs.h HAVE_SYS_VFS_H)
check_include_file(sys/statvfs.h HAVE_SYS_STATVFS_H)
check_include_file(sys/param.h HAVE_SYS_PARAM_H)
check_include_file(sys/mount.h HAVE_SYS_MOUNT_H)

# ------------------------------------------------------------------
# Function checks - memory and string
# ------------------------------------------------------------------
check_function_exists(memcpy HAVE_MEMCPY)
check_function_exists(memmove HAVE_MEMMOVE)
check_function_exists(memset HAVE_MEMSET)
check_function_exists(strcasecmp HAVE_STRCASECMP)
check_function_exists(strchr HAVE_STRCHR)
check_function_exists(strdup HAVE_STRDUP)
check_function_exists(strerror HAVE_STRERROR)
check_function_exists(strstr HAVE_STRSTR)
check_function_exists(strtol HAVE_STRTOL)
check_function_exists(snprintf HAVE_SNPRINTF)
check_function_exists(vsnprintf HAVE_VSNPRINTF)

# ------------------------------------------------------------------
# Function checks - system and network
# ------------------------------------------------------------------
check_function_exists(gethostbyname HAVE_GETHOSTBYNAME)
check_function_exists(gethostname HAVE_GETHOSTNAME)
check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)
check_function_exists(uname HAVE_UNAME)
check_function_exists(regcomp HAVE_REGCOMP)
check_function_exists(socket HAVE_SOCKET)
check_function_exists(connect HAVE_CONNECT)
check_function_exists(accept HAVE_ACCEPT)
check_function_exists(dup2 HAVE_DUP2)

# ------------------------------------------------------------------
# Function checks - memory management
# ------------------------------------------------------------------
check_function_exists(mmap HAVE_MMAP)
check_function_exists(mremap HAVE_MREMAP)
check_function_exists(munmap HAVE_MUNMAP)
check_function_exists(posix_memalign HAVE_POSIX_MEMALIGN)
check_function_exists(memalign HAVE_MEMALIGN)
check_function_exists(malloc HAVE_MALLOC)
check_function_exists(realloc HAVE_REALLOC)

# ------------------------------------------------------------------
# Function checks - environment and process
# ------------------------------------------------------------------
check_function_exists(setenv HAVE_SETENV)
check_function_exists(getenv HAVE_GETENV)
check_function_exists(fdatasync HAVE_FDATASYNC)
check_function_exists(fork HAVE_FORK)
check_function_exists(statfs HAVE_STATFS)

# ------------------------------------------------------------------
# Function checks - context switching
# ------------------------------------------------------------------
check_function_exists(makecontext HAVE_MAKECONTEXT)
check_function_exists(setcontext HAVE_SETCONTEXT)
check_function_exists(getcontext HAVE_GETCONTEXT)
check_function_exists(swapcontext HAVE_SWAPCONTEXT)

# ------------------------------------------------------------------
# Function checks - pthreads
# ------------------------------------------------------------------
check_function_exists(pthread_yield HAVE_PTHREAD_YIELD)
check_function_exists(pthread_atfork HAVE_PTHREAD_ATFORK)
check_function_exists(pthread_cancel HAVE_PTHREAD_CANCEL)
check_function_exists(pthread_create HAVE_PTHREAD_CREATE)
check_function_exists(pthread_detach HAVE_PTHREAD_DETACH)
check_function_exists(pthread_equal HAVE_PTHREAD_EQUAL)
check_function_exists(pthread_exit HAVE_PTHREAD_EXIT)
check_function_exists(pthread_getconcurrency HAVE_PTHREAD_GETCONCURRENCY)
check_function_exists(pthread_getcpuclockid HAVE_PTHREAD_GETCPUCLOCKID)
check_function_exists(pthread_getschedparam HAVE_PTHREAD_GETSCHEDPARAM)
check_function_exists(pthread_getspecific HAVE_PTHREAD_GETSPECIFIC)
check_function_exists(pthread_join HAVE_PTHREAD_JOIN)
check_function_exists(pthread_key_create HAVE_PTHREAD_KEY_CREATE)
check_function_exists(pthread_key_delete HAVE_PTHREAD_KEY_DELETE)
check_function_exists(pthread_once HAVE_PTHREAD_ONCE)
check_function_exists(pthread_self HAVE_PTHREAD_SELF)
check_function_exists(pthread_setspecific HAVE_PTHREAD_SETSPECIFIC)
check_function_exists(pthread_sigmask HAVE_PTHREAD_SIGMASK)
check_function_exists(pthread_kill HAVE_PTHREAD_KILL)

# ------------------------------------------------------------------
# Function checks - scheduling
# ------------------------------------------------------------------
check_function_exists(sched_get_priority_max HAVE_SCHED_GET_PRIORITY_MAX)
check_function_exists(sched_get_priority_min HAVE_SCHED_GET_PRIORITY_MIN)

# ------------------------------------------------------------------
# ROMIO statfs struct checks
# ------------------------------------------------------------------
# Check for struct statfs with f_type and f_fstypename (needed by ROMIO)
set(_STATFS_INCLUDES "#include <sys/types.h>")
if(HAVE_SYS_VFS_H)
    set(_STATFS_INCLUDES "${_STATFS_INCLUDES}\n#include <sys/vfs.h>")
endif()
if(HAVE_SYS_STATVFS_H)
    set(_STATFS_INCLUDES "${_STATFS_INCLUDES}\n#include <sys/statvfs.h>")
endif()
if(HAVE_SYS_PARAM_H)
    set(_STATFS_INCLUDES "${_STATFS_INCLUDES}\n#include <sys/param.h>")
endif()
if(HAVE_SYS_MOUNT_H)
    set(_STATFS_INCLUDES "${_STATFS_INCLUDES}\n#include <sys/mount.h>")
endif()
if(HAVE_STRING_H)
    set(_STATFS_INCLUDES "${_STATFS_INCLUDES}\n#include <string.h>")
endif()

check_c_source_compiles("
${_STATFS_INCLUDES}
int main() {
    struct statfs f;
    memset(&f, 0, sizeof(f));
    return 0;
}
" HAVE_STRUCT_STATFS)

check_c_source_compiles("
${_STATFS_INCLUDES}
int main() {
    struct statfs f;
    memset(&f, 0, sizeof(f));
    f.f_type = 0;
    return 0;
}
" ROMIO_HAVE_STRUCT_STATFS_WITH_F_TYPE)

check_c_source_compiles("
${_STATFS_INCLUDES}
int main() {
    struct statfs f;
    memset(&f, 0, sizeof(f));
    strncmp(\"nfs\", f.f_fstypename, 3);
    return 0;
}
" ROMIO_HAVE_STRUCT_STATFS_WITH_F_FSTYPENAME)

# ------------------------------------------------------------------
# Type size checks - basic types
# ------------------------------------------------------------------
check_type_size("int" SIZEOF_INT LANGUAGE C)
check_type_size("long" SIZEOF_LONG LANGUAGE C)
check_type_size("size_t" SIZEOF_SIZE_T LANGUAGE C)
check_type_size("ssize_t" SIZEOF_SSIZE_T LANGUAGE C)
check_type_size("void*" SIZEOF_VOID_PTR LANGUAGE C)

# ------------------------------------------------------------------
# Type size checks - pthread types
# ------------------------------------------------------------------
if(HAVE_PTHREAD_H)
    set(CMAKE_REQUIRED_LIBRARIES Threads::Threads)
    set(CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")
    set(CMAKE_EXTRA_INCLUDE_FILES "pthread.h;sys/types.h;sys/sysmacros.h;sched.h")
    check_type_size("pthread_t" SIZEOF_PTHREAD_T LANGUAGE C)
    check_type_size("pthread_key_t" SIZEOF_PTHREAD_KEY_T LANGUAGE C)
    check_type_size("pthread_attr_t" SIZEOF_PTHREAD_ATTR_T LANGUAGE C)
    check_type_size("pthread_once_t" SIZEOF_PTHREAD_ONCE_T LANGUAGE C)
    check_type_size("pthread_mutex_t" SIZEOF_PTHREAD_MUTEX_T LANGUAGE C)
    check_type_size("pthread_mutexattr_t" SIZEOF_PTHREAD_MUTEXATTR_T LANGUAGE C)
    check_type_size("pthread_cond_t" SIZEOF_PTHREAD_COND_T LANGUAGE C)
    check_type_size("pthread_condattr_t" SIZEOF_PTHREAD_CONDATTR_T LANGUAGE C)
    check_type_size("pthread_rwlock_t" SIZEOF_PTHREAD_RWLOCK_T LANGUAGE C)
    check_type_size("pthread_rwlockattr_t" SIZEOF_PTHREAD_RWLOCKATTR_T LANGUAGE C)
    check_type_size("pthread_barrier_t" SIZEOF_PTHREAD_BARRIER_T LANGUAGE C)
    check_type_size("pthread_barrierattr_t" SIZEOF_PTHREAD_BARRIERATTR_T LANGUAGE C)
    check_type_size("pthread_spinlock_t" SIZEOF_PTHREAD_SPINLOCK_T LANGUAGE C)
    check_type_size("cpu_set_t" SIZEOF_CPU_SET_T LANGUAGE C)
    unset(CMAKE_EXTRA_INCLUDE_FILES)
    unset(CMAKE_REQUIRED_DEFINITIONS)
    unset(CMAKE_REQUIRED_LIBRARIES)
endif()

# ------------------------------------------------------------------
# Type size checks - semaphore and signal types
# ------------------------------------------------------------------
if(HAVE_SEMAPHORE_H)
    set(CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")
    set(CMAKE_EXTRA_INCLUDE_FILES "semaphore.h")
    check_type_size("sem_t" SIZEOF_SEM_T LANGUAGE C)
    unset(CMAKE_EXTRA_INCLUDE_FILES)
    unset(CMAKE_REQUIRED_DEFINITIONS)
endif()

set(CMAKE_EXTRA_INCLUDE_FILES "signal.h")
check_type_size("sigset_t" SIZEOF_SIGSET_T LANGUAGE C)
unset(CMAKE_EXTRA_INCLUDE_FILES)
