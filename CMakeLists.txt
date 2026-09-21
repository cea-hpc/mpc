cmake_minimum_required(VERSION 3.20.3)

# ------------------------------------------------------------------
# Version extraction using config/mpc_get_version.sh
# ------------------------------------------------------------------
set(_MPC_VERSION_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/config/mpc_get_version.sh")
execute_process(
    COMMAND bash ${_MPC_VERSION_SCRIPT} --major
    OUTPUT_VARIABLE _MPC_VERSION_MAJOR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND bash ${_MPC_VERSION_SCRIPT} --minor
    OUTPUT_VARIABLE _MPC_VERSION_MINOR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND bash ${_MPC_VERSION_SCRIPT} --patch
    OUTPUT_VARIABLE _MPC_VERSION_PATCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND bash ${_MPC_VERSION_SCRIPT} --taint
    OUTPUT_VARIABLE _MPC_VERSION_TAINT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(MPC_VERSION_MAJOR "${_MPC_VERSION_MAJOR}")
set(MPC_VERSION_MINOR "${_MPC_VERSION_MINOR}")
set(MPC_VERSION_PATCH "${_MPC_VERSION_PATCH}")
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

project(
    MPC
    VERSION ${_MPC_VERSION_MAJOR}.${_MPC_VERSION_MINOR}.${_MPC_VERSION_PATCH}
    DESCRIPTION "Multi-Processor Computing Framework"
    HOMEPAGE_URL "https://mpc.hpcframework.com"
    LANGUAGES C CXX
)

# Enable ASM language for architecture-specific assembly sources
enable_language(ASM)

# Fortran is optionally activated after optional dependencies enforcement

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

set(MPC_SUPPORTED_ARCHITECTURES
    "x86_64"
    "ia64"
    "i686"
    "arm"
    "aarch64"
    "powerpc64"
)
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
option(MPC_ENABLE_OPENSHMEM "Enable OpenSHMEM interface" ON)
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
option(MPC_ENABLE_TOPOLOGY_SIMULATION "Enable topology simulation" OFF)
option(MPC_ENABLE_MPCALLOC "Enable MPC allocator" ON)
set(MPC_ENABLE_OFI "AUTO" CACHE STRING "Enable libfabric/OFI: AUTO (auto-detect), ON, or OFF")
set_property(CACHE MPC_ENABLE_OFI PROPERTY STRINGS AUTO ON OFF)

# BXI version (optional, for Portals specialization)
set(MPC_BXI_VERSION "" CACHE STRING "BXI version for Portals specialization (2 or 3)")

# Dependency prefixes (overridden by installmpc or user)
set(MPC_HWLOC_PREFIX "" CACHE PATH "hwloc install prefix")
set(MPC_OPENPA_PREFIX "" CACHE PATH "openpa install prefix")
set(MPC_PMIX_PREFIX "" CACHE PATH "PMIx install prefix")
set(MPC_PMI_PREFIX "" CACHE PATH "PMI1 install prefix")
set(MPC_HYDRA_PREFIX "" CACHE PATH "Hydra launcher install prefix")
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
    set(MPC_ENABLE_OPENSHMEM OFF)
endif()

if(NOT MPC_ENABLE_MPI)
    message(WARNING "Disabling MPI means disabling Fortran and MPIIO")
    set(MPC_ENABLE_FORTRAN OFF)
    set(MPC_ENABLE_MPIIO OFF)
endif()

if(MPC_ENABLE_MPI AND NOT MPC_ENABLE_LOWCOMM)
    message(FATAL_ERROR "MPI requires Lowcomm (enable MPC_ENABLE_LOWCOMM)")
endif()

if(MPC_ENABLE_OPENMP AND NOT MPC_ENABLE_THREADS)
    message(FATAL_ERROR "OpenMP requires threads (enable MPC_ENABLE_THREADS)")
endif()

if(MPC_ENABLE_MPI AND NOT MPC_ENABLE_THREADS)
    message(FATAL_ERROR "MPI requires threads (enable MPC_ENABLE_THREADS)")
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

# ==================================================================
# External modules (see cmake/*.cmake)
# ==================================================================
include(cmake/Dependencies.cmake)
include(cmake/CompilerChecks.cmake)
include(cmake/RuntimeConstants.cmake)
include(cmake/ConfigVariables.cmake)
include(cmake/FortranSetup.cmake)
include(cmake/CompileDefinitions.cmake)

# ------------------------------------------------------------------
# Generate config headers
# ------------------------------------------------------------------
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/mpc_config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/mpc_config.h" @ONLY)

# Generate mpc_threads_config.h (needed by MPC_Common when threads enabled)
if(MPC_ENABLE_THREADS)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/src/include/mpc_threads_config.h.in"
        "${CMAKE_CURRENT_BINARY_DIR}/src/include/mpc_threads_config.h"
        @ONLY
    )
endif()

# Generate mpc.h (needed by MPC_Lowcomm and others)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/include/mpc.h.in" "${CMAKE_CURRENT_BINARY_DIR}/src/include/mpc.h" @ONLY)

# ------------------------------------------------------------------
# Summary (setup)
# ------------------------------------------------------------------
string(JOIN " " MPC_OPT_FLAGS_STR ${MPC_OPT_FLAGS})
set(MPC_CONFIG_RESULT
    "
########## MPC Configuration Summary ##########
  Install prefix : ${CMAKE_INSTALL_PREFIX}
  Architecture   : ${MPC_TARGET_ARCHITECTURE}
  Threads        : ${MPC_ENABLE_THREADS}
  OpenMP         : ${MPC_ENABLE_OPENMP}
  Lowcomm        : ${MPC_ENABLE_LOWCOMM}
  MPI            : ${MPC_ENABLE_MPI}
  OpenSHMEM      : ${MPC_ENABLE_OPENSHMEM}
  Fortran        : ${MPC_ENABLE_FORTRAN}
  MPI-IO         : ${MPC_ENABLE_MPIIO}
  Profiler       : ${MPC_ENABLE_PROFILER}
  CUDA           : ${MPC_CUDA_ENABLED}
  ROCM           : ${MPC_ROCM_ENABLED}
  Active Messages: ${MPC_HAVE_AM}
  Launcher       : ${MPC_LAUNCHER}
  Networks       : ${MPC_NETWORK_LIST}
  CMA            : ${MPC_ENABLE_CMA}
  Topo sim       : ${MPC_ENABLE_TOPOLOGY_SIMULATION}
  MPC Alloc      : ${MPC_ENABLE_MPCALLOC}
  DMTCP          : ${MPC_HAVE_DMTCP}
  Compil Flags   : ${MPC_OPT_FLAGS_STR}
#############################################
"
)

# ------------------------------------------------------------------
# Subdirectories
# ------------------------------------------------------------------
add_subdirectory(src)
add_subdirectory(pkgconfig)

# ------------------------------------------------------------------
# Install CMake package config and example files
# ------------------------------------------------------------------
install(FILES cmake/mpcframework-config.cmake DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/mpcframework/cmake)
install(
    FILES cmake/example/CMakeLists.txt cmake/example/t.c
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/mpcframework/cmake/example
)

# ------------------------------------------------------------------
# Summary (display)
# ------------------------------------------------------------------
message(STATUS "")
message(STATUS "MPC ${MPC_VERSION} Configuration Summary")
message("${MPC_CONFIG_RESULT}")
message(STATUS "")
