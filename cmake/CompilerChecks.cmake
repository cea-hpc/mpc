# ==================================================================
# CompilerChecks.cmake — Header, function, and type size checks
# ==================================================================

# ------------------------------------------------------------------
# Header file checks
# ------------------------------------------------------------------
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
check_function_exists(strdup HAVE_STRDUP)
check_function_exists(strerror HAVE_STRERROR)
check_function_exists(vsnprintf HAVE_VSNPRINTF)

# ------------------------------------------------------------------
# Function checks - memory management
# ------------------------------------------------------------------
check_function_exists(mremap HAVE_MREMAP)
check_function_exists(posix_memalign HAVE_POSIX_MEMALIGN)
check_function_exists(memalign HAVE_MEMALIGN)

# ------------------------------------------------------------------
# Function checks - environment and process
# ------------------------------------------------------------------
check_function_exists(statfs HAVE_STATFS)


# ------------------------------------------------------------------
# Function checks - pthreads
# ------------------------------------------------------------------
check_function_exists(pthread_atfork HAVE_PTHREAD_ATFORK)
check_function_exists(pthread_cancel HAVE_PTHREAD_CANCEL)
check_function_exists(pthread_create HAVE_PTHREAD_CREATE)
check_function_exists(pthread_detach HAVE_PTHREAD_DETACH)
check_function_exists(pthread_getconcurrency HAVE_PTHREAD_GETCONCURRENCY)
check_function_exists(pthread_once HAVE_PTHREAD_ONCE)
check_function_exists(pthread_sigmask HAVE_PTHREAD_SIGMASK)
check_function_exists(pthread_kill HAVE_PTHREAD_KILL)

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
check_type_size("long" SIZEOF_LONG LANGUAGE C)

# ------------------------------------------------------------------
# Type size checks - pthread types
# ------------------------------------------------------------------
if(HAVE_PTHREAD_H)
    set(CMAKE_REQUIRED_LIBRARIES Threads::Threads)
    set(CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")
    set(CMAKE_EXTRA_INCLUDE_FILES "pthread.h;sys/types.h;sys/sysmacros.h;sched.h")
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


