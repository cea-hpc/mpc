# MPC_Threads/CMakeLists.txt
# Builds libmpcthread shared library

set(MPC_THREADS_SOURCES
    src/sctk_context.c
    src/futex.c
    src/thread_ptr.c
    src/thread.c
    src/tls.c
    src/kthread.c
    src/microthread.c
    src/romio_ctx.c
    src/pthread_engine.c
    src/ethread_engine.c
    src/ethread_mxn_engine.c
    src/ethread_posix.c
    src/accelerator.c
)

# CUDA support (optional)
if(MPC_CUDA_ENABLED)
    list(APPEND MPC_THREADS_SOURCES src/accelerator/cuda.c)
endif()

# ROCM support (optional)
if(MPC_ROCM_ENABLED)
    list(APPEND MPC_THREADS_SOURCES src/accelerator/rocm.c)
endif()

# Create the shared library
add_library(mpcthread SHARED ${MPC_THREADS_SOURCES})

# Include directories
target_include_directories(mpcthread
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
        ${CMAKE_SOURCE_DIR}/src/MPC_Config/src/generated
        ${CMAKE_SOURCE_DIR}/src/MPC_Topology/include
        ${CMAKE_BINARY_DIR}
        ${OPA_INCLUDE_DIRS}
)

if(MPC_ENABLE_LOWCOMM)
    target_include_directories(mpcthread
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src/MPC_Lowcomm/src
            ${CMAKE_SOURCE_DIR}/src/MPC_Lowcomm/include
    )
    # Include dirs only; do NOT link mpclowcomm (circular dep resolved at runtime)
endif()

if(MPC_ENABLE_MPI)
    target_include_directories(mpcthread
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src/MPC_MPI/include
            ${CMAKE_SOURCE_DIR}/src/MPC_MPI/src
            ${CMAKE_SOURCE_DIR}/src/MPC_Lowcomm/include
    )
endif()

if(MPC_ENABLE_OPENMP)
    target_include_directories(mpcthread
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src/MPC_OpenMP/include
            ${CMAKE_SOURCE_DIR}/src/MPC_OpenMP/src
            ${CMAKE_SOURCE_DIR}/src/MPC_OpenMP/src/tools/src
            ${CMAKE_SOURCE_DIR}/src/MPC_OpenMP/src/tools/callback
            ${CMAKE_SOURCE_DIR}/src/MPC_OpenMP/src/tools/trace
    )
endif()

# Link dependencies
# Link OBJECT targets for include dirs/compile defs, embed OBJECTS for actual code
target_link_libraries(mpcthread
    PRIVATE
        mpclaunch
        mpctopology
        mpccommon
        mpcconfig
        ${CMAKE_DL_LIBS}
        ${CMAKE_RT_LIBS}
        ${OPA_LIBRARIES}
        ${HWLOC_LIBS_CMAKE}
        Threads::Threads
)
target_sources(mpcthread PRIVATE
    $<TARGET_OBJECTS:mpclaunch>
    $<TARGET_OBJECTS:mpctopology>
    $<TARGET_OBJECTS:mpccommon>
)
target_link_options(mpcthread PRIVATE ${OPA_LDFLAGS} ${HWLOC_LDFLAGS})
if(MPCALLOC_FOUND)
    target_link_libraries(mpcthread PRIVATE ${ALLOC_LIBS_CMAKE})
endif()

if(MPC_LAUNCHER STREQUAL "PMIX")
    target_link_libraries(mpcthread PRIVATE ${PMIX_LINK_LIBRARIES})
elseif(MPC_LAUNCHER STREQUAL "PMI1")
    target_link_libraries(mpcthread PRIVATE ${PMI_LINK_LIBRARIES})
endif()

if(MPC_CUDA_ENABLED)
    target_link_libraries(mpcthread PRIVATE CUDA::cuda_driver)
endif()

# Set output name
set_target_properties(mpcthread PROPERTIES
    OUTPUT_NAME mpcthread
    VERSION ${MPC_VERSION}
    SOVERSION ${MPC_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
)

# Install library
install(TARGETS mpcthread
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Install public headers
install(FILES
    include/mpc_thread.h
    include/mpc_thread_engines.h
    include/mpcposix/pthread.h
    include/mpcposix/semaphore.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mpcframework
)
