# ==================================================================
# Dependencies.cmake — Find external dependencies
# ==================================================================

# ------------------------------------------------------------------
# CMake modules and helper target
# ------------------------------------------------------------------
include(GNUInstallDirs)
include(CheckIncludeFile)
include(CheckLibraryExists)
include(CheckSymbolExists)
include(CheckTypeSize)
include(CheckFunctionExists)
include(CheckCSourceCompiles)
include(CheckCSourceRuns)
include(FindPackageHandleStandardArgs)

# Global mpc_config.h include: generated in build root, needed by all subprojects
add_library(mpc_config_headers INTERFACE)
target_include_directories(mpc_config_headers INTERFACE ${CMAKE_BINARY_DIR})

find_package(PkgConfig REQUIRED)

# ------------------------------------------------------------------
# Required dependencies
# ------------------------------------------------------------------
# hwloc (REQUIRED)
pkg_check_modules(HWLOC REQUIRED hwloc>=2.0.0)

# openpa (OPA)
pkg_check_modules(OPA openpa)
if(NOT OPA_FOUND)
    message(FATAL_ERROR "openpa library not found. Install openpa or provide -DOPA_INCLUDE_DIR/-DOPA_LIBRARY")
endif()

# pthread
find_package(Threads REQUIRED)

# Standard system libraries
check_library_exists(m log "" HAVE_LIBM)
check_library_exists(rt clock_gettime "" HAVE_LIBRT)
check_library_exists(dl dlopen "" HAVE_LIBDL)

# ------------------------------------------------------------------
# Optional dependencies
# ------------------------------------------------------------------
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

# ------------------------------------------------------------------
# Network drivers
# ------------------------------------------------------------------
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

# ------------------------------------------------------------------
# ROMIO template variables (for mpcio.h.in)
# ------------------------------------------------------------------
set(DEFINE_HAVE_MPI_GREQUEST "#define HAVE_MPI_GREQUEST")
set(DEFINE_MPI_OFFSET "/* MPI OFFSET is in mpi.h */")
set(NEEDS_MPI_FINT "")
set(HAVE_MPI_INFO "#define HAVE_MPI_INFO")
set(HAVE_MPI_DARRAY_SUBARRAY "#define HAVE_MPI_DARRAY_SUBARRAY")

# ------------------------------------------------------------------
# GPU support
# ------------------------------------------------------------------
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

# ------------------------------------------------------------------
# Launcher dependencies
# ------------------------------------------------------------------
# PMIx (optional, for launcher)
if(MPC_LAUNCHER STREQUAL "PMIX")
    pkg_check_modules(PMIX REQUIRED pmix)
elseif(MPC_LAUNCHER STREQUAL "PMI1")
    pkg_check_modules(PMI REQUIRED pmi)
endif()
