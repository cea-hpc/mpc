# ==================================================================
# ConfigVariables.cmake — Variables for configure_file() substitution
# ==================================================================

# ------------------------------------------------------------------
# Paths and environment
# ------------------------------------------------------------------
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

# Package info
set(PACKAGE_BUGREPORT "https://github.com/cea-hpc/mpc/issues")
set(MPC_VERSION_STRING "${MPC_VERSION}${_MPC_VERSION_TAINT}")

# ------------------------------------------------------------------
# Library flags
# ------------------------------------------------------------------
# CMAKE_RT_LIBS and CMAKE_DL_LIBS
set(CMAKE_RT_LIBS "")
if(HAVE_LIBRT)
    set(CMAKE_RT_LIBS rt)
endif()
set(CMAKE_DL_LIBS "")
if(HAVE_LIBDL)
    set(CMAKE_DL_LIBS dl)
endif()

# ------------------------------------------------------------------
# Allocator
# ------------------------------------------------------------------
set(MPC_ALLOCATOR "Standard")
set(ALLOC_CFLAGS "")
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
    else()
        message(WARNING "MPC allocator requested but not found, using Standard")
        unset(MPC_Allocator)
    endif()
else()
    unset(MPC_Allocator)
endif()

# ------------------------------------------------------------------
# DMTCP
# ------------------------------------------------------------------
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

# ------------------------------------------------------------------
# Profiler
# ------------------------------------------------------------------
if(MPC_ENABLE_PROFILER)
    set(MPC_HAVE_PROFILER "yes")
else()
    set(MPC_HAVE_PROFILER "no")
endif()

# ------------------------------------------------------------------
# Misc
# ------------------------------------------------------------------
# Hydra
if(MPC_HYDRA_PREFIX)
    find_program(MPIEXEC_HYDRA mpiexec.hydra HINTS "${MPC_HYDRA_PREFIX}/bin")
else()
    find_program(MPIEXEC_HYDRA mpiexec.hydra)
endif()

# MPC_LINK_ARGS (empty for now, was computed by autotools)
set(MPC_LINK_ARGS "")

# Compiler paths (from CMake)
# CC, CXX, FC are set by CMake automatically

# ASAN flags
set(USING_ASAN "no")
set(USING_ASAN_DYNAMIC "no")
if(MPC_ENABLE_DEBUG_MEM)
    set(USING_ASAN "yes")
    set(USING_ASAN_DYNAMIC "yes")
endif()

# Compiler variables for configure_file() @CC@, @CXX@, @FC@ substitution
set(CC "${CMAKE_C_COMPILER}")
set(CXX "${CMAKE_CXX_COMPILER}")
if(MPC_ENABLE_FORTRAN)
    set(FC "${CMAKE_Fortran_COMPILER}")
else()
    set(FC "")
endif()
