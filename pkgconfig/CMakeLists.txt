# pkgconfig/CMakeLists.txt
# Generate and install .pc files for pkg-config

set(PKG_INCDIR "${CMAKE_INSTALL_PREFIX}/include/mpcframework")
set(PKG_LIBDIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")

# Thread pkg-config dependency
set(MPC_THREAD_PKGCONFIG_DEP "")
if(MPC_ENABLE_THREADS)
    set(MPC_THREAD_PKGCONFIG_DEP "libmpcthread = ${MPC_VERSION}")
endif()

# IO pkg-config lib
set(MPC_IO_PKGCONFIG_LIB "")
if(MPC_ENABLE_MPIIO)
    set(MPC_IO_PKGCONFIG_LIB "-lmpcio")
endif()

# Fortran pkg-config
set(MPC_FORTRAN_PKGCONFIG_LIB "")
set(MPC_FORTRAN_PKGCONFIG_CFLAGS "")
if(MPC_ENABLE_FORTRAN)
    set(MPC_FORTRAN_PKGCONFIG_LIB "-lmpcfortran")
    set(MPC_FORTRAN_PKGCONFIG_CFLAGS "-I${CMAKE_INSTALL_PREFIX}/include/mpcframework/fortran/")
endif()

# Generate .pc files
configure_file(libmpcthread.pc.in "${CMAKE_CURRENT_BINARY_DIR}/libmpcthread.pc" @ONLY)
configure_file(libmpclowcomm.pc.in "${CMAKE_CURRENT_BINARY_DIR}/libmpclowcomm.pc" @ONLY)
configure_file(libmpcmpi.pc.in "${CMAKE_CURRENT_BINARY_DIR}/libmpcmpi.pc" @ONLY)

# Install .pc files
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/libmpcthread.pc"
        "${CMAKE_CURRENT_BINARY_DIR}/libmpclowcomm.pc"
        "${CMAKE_CURRENT_BINARY_DIR}/libmpcmpi.pc"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
