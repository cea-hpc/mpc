# ==================================================================
# CompileDefinitions.cmake — Compile definitions, flags, and module setup
# ==================================================================

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
else()
    set(MPC_MPI_ENABLED no)
endif()

if(MPC_ENABLE_FORTRAN)
    set(MPC_FORTRAN_ENABLED yes)
else()
    set(MPC_FORTRAN_ENABLED no)
endif()

if(MPC_ENABLE_MPIIO)
    set(MPC_MPIIO_ENABLED yes)
else()
    set(MPC_MPIIO_ENABLED no)
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
