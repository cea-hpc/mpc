# ==================================================================
# CompileDefinitions.cmake — Compile definitions, flags, and module setup
# ==================================================================

# ------------------------------------------------------------------
# Global compile definitions
# ------------------------------------------------------------------
set(MPC_COMPILE_DEFINITIONS "")

if(MPC_ENABLE_THREADS)
    set(MPC_Threads 1)
    list(APPEND MPC_COMPILE_DEFINITIONS HAVE_MPC_THREAD=1)
    set(HAVE_MPC_THREAD 1)
else()
    set(MPC_Threads 0)
    list(APPEND MPC_COMPILE_DEFINITIONS HAVE_MPC_THREAD=0)
    set(HAVE_MPC_THREAD 0)
endif()
if(MPC_ENABLE_OPENMP)
    set(MPC_OpenMP 1)
else()
    set(MPC_OpenMP 0)
endif()
if(MPC_ENABLE_LOWCOMM)
    set(MPC_Lowcomm 1)
else()
    set(MPC_Lowcomm 0)
endif()

if(MPC_ENABLE_FORTRAN)
    set(MPC_FORTRAN 1)
else()
    set(MPC_FORTRAN 0)
endif()

if(MPC_ENABLE_PROFILER)
    set(MPC_Profiler 1)
else()
    set(MPC_Profiler 0)
endif()
if(MPC_CUDA_ENABLED)
    set(MPC_USE_CUDA 1)
else()
    set(MPC_USE_CUDA 0)
endif()
if(MPC_ROCM_ENABLED)
    set(MPC_USE_ROCM 1)
else()
    set(MPC_USE_ROCM 0)
endif()



if(PORTALS4_DRIVER_ENABLED)
    set(MPC_USE_PORTALS 1)
endif()

if(MPC_ENABLE_CMA AND HAVE_UIO_H)
    set(MPC_USE_CMA 1)
endif()

if(MPC_HAVE_DMTCP)
    set(MPC_USE_DMTCP 1)
endif()

# PTHREAD_STACK_MIN (not in autotools mpc_config.h.in)
list(APPEND MPC_COMPILE_DEFINITIONS "MPC_CONFIG_PTHREAD_STACK_MIN=${MPC_CONFIG_PTHREAD_STACK_MIN}")

# MPC_ARCH_DETECTED_FUNCTION (not in autotools mpc_config.h.in)
list(APPEND MPC_COMPILE_DEFINITIONS "MPC_ARCH_DETECTED_FUNCTION=${MPC_ARCH_DETECTED_FUNCTION}")

# MPC_ARCH_DETECTED_STRING (not in autotools mpc_config.h.in)
list(APPEND MPC_COMPILE_DEFINITIONS "MPC_ARCH_DETECTED_STRING=${MPC_ARCH_DETECTED_STRING}")

# Launcher defines (set CMake variables for #cmakedefine in mpc_config.h.in)
if(MPC_LAUNCHER STREQUAL "PMIX")
    set(MPC_USE_PMIX 1)
elseif(MPC_LAUNCHER STREQUAL "PMI1")
    set(MPC_USE_PMI1 1)
elseif(MPC_LAUNCHER STREQUAL "HYDRA")
    set(MPC_USE_HYDRA 1)
    set(USE_PMI_PORT 1)
endif()

# KMP arch defines
if(MPC_ARCH_MICROTASK_ARCH STREQUAL "x86_64")
    set(KMP_ARCH_X86_64 1)
elseif(MPC_ARCH_MICROTASK_ARCH STREQUAL "x86")
    set(KMP_ARCH_X86 1)
endif()

# ------------------------------------------------------------------
# Compiler flags (applied globally to all targets)
# ------------------------------------------------------------------

# Optimizations
set(MPC_OPT_FLAGS "")
if(MPC_ENABLE_OPTIM)
    list(APPEND MPC_OPT_FLAGS "-O3")
endif()

# Debug
if(MPC_ENABLE_DEBUG)
    list(APPEND MPC_OPT_FLAGS "-g" "-Wall" "-Wextra")
endif()

# Native architecture optimization
if(MPC_ENABLE_OPT_NATIVE)
    list(APPEND MPC_OPT_FLAGS "-march=native")
endif()

# ASAN (Address Sanitizer)
if(MPC_ENABLE_DEBUG_MEM)
    list(APPEND MPC_OPT_FLAGS "-fsanitize=address" "-fno-omit-frame-pointer")
    add_link_options(-fsanitize=address)
endif()

# Apply all accumulated flags
if(MPC_OPT_FLAGS)
    add_compile_options(${MPC_OPT_FLAGS})
endif()

# ------------------------------------------------------------------
# CMake MPC MODULES setup
# ------------------------------------------------------------------

# CMake variables for #cmakedefine in mpc_config.h.in
if(MPC_ENABLE_THREADS)
    set(MPC_THREAD_ENABLED yes)
else()
    set(MPC_THREAD_ENABLED no)
endif()

if(MPC_ENABLE_OPENMP)
    set(MPC_OPENMP_ENABLED yes)
else()
    set(MPC_OPENMP_ENABLED no)
endif()

if(MPC_ENABLE_LOWCOMM)
    set(MPC_LOWCOMM_ENABLED yes)
else()
    set(MPC_LOWCOMM_ENABLED no)
endif()

if(MPC_ENABLE_MPI)
    set(MPC_MPI_ENABLED yes)
    set(MPC_MPI 1)
else()
    set(MPC_MPI_ENABLED no)
    set(MPC_MPI 0)
endif()

if(MPC_ENABLE_FORTRAN)
    set(MPC_FORTRAN_ENABLED yes)
else()
    set(MPC_FORTRAN_ENABLED no)
endif()

if(MPC_ENABLE_MPIIO)
    set(MPC_MPIIO_ENABLED yes)
    set(MPC_MPIIO 1)
else()
    set(MPC_MPIIO_ENABLED no)
    set(MPC_MPIIO 0)
endif()

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
