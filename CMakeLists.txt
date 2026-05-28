cmake_minimum_required(VERSION 3.20.3)

# ------------------------------------------------------------------
# Version extraction from .VERSION file
# ------------------------------------------------------------------
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/.VERSION" _VERSION_LINES)
foreach(_LINE IN LISTS _VERSION_LINES)
    if(_LINE MATCHES "^major=([0-9]+)$")
        set(_MPC_VERSION_MAJOR "${CMAKE_MATCH_1}")
    elseif(_LINE MATCHES "^minor=([0-9]+)$")
        set(_MPC_VERSION_MINOR "${CMAKE_MATCH_1}")
    elseif(_LINE MATCHES "^patch=([0-9]+)$")
        set(_MPC_VERSION_PATCH "${CMAKE_MATCH_1}")
    endif()
endforeach()
set(_MPC_VERSION_TAINT "")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    execute_process(
        COMMAND git -C "${CMAKE_CURRENT_SOURCE_DIR}" rev-parse --short HEAD
        OUTPUT_VARIABLE _MPC_GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(_MPC_GIT_HASH)
        set(_MPC_VERSION_TAINT "@${_MPC_GIT_HASH}")
        execute_process(
            COMMAND git -C "${CMAKE_CURRENT_SOURCE_DIR}" diff --quiet
            RESULT_VARIABLE _MPC_GIT_DIRTY
            ERROR_QUIET
        )
        if(NOT _MPC_GIT_DIRTY EQUAL 0)
            set(_MPC_VERSION_TAINT "${_MPC_VERSION_TAINT}-dirty")
        endif()
    endif()
endif()
set(MPC_VERSION_PRE "${_MPC_VERSION_TAINT}")

# ------------------------------------------------------------------
# Project definition
# ------------------------------------------------------------------
# Set default compilers to match autotools (AC_PROG_CC/CXX/FC)
if(NOT CMAKE_C_COMPILER)
    find_program(DEFAULT_CC gcc)
    if(DEFAULT_CC)
        set(CMAKE_C_COMPILER "${DEFAULT_CC}" CACHE STRING "C compiler" FORCE)
    endif()
endif()
if(NOT CMAKE_CXX_COMPILER)
    find_program(DEFAULT_CXX g++)
    if(DEFAULT_CXX)
        set(CMAKE_CXX_COMPILER "${DEFAULT_CXX}" CACHE STRING "CXX compiler" FORCE)
    endif()
endif()
if(NOT CMAKE_Fortran_COMPILER)
    find_program(DEFAULT_FC gfortran)
    if(DEFAULT_FC)
        set(CMAKE_Fortran_COMPILER "${DEFAULT_FC}" CACHE STRING "Fortran compiler" FORCE)
    endif()
endif()

project(MPC
    VERSION ${_MPC_VERSION_MAJOR}.${_MPC_VERSION_MINOR}.${_MPC_VERSION_PATCH}
    DESCRIPTION "Multi-Processor Computing Framework"
    HOMEPAGE_URL "https://mpc.hpcframework.com"
    LANGUAGES C CXX
)

# Enable ASM language for architecture-specific assembly sources
enable_language(ASM)



# ------------------------------------------------------------------
# Default build type
# ------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ------------------------------------------------------------------
# Default install prefix
# ------------------------------------------------------------------
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Install prefix" FORCE)
endif()

# ------------------------------------------------------------------
# Architecture detection
# ------------------------------------------------------------------
set(MPC_TARGET_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
string(TOLOWER "${MPC_TARGET_ARCHITECTURE}" MPC_TARGET_ARCHITECTURE)

set(MPC_SUPPORTED_ARCHITECTURES "x86_64" "ia64" "i686" "arm" "aarch64" "powerpc64")
list(FIND MPC_SUPPORTED_ARCHITECTURES "${MPC_TARGET_ARCHITECTURE}" _ARCH_INDEX)
if(_ARCH_INDEX EQUAL -1)
    message(FATAL_ERROR "Architecture ${MPC_TARGET_ARCHITECTURE} is not supported")
endif()

# Context switch architecture
set(MPC_CONTEXT_SWITCH_SUPPORTED_ARCHITECTURES "x86_64")
list(FIND MPC_CONTEXT_SWITCH_SUPPORTED_ARCHITECTURES "${MPC_TARGET_ARCHITECTURE}" _CTX_INDEX)
if(_CTX_INDEX GREATER -1)
    set(MPC_ARCH_CONTEXT_SWITCH "${MPC_TARGET_ARCHITECTURE}")
else()
    set(MPC_ARCH_CONTEXT_SWITCH "generic")
endif()

# Microtask architecture
set(MPC_MICROTASK_SUPPORTED_ARCHITECTURES "x86_64")
list(FIND MPC_MICROTASK_SUPPORTED_ARCHITECTURES "${MPC_TARGET_ARCHITECTURE}" _MT_INDEX)
if(_MT_INDEX GREATER -1)
    set(MPC_ARCH_MICROTASK_ARCH "${MPC_TARGET_ARCHITECTURE}")
else()
    set(MPC_ARCH_MICROTASK_ARCH "generic")
endif()

# ------------------------------------------------------------------
# Configuration options
# ------------------------------------------------------------------
option(MPC_ENABLE_THREADS "Enable pthread and user-level thread support" ON)
option(MPC_ENABLE_OPENMP "Enable OpenMP interface" ON)
option(MPC_ENABLE_LOWCOMM "Enable Lowcomm support" ON)
option(MPC_ENABLE_MPI "Enable MPI interface" ON)
option(MPC_ENABLE_FORTRAN "Enable Fortran support" ON)
option(MPC_ENABLE_MPIIO "Enable MPI-IO (ROMIO)" ON)
option(BUILD_ROMIO_EMBEDDED "Build ROMIO embedded in MPC (skip large_count.c fallback)" ON)
option(MPC_ENABLE_PROFILER "Enable internal profiler" OFF)
option(MPC_ENABLE_CUDA "Enable CUDA context switching" OFF)
option(MPC_ENABLE_ROCM "Enable ROCM context switching" OFF)
option(MPC_ENABLE_AM "Enable Active Messages (requires libffi)" ON)
option(MPC_ENABLE_DMTCP "Enable DMTCP checkpoint/restart" OFF)
option(PORTALS4_CONTROL_FLOW_ENABLED "Enable Portals4 control flow" OFF)
option(MPC_ENABLE_WORKSHARE "Enable MPI Workshare" OFF)
option(MPC_ENABLE_SHELL_COLORS "Enable colored shell output" OFF)
option(MPC_ENABLE_CMA "Enable Cross Memory Attach for SHM" ON)
option(MPC_ENABLE_DEBUG_MESSAGES "Enable debug messages at runtime" OFF)
option(MPC_ENABLE_DEBUG "Compile with debug symbols" OFF)
option(MPC_ENABLE_DEBUG_MEM "Compile with ASAN" OFF)
option(MPC_ENABLE_OPTIM "Enable standard optimizations (-O3)" ON)
option(MPC_ENABLE_OPT_NATIVE "Build for current architecture (-march=native)" OFF)
option(MPC_ENABLE_SLURM "Enable SLURM support" OFF)
option(MPC_ENABLE_TOPOLOGY_SIMULATION "Enable topology simulation" OFF)
option(MPC_ENABLE_CK "Enable Concurrency-Kit" OFF)
option(MPC_ENABLE_MPCALLOC "Enable MPC allocator" OFF)
set(MPC_ENABLE_OFI "AUTO" CACHE STRING "Enable libfabric/OFI: AUTO (auto-detect), ON, or OFF")
set_property(CACHE MPC_ENABLE_OFI PROPERTY STRINGS AUTO ON OFF)

# Dependency prefixes (overridden by installmpc or user)
set(MPC_HWLOC_PREFIX "" CACHE PATH "hwloc install prefix")
set(MPC_OPENPA_PREFIX "" CACHE PATH "openpa install prefix")
set(MPC_PMIX_PREFIX "" CACHE PATH "PMIx install prefix")
set(MPC_PMI_PREFIX "" CACHE PATH "PMI1 install prefix")
set(MPC_LIBFABRIC_PREFIX "" CACHE PATH "libfabric install prefix")
set(MPC_CUDA_PREFIX "" CACHE PATH "CUDA install prefix")
set(MPC_ROCM_PREFIX "" CACHE PATH "ROCm install prefix")
set(MPC_MPCALLOC_PREFIX "" CACHE PATH "MPC allocator install prefix")
set(MPC_DMTCP_PREFIX "" CACHE PATH "DMTCP install prefix")

# Launcher selection: HYDRA, PMI1, PMIX
set(MPC_LAUNCHER "HYDRA" CACHE STRING "Launcher: HYDRA, PMI1, or PMIX")
set_property(CACHE MPC_LAUNCHER PROPERTY STRINGS HYDRA PMI1 PMIX)

# ------------------------------------------------------------------
# Option dependency enforcement
# ------------------------------------------------------------------
if(NOT MPC_ENABLE_THREADS)
    message(WARNING "Disabling threads means disabling OpenMP and MPI")
    set(MPC_ENABLE_OPENMP OFF)
    set(MPC_ENABLE_MPI OFF)
endif()

if(NOT MPC_ENABLE_LOWCOMM)
    message(WARNING "Disabling lowcomm means disabling MPC's MPI layer")
    set(MPC_ENABLE_MPI OFF)
endif()

if(MPC_ENABLE_MPI AND NOT MPC_ENABLE_LOWCOMM)
    message(FATAL_ERROR "MPI requires Lowcomm (enable MPC_ENABLE_LOWCOMM)")
endif()

if(MPC_ENABLE_OPENMP AND NOT MPC_ENABLE_THREADS)
    message(FATAL_ERROR "OpenMP requires threads (enable MPC_ENABLE_THREADS)")
endif()

if(MPC_ENABLE_FORTRAN AND NOT MPC_ENABLE_MPI)
    message(FATAL_ERROR "Fortran requires MPI (enable MPC_ENABLE_MPI)")
endif()

if(MPC_ENABLE_MPIIO AND NOT MPC_ENABLE_MPI)
    message(FATAL_ERROR "MPI-IO requires MPI (enable MPC_ENABLE_MPI)")
endif()

# Enable Fortran language after options are resolved
if(MPC_ENABLE_FORTRAN)
    enable_language(Fortran)
endif()

# ------------------------------------------------------------------
# Find external dependencies
# ------------------------------------------------------------------
include(GNUInstallDirs)
include(CheckIncludeFile)
include(CheckLibraryExists)

# Global mpc_config.h include: generated in build root, needed by all subprojects
add_library(mpc_config_headers INTERFACE)
target_include_directories(mpc_config_headers INTERFACE ${CMAKE_BINARY_DIR})
include(CheckSymbolExists)
include(CheckTypeSize)
include(CheckFunctionExists)
include(CheckCSourceCompiles)
include(CheckCSourceRuns)
include(FindPackageHandleStandardArgs)

find_package(PkgConfig REQUIRED)

# hwloc (REQUIRED)
pkg_check_modules(HWLOC REQUIRED hwloc>=2.0.0)

# openpa (OPA)
pkg_check_modules(OPA openpa)
if(NOT OPA_FOUND)
    message(FATAL_ERROR "openpa library not found. Install openpa or provide -DOPA_INCLUDE_DIR/-DOPA_LIBRARY")
endif()

# pthread
find_package(Threads REQUIRED)

# math library
check_library_exists(m log "" HAVE_LIBM)

# rt library
check_library_exists(rt clock_gettime "" HAVE_LIBRT)

# dl library
check_library_exists(dl dlopen "" HAVE_LIBDL)

# libffi (optional, for active messages)
if(MPC_ENABLE_AM)
    pkg_check_modules(FFI libffi)
    if(FFI_FOUND)
        set(MPC_HAVE_AM YES)
        set(HAVE_LIB_FFI YES)
    else()
        set(MPC_HAVE_AM NO)
        set(HAVE_LIB_FFI NO)
        message(STATUS "libffi not found, Active Messages disabled")
    endif()
else()
    set(MPC_HAVE_AM NO)
endif()

# libfabric (optional) - AUTO matches autotools behavior: probe and enable if found
if(MPC_ENABLE_OFI STREQUAL "OFF")
    set(OFI_DRIVER_ENABLED NO)
    set(MPC_NETWORK_LIST "tcp shm")
else()
    pkg_check_modules(OFI libfabric)
    if(OFI_FOUND)
        set(OFI_DRIVER_ENABLED YES)
        set(MPC_NETWORK_LIST "tcp shm ofi")
    else()
        set(OFI_DRIVER_ENABLED NO)
        set(MPC_NETWORK_LIST "tcp shm")
        if(MPC_ENABLE_OFI STREQUAL "ON")
            message(FATAL_ERROR "libfabric requested but not found")
        else()
            message(STATUS "libfabric not found, OFI disabled")
        endif()
    endif()
endif()

# Portals4 (optional)
pkg_check_modules(PORTALS portals4)
if(PORTALS_FOUND)
    set(PORTALS4_DRIVER_ENABLED YES)
    list(APPEND MPC_NETWORK_LIST "portals4" "portals4_am")
endif()

# ROMIO template variables (for mpcio.h.in)
set(DEFINE_HAVE_MPI_GREQUEST "#define HAVE_MPI_GREQUEST")
set(DEFINE_MPI_OFFSET "/* MPI OFFSET is in mpi.h */")
set(NEEDS_MPI_FINT "")
set(HAVE_MPI_INFO "#define HAVE_MPI_INFO")
set(HAVE_MPI_DARRAY_SUBARRAY "#define HAVE_MPI_DARRAY_SUBARRAY")

# libvmem (optional)
pkg_check_modules(VMEM libvmem)
if(VMEM_FOUND)
    set(MPC_HAVE_LIBVMEM YES)
else()
    set(MPC_HAVE_LIBVMEM NO)
endif()

# libunwind (optional)
pkg_check_modules(UNW libunwind)
if(UNW_FOUND)
    set(MPC_HAVE_LIBUNWIND YES)
else()
    set(MPC_HAVE_LIBUNWIND NO)
endif()

# CUDA (optional)
if(MPC_ENABLE_CUDA)
    if(MPC_CUDA_PREFIX)
        list(APPEND CMAKE_PREFIX_PATH "${MPC_CUDA_PREFIX}")
    endif()
    find_package(CUDAToolkit)
    if(CUDAToolkit_FOUND)
        set(MPC_CUDA_ENABLED YES)
    else()
        message(FATAL_ERROR "CUDA requested but not found")
        set(MPC_CUDA_ENABLED NO)
    endif()
else()
    set(MPC_CUDA_ENABLED NO)
endif()

# ROCM/HIP (optional)
if(MPC_ENABLE_ROCM)
    if(MPC_ROCM_PREFIX)
        list(APPEND CMAKE_PREFIX_PATH "${MPC_ROCM_PREFIX}")
    endif()
    find_package(hip)
    if(hip_FOUND)
        set(MPC_ROCM_ENABLED YES)
    else()
        message(FATAL_ERROR "ROCM requested but not found")
        set(MPC_ROCM_ENABLED NO)
    endif()
else()
    set(MPC_ROCM_ENABLED NO)
endif()

# PMIx (optional, for launcher)
if(MPC_LAUNCHER STREQUAL "PMIX")
    pkg_check_modules(PMIX REQUIRED pmix)
elseif(MPC_LAUNCHER STREQUAL "PMI1")
    pkg_check_modules(PMI REQUIRED pmi)
endif()

# ------------------------------------------------------------------
# Compiler checks
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
check_function_exists(statfs HAVE_STATFS)

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
check_function_exists(gethostbyname HAVE_GETHOSTBYNAME)
check_function_exists(gethostname HAVE_GETHOSTNAME)
check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)
check_function_exists(mmap HAVE_MMAP)
check_function_exists(mremap HAVE_MREMAP)
check_function_exists(munmap HAVE_MUNMAP)
check_function_exists(posix_memalign HAVE_POSIX_MEMALIGN)
check_function_exists(memalign HAVE_MEMALIGN)
check_function_exists(setenv HAVE_SETENV)
check_function_exists(getenv HAVE_GETENV)
check_function_exists(uname HAVE_UNAME)
check_function_exists(regcomp HAVE_REGCOMP)
check_function_exists(socket HAVE_SOCKET)
check_function_exists(connect HAVE_CONNECT)
check_function_exists(accept HAVE_ACCEPT)
check_function_exists(dup2 HAVE_DUP2)
check_function_exists(fdatasync HAVE_FDATASYNC)
check_function_exists(fork HAVE_FORK)
check_function_exists(malloc HAVE_MALLOC)
check_function_exists(realloc HAVE_REALLOC)
check_function_exists(makecontext HAVE_MAKECONTEXT)
check_function_exists(setcontext HAVE_SETCONTEXT)
check_function_exists(getcontext HAVE_GETCONTEXT)
check_function_exists(swapcontext HAVE_SWAPCONTEXT)
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
check_function_exists(sched_get_priority_max HAVE_SCHED_GET_PRIORITY_MAX)
check_function_exists(sched_get_priority_min HAVE_SCHED_GET_PRIORITY_MIN)

# Type sizes
check_type_size("int" SIZEOF_INT LANGUAGE C)
check_type_size("long" SIZEOF_LONG LANGUAGE C)
check_type_size("size_t" SIZEOF_SIZE_T LANGUAGE C)
check_type_size("ssize_t" SIZEOF_SSIZE_T LANGUAGE C)
check_type_size("void*" SIZEOF_VOID_PTR LANGUAGE C)

if(HAVE_PTHREAD_H)
    set(CMAKE_REQUIRED_LIBRARIES Threads::Threads)
    set(CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")
    set(CMAKE_EXTRA_INCLUDE_FILES "sys/types.h;sys/sysmacros.h;sched.h")
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

if(HAVE_SEMAPHORE_H)
    set(CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")
    check_type_size("sem_t" SIZEOF_SEM_T LANGUAGE C)
    unset(CMAKE_REQUIRED_DEFINITIONS)
endif()

check_type_size("sigset_t" SIZEOF_SIGSET_T LANGUAGE C)

# Fallback values for type sizes that may fail detection
# Use CACHE ... FORCE to override empty cache entries from check_type_size
if(NOT SIZEOF_CPU_SET_T)
    set(SIZEOF_CPU_SET_T 128 CACHE INTERNAL "Size of cpu_set_t" FORCE)
endif()
if(NOT SIZEOF_SEM_T)
    set(SIZEOF_SEM_T 32 CACHE INTERNAL "Size of sem_t" FORCE)
endif()
if(NOT SIZEOF_PTHREAD_SPINLOCK_T)
    set(SIZEOF_PTHREAD_SPINLOCK_T 4 CACHE INTERNAL "Size of pthread_spinlock_t" FORCE)
endif()

# ------------------------------------------------------------------
# Runtime constant extraction via try_run
# ------------------------------------------------------------------
function(mpc_extract_define HEADER DEFINE CMAKE_VAR)
    set(_TEST_SRC "
#include <${HEADER}>
#include <stdio.h>
int main() {
    long long val = (long long) ${DEFINE};
    FILE *f = fopen(\"${CMAKE_CURRENT_BINARY_DIR}/define_val.out\", \"w\");
    if (!f) return 1;
    fprintf(f, \"%lld\\n\", val);
    fclose(f);
    return 0;
}
")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/define_test.c" "${_TEST_SRC}")
    try_run(_RUN_RES _COMPILE_RES
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/define_test.c"
        COMPILE_OUTPUT_VARIABLE _COMPILE_OUT
        RUN_OUTPUT_VARIABLE _RUN_OUT
    )
    if(_COMPILE_RES AND _RUN_RES EQUAL 0)
        file(READ "${CMAKE_CURRENT_BINARY_DIR}/define_val.out" _VAL)
        string(STRIP "${_VAL}" _VAL)
        set(${CMAKE_VAR} "${_VAL}" PARENT_SCOPE)
    else()
        set(${CMAKE_VAR} "0" PARENT_SCOPE)
    endif()
    file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/define_test.c" "${CMAKE_CURRENT_BINARY_DIR}/define_val.out")
endfunction()

# NSIG / _NSIG
mpc_extract_define("signal.h" "NSIG" MPC_CONFIG_NSIG)
if(NOT MPC_CONFIG_NSIG OR MPC_CONFIG_NSIG STREQUAL "0")
    mpc_extract_define("signal.h" "_NSIG" MPC_CONFIG_NSIG)
    if(NOT MPC_CONFIG_NSIG OR MPC_CONFIG_NSIG STREQUAL "0")
        set(MPC_CONFIG_NSIG "32")
    endif()
endif()

# Pthread constants
mpc_extract_define("pthread.h" "PTHREAD_MUTEX_NORMAL" MPC_CONFIG_PTHREAD_MUTEX_NORMAL)
mpc_extract_define("pthread.h" "PTHREAD_MUTEX_RECURSIVE" MPC_CONFIG_PTHREAD_MUTEX_RECURSIVE)
mpc_extract_define("pthread.h" "PTHREAD_MUTEX_ERRORCHECK" MPC_CONFIG_PTHREAD_MUTEX_ERRORCHECK)
mpc_extract_define("pthread.h" "PTHREAD_MUTEX_DEFAULT" MPC_CONFIG_PTHREAD_MUTEX_DEFAULT)
mpc_extract_define("pthread.h" "PTHREAD_PRIO_NONE" MPC_CONFIG_PTHREAD_PRIO_NONE)
mpc_extract_define("pthread.h" "PTHREAD_PRIO_INHERIT" MPC_CONFIG_PTHREAD_PRIO_INHERIT)
mpc_extract_define("pthread.h" "PTHREAD_PRIO_PROTECT" MPC_CONFIG_PTHREAD_PRIO_PROTECT)
mpc_extract_define("pthread.h" "PTHREAD_CREATE_JOINABLE" MPC_CONFIG_PTHREAD_CREATE_JOINABLE)
mpc_extract_define("pthread.h" "PTHREAD_CREATE_DETACHED" MPC_CONFIG_PTHREAD_CREATE_DETACHED)
mpc_extract_define("pthread.h" "PTHREAD_INHERIT_SCHED" MPC_CONFIG_PTHREAD_INHERIT_SCHED)
mpc_extract_define("pthread.h" "PTHREAD_EXPLICIT_SCHED" MPC_CONFIG_PTHREAD_EXPLICIT_SCHED)
mpc_extract_define("pthread.h" "PTHREAD_SCOPE_SYSTEM" MPC_CONFIG_PTHREAD_SCOPE_SYSTEM)
mpc_extract_define("pthread.h" "PTHREAD_SCOPE_PROCESS" MPC_CONFIG_PTHREAD_SCOPE_PROCESS)
mpc_extract_define("pthread.h" "PTHREAD_PROCESS_PRIVATE" MPC_CONFIG_PTHREAD_PROCESS_PRIVATE)
mpc_extract_define("pthread.h" "PTHREAD_PROCESS_SHARED" MPC_CONFIG_PTHREAD_PROCESS_SHARED)
mpc_extract_define("pthread.h" "PTHREAD_CANCEL_ENABLE" MPC_CONFIG_PTHREAD_CANCEL_ENABLE)
mpc_extract_define("pthread.h" "PTHREAD_CANCEL_DISABLE" MPC_CONFIG_PTHREAD_CANCEL_DISABLE)
mpc_extract_define("pthread.h" "PTHREAD_CANCEL_DEFERRED" MPC_CONFIG_PTHREAD_CANCEL_DEFERRED)
mpc_extract_define("pthread.h" "PTHREAD_BARRIER_SERIAL_THREAD" MPC_CONFIG_PTHREAD_BARRIER_SERIAL_THREAD)
mpc_extract_define("pthread.h" "PTHREAD_CANCEL_ASYNCHRONOUS" MPC_CONFIG_PTHREAD_CANCEL_ASYNCHRONOUS)
mpc_extract_define("pthread.h" "PTHREAD_CANCELED" MPC_CONFIG_PTHREAD_CANCELED)
mpc_extract_define("pthread.h" "PTHREAD_KEYS_MAX" MPC_THREAD_KEYS_MAX)
if(NOT MPC_THREAD_KEYS_MAX OR MPC_THREAD_KEYS_MAX STREQUAL "0")
    set(MPC_THREAD_KEYS_MAX 1024)
endif()

mpc_extract_define("sched.h" "SCHED_OTHER" MPC_CONFIG_SCHED_OTHER)
mpc_extract_define("sched.h" "SCHED_RR" MPC_CONFIG_SCHED_RR)
mpc_extract_define("sched.h" "SCHED_FIFO" MPC_CONFIG_SCHED_FIFO)

# SEM constants
mpc_extract_define("semaphore.h" "SEM_VALUE_MAX" MPC_CONFIG_SEM_VALUE_MAX)
if(NOT MPC_CONFIG_SEM_VALUE_MAX OR MPC_CONFIG_SEM_VALUE_MAX STREQUAL "0")
    set(MPC_CONFIG_SEM_VALUE_MAX "2147483647")
endif()
mpc_extract_define("semaphore.h" "SEM_FAILED" MPC_CONFIG_SEM_FAILED)

# Compiler detection
check_c_source_compiles("
#ifndef __GNUC__
#error
#endif
int main() { return 0; }
" MPC_COMPILER_IS_GCC)

check_c_source_compiles("
#ifndef __clang__
#error
#endif
int main() { return 0; }
" MPC_COMPILER_IS_CLANG)

# TLS support
set(CMAKE_REQUIRED_FLAGS "")
check_c_source_compiles("
int main() {
    _Thread_local int x = 0;
    return x;
}
" TLS_SUPPORT)
if(NOT TLS_SUPPORT)
    set(TLS_SUPPORT 0)
else()
    set(TLS_SUPPORT 1)
endif()

# pragma message support
set(_OLD_CFLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
check_c_source_compiles("
int main() {
    #pragma message \"test\"
    return 0;
}
" HAVE_PRAGMA_MESSAGE)
set(CMAKE_C_FLAGS "${_OLD_CFLAGS}")

# __func__ support
check_c_source_compiles("
#include <stdio.h>
int main() {
    printf(\"%s\", __func__);
    return 0;
}
" HAVE___FUNC__)
if(HAVE___FUNC__)
    set(MPC_ARCH_DETECTED_FUNCTION "__func__")
else()
    message(FATAL_ERROR "Unable to detect function name macro")
endif()

# __STRING support - GCC and Clang both support this built-in preprocessor macro
if(MPC_COMPILER_IS_GCC OR MPC_COMPILER_IS_CLANG OR CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(MPC_ARCH_DETECTED_STRING "__STRING")
else()
    message(FATAL_ERROR "Missing __STRING macro (GCC/Clang required)")
endif()

# visibility attribute
check_c_source_compiles("
__attribute__((visibility(\"default\"))) int foo(void);
int main() { return 0; }
" HAVE_VISIBILITY)
if(HAVE_VISIBILITY)
    set(HAVE_VISIBILITY 1)
else()
    set(HAVE_VISIBILITY 0)
endif()

# environ variable
check_c_source_compiles("
#include <stdlib.h>
extern char **environ;
int main() { return (int)(long)environ; }
" HAVE_ENVIRON_VAR)

# __builtin_expect support
check_c_source_compiles("
int main() {
    int x = 1;
    return __builtin_expect(x, 0);
}
" HAVE_BUILTIN_EXPECT)
if(HAVE_BUILTIN_EXPECT)
    set(HAVE_BUILTIN_EXPECT 1)
else()
    set(HAVE_BUILTIN_EXPECT 0)
endif()

# PTHREAD_STACK_MIN
mpc_extract_define("pthread.h" "PTHREAD_STACK_MIN" MPC_CONFIG_PTHREAD_STACK_MIN)
if(NOT MPC_CONFIG_PTHREAD_STACK_MIN OR MPC_CONFIG_PTHREAD_STACK_MIN STREQUAL "0")
    mpc_extract_define("limits.h" "PTHREAD_STACK_MIN" MPC_CONFIG_PTHREAD_STACK_MIN)
    if(NOT MPC_CONFIG_PTHREAD_STACK_MIN OR MPC_CONFIG_PTHREAD_STACK_MIN STREQUAL "0")
        set(MPC_CONFIG_PTHREAD_STACK_MIN "8192")
    endif()
endif()

# Process mode / extls (matches autotools: default no, auto-enables if extls not found)
option(MPC_ENABLE_PROCESS_MODE "Compile MPC in process mode (OFF=try extls first)" OFF)

# Detect extls (autopriv) for TLS privatization
pkg_check_modules(EXTLS autopriv)
if(EXTLS_FOUND)
    set(HAVE_EXTLS YES)
    set(EXTLS_CFLAGS "${EXTLS_CFLAGS}")
    set(EXTLS_LIBS "${EXTLS_LIBRARIES}")
else()
    set(HAVE_EXTLS NO)
    set(EXTLS_CFLAGS "")
    set(EXTLS_LIBS "")
endif()

if(MPC_ENABLE_PROCESS_MODE)
    set(MPC_IN_PROCESS_MODE "yes")
    set(MPC_IN_PROCESS_MODE_INT 1)
elseif(NOT HAVE_EXTLS)
    # No extls available, fall back to process mode (matches autotools)
    set(MPC_IN_PROCESS_MODE "yes")
    set(MPC_IN_PROCESS_MODE_INT 1)
else()
    unset(MPC_IN_PROCESS_MODE)
    set(MPC_IN_PROCESS_MODE_INT 0)
endif()

# CMAKE_RT_LIBS and CMAKE_DL_LIBS
set(CMAKE_RT_LIBS "")
if(HAVE_LIBRT)
    set(CMAKE_RT_LIBS rt)
endif()
set(CMAKE_DL_LIBS "")
if(HAVE_LIBDL)
    set(CMAKE_DL_LIBS dl)
endif()

# Launcher program detection
set(MPC_LAUNCHER_PROGRAM "none")
if(MPC_LAUNCHER STREQUAL "HYDRA")
    set(MPC_LAUNCHER_PROGRAM "HYDRA")
endif()

# SRUN support
find_program(SRUN_IN_PATH srun)
set(MPC_HAS_SRUN_SUPPORT NO)
if(SRUN_IN_PATH AND (MPC_LAUNCHER STREQUAL "PMIX" OR MPC_LAUNCHER STREQUAL "PMI1"))
    set(MPC_HAS_SRUN_SUPPORT YES)
endif()

# PRRTE launcher program
if(MPC_LAUNCHER STREQUAL "PMIX")
    find_program(PRTERUN prterun)
    if(NOT PRTERUN)
        find_program(PRTERUN prun)
    endif()
    if(PRTERUN)
        set(MPC_LAUNCHER_PROGRAM "PRRTE")
        set(MPC_LAUNCHER_PROGRAM_PRRTE YES)
    endif()
endif()

# Prefix for configure_file() @prefix@ substitution (matches autoconf)
set(prefix "${CMAKE_INSTALL_PREFIX}")

# PKG paths for pkg-config
set(PKG_DATADIR "${CMAKE_INSTALL_PREFIX}/share/mpcframework/")
set(PKG_INCDIR "${CMAKE_INSTALL_PREFIX}/include/mpcframework/")

# Environment path variables (from autotools)
set(CONFIGURE_PATH "$ENV{PATH}")
set(CONFIGURE_LD_LIBRARY_PATH "$ENV{LD_LIBRARY_PATH}")
set(CONFIGURE_PKG_CONFIG_PATH "$ENV{PKG_CONFIG_PATH}")
set(CONFIGURE_CPATH "$ENV{CPATH}")
set(CONFIGURE_LIBRARY_PATH "$ENV{LIBRARY_PATH}")

# Generation timestamp
string(TIMESTAMP MPC_GENERATION_DATE "%s")

# Package info
set(PACKAGE_BUGREPORT "france@paratools.com")
set(MPC_VERSION_STRING "${MPC_VERSION}${_MPC_VERSION_TAINT}")

# Allocator (default: Standard)
set(MPC_ALLOCATOR "Standard")
set(ALLOC_CFLAGS "")
set(ALLOC_LIBS "")
if(MPC_ENABLE_MPCALLOC)
    pkg_check_modules(MPCALLOC mpcalloc)
    if(MPCALLOC_FOUND)
        set(MPC_ALLOCATOR "mpcalloc")
        set(MPC_Allocator 1)
        add_compile_options(${MPCALLOC_CFLAGS})
        set(MPC_ALLOCATOR_PRELOAD "libmpcalloc.so")
        find_program(ALLOC_PRELOAD_BIN mpcalloc_preload)
        if(ALLOC_PRELOAD_BIN)
            get_filename_component(ALLOC_BIN_DIR "${ALLOC_PRELOAD_BIN}" DIRECTORY)
            get_filename_component(ALLOC_PREFIX "${ALLOC_BIN_DIR}" DIRECTORY)
            set(CANDIDATE_ALLOC_LIB_PATH "${ALLOC_PREFIX}/lib/libmpcalloc.so")
            if(EXISTS "${CANDIDATE_ALLOC_LIB_PATH}")
                set(MPC_ALLOCATOR_PRELOAD "${CANDIDATE_ALLOC_LIB_PATH}")
            endif()
        endif()
        set(ALLOC_CFLAGS "${MPCALLOC_CFLAGS}")
        set(ALLOC_LIBS_CMAKE "${MPCALLOC_LIBRARIES}")
        set(ALLOC_LIBS "")
        foreach(_alloc_dir ${MPCALLOC_LIBRARY_DIRS})
            set(ALLOC_LIBS "${ALLOC_LIBS} -Wl,-rpath=${_alloc_dir} -L${_alloc_dir}")
        endforeach()
        foreach(_alloc_lib ${MPCALLOC_LIBRARIES})
            set(ALLOC_LIBS "${ALLOC_LIBS} -l${_alloc_lib}")
        endforeach()
    else()
        message(WARNING "MPC allocator requested but not found, using Standard")
        unset(MPC_Allocator)
    endif()
else()
    unset(MPC_Allocator)
endif()

# Extls (privatization support)
set(EXTLS_CFLAGS "")
set(EXTLS_LIBS "")

# HWLOC flags from pkg-config
set(HWLOC_CFLAGS "${HWLOC_CFLAGS}")
set(HWLOC_LIBS_CMAKE "${HWLOC_LIBRARIES}")
set(HWLOC_LIBS "")
foreach(_hwloc_dir ${HWLOC_LIBRARY_DIRS})
    set(HWLOC_LIBS "${HWLOC_LIBS} -Wl,-rpath=${_hwloc_dir} -L${_hwloc_dir}")
endforeach()
foreach(_hwloc_lib ${HWLOC_LIBRARIES})
    set(HWLOC_LIBS "${HWLOC_LIBS} -l${_hwloc_lib}")
endforeach()

# OPA flags from pkg-config
set(OPA_CFLAGS "${OPA_CFLAGS}")
set(OPA_LIBS_CMAKE "${OPA_LIBRARIES}")
set(OPA_LIBS "")
foreach(_opa_dir ${OPA_LIBRARY_DIRS})
    set(OPA_LIBS "${OPA_LIBS} -Wl,-rpath=${_opa_dir} -L${_opa_dir}")
endforeach()
foreach(_opa_lib ${OPA_LIBRARIES})
    set(OPA_LIBS "${OPA_LIBS} -l${_opa_lib}")
endforeach()

# Compiler paths (from CMake)
# CC, CXX, FC are set by CMake automatically

# ASAN flags
set(USING_ASAN "no")
set(USING_ASAN_DYNAMIC "no")
if(MPC_ENABLE_DEBUG_MEM)
    set(USING_ASAN "yes")
    set(USING_ASAN_DYNAMIC "yes")
endif()

# DMTCP
if(MPC_ENABLE_DMTCP)
    pkg_check_modules(DMTCP dmtcp)
    if(DMTCP_FOUND)
        set(MPC_HAVE_DMTCP "yes")
    else()
        set(MPC_HAVE_DMTCP "no")
        message(WARNING "DMTCP requested but not found")
    endif()
else()
    set(MPC_HAVE_DMTCP "no")
endif()

# Profiler
if(MPC_ENABLE_PROFILER)
    set(MPC_HAVE_PROFILER "yes")
else()
    set(MPC_HAVE_PROFILER "no")
endif()

# Hydra
find_program(MPIEXEC_HYDRA mpiexec.hydra)

# MPC_LINK_ARGS (empty for now, was computed by autotools)
set(MPC_LINK_ARGS "")

# Compiler variables for configure_file() @CC@, @CXX@, @FC@ substitution
set(CC "${CMAKE_C_COMPILER}")
set(CXX "${CMAKE_CXX_COMPILER}")
if(MPC_ENABLE_FORTRAN)
    set(FC "${CMAKE_Fortran_COMPILER}")
else()
    set(FC "")
endif()

# Fortran -fallow-argument-mismatch check (matches autotools)
if(MPC_ENABLE_FORTRAN)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/fortran_test_check.f90" "PROGRAM foo\nEND PROGRAM\n")
    try_compile(FORTRAN_ARG_MISMATCH_SUPPORTED
        "${CMAKE_CURRENT_BINARY_DIR}/fortran_test_compile"
        "${CMAKE_CURRENT_BINARY_DIR}/fortran_test_check.f90"
        CMAKE_FLAGS "CMAKE_TRY_COMPILE_TARGET_TYPE:STATIC_LIBRARY"
        COMPILE_DEFINITIONS "-fallow-argument-mismatch"
    )
    if(FORTRAN_ARG_MISMATCH_SUPPORTED)
        set(FORTRAN_ARG_MISMATCH_ARG "-fallow-argument-mismatch")
    else()
        set(FORTRAN_ARG_MISMATCH_ARG "")
    endif()
    file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/fortran_test_check.f90")
else()
    set(FORTRAN_ARG_MISMATCH_ARG "")
endif()

# ------------------------------------------------------------------
# Fortran symbol naming convention detection (matches autotools)
# ------------------------------------------------------------------
if(MPC_ENABLE_FORTRAN)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/fortran_name_test.f90" "      subroutine fortran_name()\n      end subroutine\n")
    try_compile(FORTRAN_NAME_COMPILE
        "${CMAKE_CURRENT_BINARY_DIR}/fortran_name_test"
        "${CMAKE_CURRENT_BINARY_DIR}/fortran_name_test.f90"
        CMAKE_FLAGS "CMAKE_TRY_COMPILE_TARGET_TYPE:STATIC_LIBRARY"
    )
    if(FORTRAN_NAME_COMPILE)
        execute_process(
            COMMAND nm "${CMAKE_CURRENT_BINARY_DIR}/fortran_name_test/libfortran_name_test.a"
            OUTPUT_VARIABLE FORTRAN_NAME_SYMBOLS
            ERROR_QUIET
        )
        if(FORTRAN_NAME_SYMBOLS MATCHES "fortran_name__")
            set(FORTRANDOUBLEUNDERSCORE 1)
            message(STATUS "Fortran symbol format: __")
        elseif(FORTRAN_NAME_SYMBOLS MATCHES "fortran_name_")
            set(FORTRANUNDERSCORE 1)
            message(STATUS "Fortran symbol format: _")
        elseif(FORTRAN_NAME_SYMBOLS MATCHES "fortran_name")
            set(FORTRANLOWER 1)
            message(STATUS "Fortran symbol format: lowercase")
        elseif(FORTRAN_NAME_SYMBOLS MATCHES "FORTRAN_NAME")
            set(FORTRANCAPS 1)
            message(STATUS "Fortran symbol format: uppercase")
        else()
            # Default to underscore convention (most common for GCC)
            set(FORTRANUNDERSCORE 1)
            message(STATUS "Fortran symbol format: _ (default)")
        endif()
    else()
        set(FORTRANUNDERSCORE 1)
        message(STATUS "Fortran symbol format: _ (default, compile test failed)")
    endif()
    file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/fortran_name_test.f90")
endif()

# ------------------------------------------------------------------
# Global compile definitions
# ------------------------------------------------------------------
set(MPC_COMPILE_DEFINITIONS
    MPC_VERSION_MAJOR=${MPC_VERSION_MAJOR}
    MPC_VERSION_MINOR=${MPC_VERSION_MINOR}
    MPC_VERSION_PATCH=${MPC_VERSION_PATCH}
    MPC_VERSION_STRING="${MPC_VERSION}${_MPC_VERSION_TAINT}"
    MPC_PREFIX_PATH="${CMAKE_INSTALL_PREFIX}"
    MPC_CONFIG_NSIG=${MPC_CONFIG_NSIG}
    TLS_SUPPORT=${TLS_SUPPORT}
)

if(MPC_ENABLE_THREADS)
    list(APPEND MPC_COMPILE_DEFINITIONS MPC_Threads=1)
    set(HAVE_MPC_THREAD 1)
else()
    set(HAVE_MPC_THREAD 0)
endif()
if(MPC_ENABLE_OPENMP)
    list(APPEND MPC_COMPILE_DEFINITIONS MPC_OpenMP=1)
endif()
if(MPC_ENABLE_LOWCOMM)
    list(APPEND MPC_COMPILE_DEFINITIONS MPC_Lowcomm=1)
endif()
if(MPC_ENABLE_MPI)
    list(APPEND MPC_COMPILE_DEFINITIONS MPC_MPI=1)
endif()
if(MPC_ENABLE_FORTRAN)
    list(APPEND MPC_COMPILE_DEFINITIONS MPC_FORTRAN=1 MPC_FORTRAN_ENABLED=1)
endif()
if(MPC_ENABLE_MPIIO)
    list(APPEND MPC_COMPILE_DEFINITIONS MPC_MPIIO=1)
endif()
if(MPC_ENABLE_PROFILER)
    list(APPEND MPC_COMPILE_DEFINITIONS MPC_Profiler=1)
endif()
if(MPC_CUDA_ENABLED)
    list(APPEND MPC_COMPILE_DEFINITIONS MPC_USE_CUDA=1)
endif()
if(MPC_ROCM_ENABLED)
    list(APPEND MPC_COMPILE_DEFINITIONS MPC_USE_ROCM=1)
endif()
if(MPC_HAVE_AM)
    list(APPEND MPC_COMPILE_DEFINITIONS MPC_HAVE_LIBFFI=1)
endif()
if(MPC_HAVE_LIBVMEM)
    list(APPEND MPC_COMPILE_DEFINITIONS MPC_HAVE_LIBVMEM=1)
endif()
if(MPC_HAVE_LIBUNWIND)
    list(APPEND MPC_COMPILE_DEFINITIONS MPC_HAVE_LIBUNWIND=1)
endif()
if(OFI_DRIVER_ENABLED)
    list(APPEND MPC_COMPILE_DEFINITIONS MPC_USE_OFI=1)
endif()
if(PORTALS4_DRIVER_ENABLED)
    list(APPEND MPC_COMPILE_DEFINITIONS MPC_USE_PORTALS=1)
endif()
if(MPC_ENABLE_SHELL_COLORS)
    list(APPEND MPC_COMPILE_DEFINITIONS MPC_ENABLE_SHELL_COLORS=1)
endif()
if(MPC_ENABLE_CMA AND HAVE_UIO_H)
    list(APPEND MPC_COMPILE_DEFINITIONS MPC_USE_CMA=1)
endif()
if(MPC_ENABLE_DEBUG_MESSAGES)
    list(APPEND MPC_COMPILE_DEFINITIONS MPC_ENABLE_DEBUG_MESSAGES=1)
endif()
if(MPC_ENABLE_WORKSHARE)
    list(APPEND MPC_COMPILE_DEFINITIONS MPC_ENABLE_WORKSHARE=1)
endif()
if(MPC_ENABLE_SLURM)
    list(APPEND MPC_COMPILE_DEFINITIONS MPC_ENABLE_SLURM=1)
endif()
if(MPC_ENABLE_TOPOLOGY_SIMULATION)
    list(APPEND MPC_COMPILE_DEFINITIONS MPC_ENABLE_TOPOLOGY_SIMULATION=1)
endif()
if(MPC_ENABLE_CK)
    list(APPEND MPC_COMPILE_DEFINITIONS MPC_HAVE_CK=1)
endif()
if(MPC_HAVE_DMTCP)
    list(APPEND MPC_COMPILE_DEFINITIONS MPC_USE_DMTCP=1)
endif()
if(HAVE_EXTLS)
    list(APPEND MPC_COMPILE_DEFINITIONS MPC_USE_EXTLS=1)
endif()
if(HAVE_PRAGMA_MESSAGE)
    list(APPEND MPC_COMPILE_DEFINITIONS HAVE_PRAGMA_MESSAGE=1)
endif()
if(HAVE_VISIBILITY)
    list(APPEND MPC_COMPILE_DEFINITIONS HAVE_VISIBILITY=1)
endif()
if(HAVE_ENVIRON_VAR)
    list(APPEND MPC_COMPILE_DEFINITIONS HAVE_ENVIRON_VAR=1)
endif()
if(NOT MPC_ENABLE_DEBUG)
    list(APPEND MPC_COMPILE_DEFINITIONS NDEBUG=1)
endif()

# Launcher defines
if(MPC_LAUNCHER STREQUAL "PMIX")
    list(APPEND MPC_COMPILE_DEFINITIONS MPC_USE_PMIX=1)
elseif(MPC_LAUNCHER STREQUAL "PMI1")
    list(APPEND MPC_COMPILE_DEFINITIONS MPC_USE_PMI1=1)
elseif(MPC_LAUNCHER STREQUAL "HYDRA")
    list(APPEND MPC_COMPILE_DEFINITIONS MPC_USE_HYDRA=1 USE_PMI_PORT=1)
endif()

# KMP arch defines
if(MPC_ARCH_MICROTASK_ARCH STREQUAL "x86_64")
    list(APPEND MPC_COMPILE_DEFINITIONS KMP_ARCH_X86_64=1)
elseif(MPC_ARCH_MICROTASK_ARCH STREQUAL "x86")
    list(APPEND MPC_COMPILE_DEFINITIONS KMP_ARCH_X86=1)
endif()

# Optimizations
if(MPC_ENABLE_OPTIM)
    set(MPC_OPT_FLAGS "-O3")
elseif(MPC_ENABLE_DEBUG)
    set(MPC_OPT_FLAGS "-g -Wall -Wextra")
endif()

# ------------------------------------------------------------------
# Compiler flags (applied globally to all targets)
# ------------------------------------------------------------------

# Native architecture optimization
if(MPC_ENABLE_OPT_NATIVE)
    add_compile_options(-march=native)
endif()

# Debug warnings
if(MPC_ENABLE_DEBUG)
    add_compile_options(-Wall -Wextra)
endif()

# ASAN (Address Sanitizer)
if(MPC_ENABLE_DEBUG_MEM)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

# CMake variables for #cmakedefine in mpc_config.h.in
if(MPC_ENABLE_THREADS)
    set(MPC_THREAD_ENABLED yes)
else()
    set(MPC_THREAD_ENABLED no)
endif()

if(MPC_ENABLE_LOWCOMM)
    set(MPC_LOWCOMM_ENABLED yes)
else()
    set(MPC_LOWCOMM_ENABLED no)
endif()

if(MPC_ENABLE_MPI)
    set(MPC_MPI_ENABLED yes)
else()
    set(MPC_MPI_ENABLED no)
endif()


if(MPC_ENABLE_FORTRAN)
    set(MPC_FORTRAN_ENABLED yes)
else()
    set(MPC_FORTRAN_ENABLED no)
endif()

if(MPC_ENABLE_OPENMP)
    set(MPC_OPENMP_ENABLED yes)
else()
    set(MPC_OPENMP_ENABLED no)
endif()

if(MPC_ENABLE_MPIIO)
    set(MPC_MPIIO_ENABLED yes)
else()
    set(MPC_MPIIO_ENABLED no)
endif()


# ------------------------------------------------------------------
# Generate config headers
# ------------------------------------------------------------------
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/mpc_config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/mpc_config.h"
    @ONLY
)

# Generate mpc_threads_config.h (needed by MPC_Common when threads enabled)
if(MPC_ENABLE_THREADS)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/src/include/mpc_threads_config.h.in"
        "${CMAKE_CURRENT_BINARY_DIR}/src/include/mpc_threads_config.h"
        @ONLY
    )
endif()

# Generate mpc.h (needed by MPC_Lowcomm and others)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/include/mpc.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/src/include/mpc.h"
    @ONLY
)

# MPC_MODULE_LIST (for mpc_cflags script)
set(MPC_MODULE_LIST "")
if(MPC_ENABLE_THREADS)
    set(MPC_MODULE_LIST "${MPC_MODULE_LIST} MPC_THREAD_ENABLED")
endif()
if(MPC_ENABLE_OPENMP)
    set(MPC_MODULE_LIST "${MPC_MODULE_LIST} MPC_OPENMP_ENABLED")
endif()
if(MPC_ENABLE_LOWCOMM)
    set(MPC_MODULE_LIST "${MPC_MODULE_LIST} MPC_LOWCOMM_ENABLED")
endif()
if(MPC_ENABLE_MPI)
    set(MPC_MODULE_LIST "${MPC_MODULE_LIST} MPC_MPI_ENABLED")
endif()
if(MPC_ENABLE_MPIIO)
    set(MPC_MODULE_LIST "${MPC_MODULE_LIST} MPC_MPIIO_ENABLED")
endif()
if(MPC_ENABLE_FORTRAN)
    set(MPC_MODULE_LIST "${MPC_MODULE_LIST} MPC_FORTRAN_ENABLED")
endif()

# ------------------------------------------------------------------
# Summary
# ------------------------------------------------------------------
# MPC_CONFIG_RESULT (for mpc_status script)
set(MPC_CONFIG_RESULT "
########## MPC Configuration Script ##########
Installation prefix: ${CMAKE_INSTALL_PREFIX}
Version: ${MPC_VERSION}
Architecture: ${MPC_TARGET_ARCHITECTURE}
Threads: ${MPC_ENABLE_THREADS}
OpenMP: ${MPC_ENABLE_OPENMP}
Lowcomm: ${MPC_ENABLE_LOWCOMM}
MPI: ${MPC_ENABLE_MPI}
Fortran: ${MPC_ENABLE_FORTRAN}
MPI-IO: ${MPC_ENABLE_MPIIO}
Profiler: ${MPC_ENABLE_PROFILER}
CUDA: ${MPC_CUDA_ENABLED}
ROCM: ${MPC_ROCM_ENABLED}
Active Messages: ${MPC_HAVE_AM}
Launcher: ${MPC_LAUNCHER}
Networks: ${MPC_NETWORK_LIST}
CMA: ${MPC_ENABLE_CMA}
SLURM: ${MPC_ENABLE_SLURM}
Topology simulation: ${MPC_ENABLE_TOPOLOGY_SIMULATION}
CK: ${MPC_ENABLE_CK}
MPC Allocator: ${MPC_ENABLE_MPCALLOC}
DMTCP: ${MPC_HAVE_DMTCP}
Optimizations: ${MPC_OPT_FLAGS}
#############################################
")

# ------------------------------------------------------------------
# Summary
# ------------------------------------------------------------------
message(STATUS "")
message(STATUS "MPC ${MPC_VERSION} Configuration Summary")
message(STATUS "  Install prefix : ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Architecture   : ${MPC_TARGET_ARCHITECTURE}")
message(STATUS "  Context switch : ${MPC_ARCH_CONTEXT_SWITCH}")
message(STATUS "  Microtask arch : ${MPC_ARCH_MICROTASK_ARCH}")
message(STATUS "  Threads        : ${MPC_ENABLE_THREADS}")
message(STATUS "  OpenMP         : ${MPC_ENABLE_OPENMP}")
message(STATUS "  Lowcomm        : ${MPC_ENABLE_LOWCOMM}")
message(STATUS "  MPI            : ${MPC_ENABLE_MPI}")
message(STATUS "  Fortran        : ${MPC_ENABLE_FORTRAN}")
message(STATUS "  MPI-IO         : ${MPC_ENABLE_MPIIO}")
message(STATUS "  Profiler       : ${MPC_ENABLE_PROFILER}")
message(STATUS "  CUDA           : ${MPC_CUDA_ENABLED}")
message(STATUS "  ROCM           : ${MPC_ROCM_ENABLED}")
message(STATUS "  Active Messages: ${MPC_HAVE_AM}")
message(STATUS "  Launcher       : ${MPC_LAUNCHER}")
message(STATUS "  Networks       : ${MPC_NETWORK_LIST}")
message(STATUS "  CMA            : ${MPC_ENABLE_CMA}")
message(STATUS "  SLURM          : ${MPC_ENABLE_SLURM}")
message(STATUS "  Topo sim       : ${MPC_ENABLE_TOPOLOGY_SIMULATION}")
message(STATUS "  CK             : ${MPC_ENABLE_CK}")
message(STATUS "  MPC Alloc      : ${MPC_ENABLE_MPCALLOC}")
message(STATUS "  DMTCP          : ${MPC_HAVE_DMTCP}")
message(STATUS "  Optimizations  : ${MPC_OPT_FLAGS}")
message(STATUS "")

# ------------------------------------------------------------------
# Subdirectories
# ------------------------------------------------------------------
add_subdirectory(src)
add_subdirectory(pkgconfig)
