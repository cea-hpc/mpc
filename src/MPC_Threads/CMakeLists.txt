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
    target_link_libraries(mpcthread PRIVATE mpclowcomm)
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
target_link_libraries(mpcthread
    PUBLIC
        mpclaunch
    PRIVATE
        Threads::Threads
        ${OPA_LIBRARIES}
)

if(MPC_CUDA_ENABLED)
    target_link_libraries(mpcthread PRIVATE CUDA::cuda_driver)
endif()

# Compile definitions
target_compile_definitions(mpcthread
    PRIVATE
        ${MPC_COMPILE_DEFINITIONS}
)

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
