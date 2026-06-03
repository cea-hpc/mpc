# MPC_OpenMP/CMakeLists.txt
# Builds libmpcomp shared library + compat libraries (libiomp5, libgomp, libomp)

set(MPC_OPENMP_SOURCES
    src/mpcomp_api.c
    src/mpcomp_core.c
    src/mpcomp_fortran.c
    src/mpcomp_loop.c
    src/mpcomp_parallel_region.c
    src/mpcomp_places_env.c
    src/mpcomp_spinning_core.c
    src/mpcomp_sync.c
    src/mpcomp_task.c
    src/mpcomp_tree.c
    src/mpcomp_alloc.c
    src/omp_gomp.c
    src/omp_intel_atomics.c
    src/omp_intel.c
    src/mpcomp_task_trace.c
    src/mpcomp_callback.c
    # OMPT tooling
    src/tools/src/mpcompt_entryPoints.c
    src/tools/src/mpcompt_instance.c
    src/tools/src/mpcompt_init.c
    src/tools/callback/mpcompt_task.c
    src/tools/callback/mpcompt_parallel.c
    src/tools/callback/mpcompt_dispatch.c
    src/tools/callback/mpcompt_target.c
    src/tools/callback/mpcompt_device.c
    src/tools/callback/mpcompt_mutex.c
    src/tools/callback/mpcompt_thread.c
    src/tools/callback/mpcompt_workShare.c
    src/tools/callback/mpcompt_cancel.c
    src/tools/callback/mpcompt_flush.c
    src/tools/callback/mpcompt_control_tool.c
    src/tools/callback/mpcompt_sync.c
)

# Create the shared library
add_library(mpcomp SHARED ${MPC_OPENMP_SOURCES})

# Include directories
target_include_directories(mpcomp
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/MPC_Common/include
        ${CMAKE_BINARY_DIR}/src/MPC_Common/include
        ${CMAKE_SOURCE_DIR}/src/include
        ${CMAKE_BINARY_DIR}/src/include
        ${CMAKE_SOURCE_DIR}/src/MPC_Arch/include
        ${CMAKE_BINARY_DIR}/src/MPC_Arch/include
        ${CMAKE_SOURCE_DIR}/src/MPC_Config/include
        ${CMAKE_SOURCE_DIR}/src/MPC_Topology/include
        ${CMAKE_SOURCE_DIR}/src/MPC_Launch/include
        ${CMAKE_SOURCE_DIR}/src/MPC_Threads/include
        ${CMAKE_SOURCE_DIR}/src/MPC_Threads/src
        ${CMAKE_SOURCE_DIR}/src/MPC_Lowcomm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/callback
        ${CMAKE_BINARY_DIR}
        ${OPA_INCLUDE_DIRS}
)

if(VMEM_FOUND)
    target_include_directories(mpcomp PRIVATE ${VMEM_INCLUDE_DIRS})
endif()

# Link dependencies
# Link OBJECT targets for include dirs/compile defs, embed OBJECTS for actual code
target_link_libraries(mpcomp
    PRIVATE
        mpclaunch
        mpctopology
        mpccommon
        mpcconfig
        mpcarch
        ${CMAKE_DL_LIBS}
        ${CMAKE_RT_LIBS}
        ${OPA_LIBRARIES}
        ${HWLOC_LIBS_CMAKE}
        Threads::Threads
)
target_sources(mpcomp PRIVATE
    $<TARGET_OBJECTS:mpclaunch>
    $<TARGET_OBJECTS:mpctopology>
    $<TARGET_OBJECTS:mpccommon>
)
target_link_options(mpcomp PRIVATE ${OPA_LDFLAGS} ${HWLOC_LDFLAGS})
if(MPCALLOC_FOUND)
    target_link_libraries(mpcomp PRIVATE ${ALLOC_LIBS_CMAKE})
endif()

if(MPC_LAUNCHER STREQUAL "PMIX")
    target_link_libraries(mpcomp PRIVATE ${PMIX_LINK_LIBRARIES})
elseif(MPC_LAUNCHER STREQUAL "PMI1")
    target_link_libraries(mpcomp PRIVATE ${PMI_LINK_LIBRARIES})
endif()

if(MPC_ENABLE_THREADS)
    target_link_libraries(mpcomp PRIVATE mpcthread)
endif()

if(VMEM_FOUND)
    target_link_libraries(mpcomp PRIVATE ${VMEM_LIBRARIES})
endif()

# Set output name & link flags
set_target_properties(mpcomp PROPERTIES
    OUTPUT_NAME mpcomp
    VERSION ${MPC_VERSION}
    SOVERSION ${MPC_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
    LINK_FLAGS "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/version_gomp.txt"
)

# Install library
install(TARGETS mpcomp
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Install public headers
install(FILES
    include/omp_abi.h
    include/mpc_omp_abi.h
    include/mpc_omp_task_label.h
    include/mpc_omp_interop_mpi.h
    include/mpc_omp_interop_cuda.h
    include/mpc_omp.h
    include/mpc_omp_task_property.h
    include/mpc_omp_task_trace.h
    include/omp.h
    include/ompt.h
    include/omp-tools.h
    include/omp_gomp_constants.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mpcframework
)

# Compatibility libraries (empty stubs that link to libmpcomp)
# These allow linking with -liomp5, -lgomp, -lomp
foreach(_COMPAT_LIB iomp5 gomp omp)
    add_library(${_COMPAT_LIB} SHARED src/fakelib/fakelib.c)
    set_target_properties(${_COMPAT_LIB} PROPERTIES
        VERSION ${MPC_VERSION}
        SOVERSION ${MPC_VERSION_MAJOR}
        POSITION_INDEPENDENT_CODE ON
    )
    install(TARGETS ${_COMPAT_LIB}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endforeach()
