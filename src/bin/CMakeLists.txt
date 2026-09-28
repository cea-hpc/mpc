# bin/CMakeLists.txt
# Installs compiler wrapper scripts and builds mpc_sets binary

# Build *_LIBS strings for mpc_ldflags.in template
set(OPA_LIBS "")
foreach(_opa_dir ${OPA_LIBRARY_DIRS})
    set(OPA_LIBS "${OPA_LIBS} -Wl,-rpath=${_opa_dir} -L${_opa_dir}")
endforeach()
foreach(_opa_lib ${OPA_LIBRARIES})
    set(OPA_LIBS "${OPA_LIBS} -l${_opa_lib}")
endforeach()

set(HWLOC_LIBS "")
foreach(_hwloc_dir ${HWLOC_LIBRARY_DIRS})
    set(HWLOC_LIBS "${HWLOC_LIBS} -Wl,-rpath=${_hwloc_dir} -L${_hwloc_dir}")
endforeach()
foreach(_hwloc_lib ${HWLOC_LIBRARIES})
    set(HWLOC_LIBS "${HWLOC_LIBS} -l${_hwloc_lib}")
endforeach()

set(ALLOC_LIBS "")
if(MPCALLOC_FOUND)
    foreach(_alloc_dir ${MPCALLOC_LIBRARY_DIRS})
        set(ALLOC_LIBS "${ALLOC_LIBS} -Wl,-rpath=${_alloc_dir} -L${_alloc_dir}")
    endforeach()
    foreach(_alloc_lib ${MPCALLOC_LIBRARIES})
        set(ALLOC_LIBS "${ALLOC_LIBS} -l${_alloc_lib}")
    endforeach()
endif()

set(EXTLS_LIBS "")
if(HAVE_EXTLS)
    foreach(_extls_dir ${EXTLS_LIBRARY_DIRS})
        set(EXTLS_LIBS "${EXTLS_LIBS} -Wl,-rpath=${_extls_dir} -L${_extls_dir}")
    endforeach()
    foreach(_extls_lib ${EXTLS_LIBRARIES})
        set(EXTLS_LIBS "${EXTLS_LIBS} -l${_extls_lib}")
    endforeach()
endif()

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
    list(APPEND MPC_BIN_SCRIPTS mpicc mpicxx "mpic++")
endif()

# OpenSHMEM scripts (conditional)
if(MPC_ENABLE_OPENSHMEM)
    list(APPEND MPC_BIN_SCRIPTS oshcc oshcxx "oshc++" oshrun)
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
install(PROGRAMS ${INSTALL_SCRIPTS} DESTINATION ${CMAKE_INSTALL_BINDIR})

# mpcvars.sh goes to install prefix root and bin/ (matches autotools)
set(_MPCVARS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/mpcvars.sh")
set(_MPCVARS_DST "${CMAKE_CURRENT_BINARY_DIR}/mpcvars.sh")
if(EXISTS "${_MPCVARS_SRC}.in")
    configure_file("${_MPCVARS_SRC}.in" "${_MPCVARS_DST}" @ONLY)
else()
    configure_file("${_MPCVARS_SRC}" "${_MPCVARS_DST}" COPYONLY)
endif()
install(PROGRAMS "${_MPCVARS_DST}" DESTINATION ".")
install(PROGRAMS "${_MPCVARS_DST}" DESTINATION ${CMAKE_INSTALL_BINDIR})

# Install mpc_compiler_common.sh to pkgdatadir
install(FILES mpc_compiler_common.sh DESTINATION ${CMAKE_INSTALL_DATADIR}/mpcframework)

# mpcrun plugins
set(MPCRUN_PLUGINS mpcrun_opt/mpcrun_none mpcrun_opt/mpcrun_none_mpc-gdb)

if(MPC_HAS_SRUN_SUPPORT)
    list(APPEND MPCRUN_PLUGINS mpcrun_opt/mpcrun_srun)
endif()

if(MPC_LAUNCHER STREQUAL "HYDRA")
    list(APPEND MPCRUN_PLUGINS mpcrun_opt/mpcrun_hydra mpcrun_opt/mpcrun_salloc_hydra)
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

    install(PROGRAMS "${_PLUGIN_DST}" DESTINATION ${CMAKE_INSTALL_DATADIR}/mpcframework/mpcrun_opt)
endforeach()

# mpc_print_config binary
add_executable(mpc_print_config ${CMAKE_SOURCE_DIR}/src/MPC_Config/src/print_config.c)
target_include_directories(
    mpc_print_config
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/MPC_Config/include
        ${CMAKE_SOURCE_DIR}/src/MPC_Arch/include
        ${CMAKE_BINARY_DIR}/src/MPC_Arch/include
        ${CMAKE_SOURCE_DIR}/src/include
        ${CMAKE_BINARY_DIR}/src/include
        ${CMAKE_BINARY_DIR}
)
target_link_libraries(
    mpc_print_config
    PRIVATE
        mpcconfig
        mpcarch
        ${OPA_LIBRARIES}
        ${OFI_LIBRARIES}
        ${CMAKE_RT_LIBS}
        Threads::Threads
        -Wl,--no-as-needed
        mpcmpi
        mpclowcomm
        mpcomp
        mpcthread
        -Wl,--as-needed
)
if(MPCALLOC_FOUND)
    target_include_directories(mpc_print_config PRIVATE ${MPCALLOC_INCLUDE_DIRS})
    target_link_libraries(mpc_print_config PRIVATE ${MPCALLOC_LDFLAGS})
endif()
install(TARGETS mpc_print_config RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# mpc_sets binary (conditional)
if(MPC_ENABLE_LOWCOMM)
    add_executable(mpc_sets mpc_sets.c)

    target_include_directories(
        mpc_sets
        PRIVATE
            ${CMAKE_BINARY_DIR}
            ${CMAKE_SOURCE_DIR}/src/MPC_Lowcomm/include
            ${CMAKE_SOURCE_DIR}/src/MPC_Common/include
            ${CMAKE_BINARY_DIR}/src/MPC_Common/include
            ${CMAKE_SOURCE_DIR}/src/include
            ${CMAKE_BINARY_DIR}/src/include
            ${CMAKE_SOURCE_DIR}/src/MPC_MPI/include
    )

    target_link_libraries(
        mpc_sets
        PRIVATE
            mpcconfig
            mpclowcomm
            mpcarch
            -Wl,--no-as-needed
            mpcmpi
            mpcomp
            mpcthread
            -Wl,--as-needed
            ${OPA_LIBRARIES}
            ${OFI_LIBRARIES}
            ${CMAKE_RT_LIBS}
            Threads::Threads
    )

    install(TARGETS mpc_sets RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
