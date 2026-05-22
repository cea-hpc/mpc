# bin/CMakeLists.txt
# Installs compiler wrapper scripts and builds mpc_sets binary

# Compiler wrapper scripts
set(MPC_BIN_SCRIPTS
    mpc_cc
    mpc_cxx
    mpc_cflags
    mpc_ldflags
    mpc_status
    mpcrun
    tmux_gdb
)

# MPI scripts (conditional)
if(MPC_ENABLE_MPI)
    list(APPEND MPC_BIN_SCRIPTS
        mpicc
        mpicxx
        "mpic++"
    )
endif()

# Fortran scripts (conditional)
if(MPC_ENABLE_FORTRAN)
    list(APPEND MPC_BIN_SCRIPTS mpc_f77)
    if(MPC_ENABLE_MPI)
        list(APPEND MPC_BIN_SCRIPTS mpif77 mpif90)
    endif()
endif()

# CUDA support (conditional)
if(MPC_CUDA_ENABLED)
    list(APPEND MPC_BIN_SCRIPTS mpc_nvcc)
endif()

# HIP support (conditional)
if(MPC_ROCM_ENABLED)
    list(APPEND MPC_BIN_SCRIPTS mpc_hipcc)
endif()

# mpirun handling
if(NOT MPC_LAUNCHER_PROGRAM_PRRTE)
    list(APPEND MPC_BIN_SCRIPTS mpirun)
endif()

# Process script templates
set(INSTALL_SCRIPTS "")
foreach(_SCRIPT IN LISTS MPC_BIN_SCRIPTS)
    set(_SCRIPT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/${_SCRIPT}")
    set(_SCRIPT_DST "${CMAKE_CURRENT_BINARY_DIR}/${_SCRIPT}")

    # Check if there's a .in template
    if(EXISTS "${_SCRIPT_SRC}.in")
        configure_file("${_SCRIPT_SRC}.in" "${_SCRIPT_DST}" @ONLY)
    else()
        configure_file("${_SCRIPT_SRC}" "${_SCRIPT_DST}" COPYONLY)
    endif()

    list(APPEND INSTALL_SCRIPTS "${_SCRIPT_DST}")
endforeach()

# Install scripts
install(PROGRAMS ${INSTALL_SCRIPTS}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# mpcvars.sh goes to install prefix root and bin/ (matches autotools)
set(_MPCVARS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/mpcvars.sh")
set(_MPCVARS_DST "${CMAKE_CURRENT_BINARY_DIR}/mpcvars.sh")
if(EXISTS "${_MPCVARS_SRC}.in")
    configure_file("${_MPCVARS_SRC}.in" "${_MPCVARS_DST}" @ONLY)
else()
    configure_file("${_MPCVARS_SRC}" "${_MPCVARS_DST}" COPYONLY)
endif()
install(PROGRAMS "${_MPCVARS_DST}"
    DESTINATION "."
)
install(PROGRAMS "${_MPCVARS_DST}"
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install mpc_compiler_common.sh to pkgdatadir
install(FILES mpc_compiler_common.sh
    DESTINATION ${CMAKE_INSTALL_DATADIR}/mpcframework
)

# mpcrun plugins
set(MPCRUN_PLUGINS
    mpcrun_opt/mpcrun_none
    mpcrun_opt/mpcrun_none_mpc-gdb
)

if(MPC_HAS_SRUN_SUPPORT)
    list(APPEND MPCRUN_PLUGINS mpcrun_opt/mpcrun_srun)
endif()

if(MPC_LAUNCHER STREQUAL "HYDRA")
    list(APPEND MPCRUN_PLUGINS
        mpcrun_opt/mpcrun_hydra
        mpcrun_opt/mpcrun_salloc_hydra
    )
endif()

if(MPC_LAUNCHER_PROGRAM_PRRTE)
    list(APPEND MPCRUN_PLUGINS mpcrun_opt/mpcrun_prrte)
endif()

# Process and install mpcrun plugins
foreach(_PLUGIN IN LISTS MPCRUN_PLUGINS)
    set(_PLUGIN_SRC "${CMAKE_CURRENT_SOURCE_DIR}/${_PLUGIN}")
    set(_PLUGIN_DST "${CMAKE_CURRENT_BINARY_DIR}/${_PLUGIN}")

    if(EXISTS "${_PLUGIN_SRC}.in")
        configure_file("${_PLUGIN_SRC}.in" "${_PLUGIN_DST}" @ONLY)
    else()
        configure_file("${_PLUGIN_SRC}" "${_PLUGIN_DST}" COPYONLY)
    endif()

    install(PROGRAMS "${_PLUGIN_DST}"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/mpcframework/mpcrun_opt
    )
endforeach()

# mpc_sets binary (conditional)
# Deferred: circular dependency between mpcthread and mpclowcomm
# if(MPC_ENABLE_LOWCOMM)
#     add_executable(mpc_sets mpc_sets.c)
#
#     target_include_directories(mpc_sets
#         PRIVATE
#             ${CMAKE_SOURCE_DIR}/src/MPC_Lowcomm/include
#             ${CMAKE_SOURCE_DIR}/src/MPC_Common/include
#             ${CMAKE_BINARY_DIR}/src/MPC_Common/include
#             ${CMAKE_SOURCE_DIR}/src/include
#             ${CMAKE_BINARY_DIR}/src/include
#             ${CMAKE_SOURCE_DIR}/src/MPC_MPI/include
#     )
#
#     set(ALL_MPC_LIBS mpcconfig)
#     if(MPC_ENABLE_THREADS)
#         list(APPEND ALL_MPC_LIBS mpcthread)
#     endif()
#     if(MPC_ENABLE_OPENMP)
#         list(APPEND ALL_MPC_LIBS mpcomp)
#     endif()
#     if(MPC_ENABLE_LOWCOMM)
#         list(APPEND ALL_MPC_LIBS mpclowcomm)
#     endif()
#     if(MPC_ENABLE_MPI)
#         list(APPEND ALL_MPC_LIBS mpcmpi)
#     endif()
#
#     target_link_libraries(mpc_sets PRIVATE ${ALL_MPC_LIBS})
#
#     install(TARGETS mpc_sets
#         RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#     )
# endif()
