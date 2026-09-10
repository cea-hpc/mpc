# MPC_MPI/CMakeLists.txt
# Builds libmpcmpi shared library

# Core MPI sources
set(MPC_MPI_SOURCES
    src/osc/osc_mpi.c
    src/osc/osc_win_attr.c
    src/comm_lib.c
    src/egreq_nbc.c
    src/mpc_extern32.c
    src/mpc_mpi_halo.c
    src/mpc_reduction.c
    src/mpit.c
    src/datatype.c
    src/egreq_progress.c
    src/mpc_info.c
    src/mpc_nbc.c
    src/mpi_conf.c
    src/mpc_coll.c
    src/session.c
    src/errh.c
    src/mgroup.c
    src/egreq_classes.c
    src/mpc_cs.c
    src/mpc_mpi.c
    src/mpc_nbc_progress_thread_binding.c
)

# Fortran binding (optional)
if(MPC_ENABLE_FORTRAN)
    list(APPEND MPC_MPI_SOURCES src/fortran.c)
endif()

# Dataloop sources
list(APPEND MPC_MPI_SOURCES
    src/dataloop/contents_support.c
    src/dataloop/darray_support.c
    src/dataloop/dataloop.c
    src/dataloop/dataloop_create.c
    src/dataloop/dataloop_create_blockindexed.c
    src/dataloop/dataloop_create_contig.c
    src/dataloop/dataloop_create_indexed.c
    src/dataloop/dataloop_create_pairtype.c
    src/dataloop/dataloop_create_struct.c
    src/dataloop/dataloop_create_vector.c
    src/dataloop/segment.c
    src/dataloop/segment_count.c
    src/dataloop/segment_flatten.c
    src/dataloop/segment_packunpack.c
    src/dataloop/subarray_support.c
    src/dataloop/typesize_support.c
)

# Mpitypes sources
list(APPEND MPC_MPI_SOURCES
    src/mpitypes/mpitypes.c
    src/mpitypes/mpitypes_dataloop.c
    src/mpitypes/mpitypes_transpack_flatten.c
)

# Create the shared library
add_library(mpcmpi SHARED ${MPC_MPI_SOURCES})

# Include directories
target_include_directories(mpcmpi
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
        ${CMAKE_SOURCE_DIR}/src/MPC_Threads/include
        ${CMAKE_SOURCE_DIR}/src/MPC_Threads/src
        ${CMAKE_SOURCE_DIR}/src/MPC_Config/include
        ${CMAKE_SOURCE_DIR}/src/MPC_Config/src/generated
        ${CMAKE_SOURCE_DIR}/src/MPC_Topology/include
        ${CMAKE_SOURCE_DIR}/src/MPC_Lowcomm/include
        ${CMAKE_SOURCE_DIR}/src/MPC_Lowcomm/src
        ${CMAKE_SOURCE_DIR}/src/MPC_Launch/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/osc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/dataloop
        ${CMAKE_CURRENT_SOURCE_DIR}/src/mpitypes
        ${CMAKE_SOURCE_DIR}/src/MPC_Lowcomm/src/lcr/rails/tcp
        ${CMAKE_SOURCE_DIR}/src/MPC_Lowcomm/src/lcr/rails/shm
        ${CMAKE_SOURCE_DIR}/src/MPC_Lowcomm/src/lcr
        ${CMAKE_SOURCE_DIR}/src/MPC_Lowcomm/src/lcp
        ${CMAKE_BINARY_DIR}
)

if(MPC_ENABLE_THREADS)
    target_include_directories(mpcmpi
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src/MPC_Threads/src
            ${CMAKE_SOURCE_DIR}/src/MPC_Threads/include
            ${CMAKE_SOURCE_DIR}/src/MPC_Topology/include
    )
endif()

if(OFI_DRIVER_ENABLED)
    target_include_directories(mpcmpi
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src/MPC_Lowcomm/src/lcr/rails/ofi
    )
endif()

if(MPC_ENABLE_OPENMP)
    target_include_directories(mpcmpi
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src/MPC_OpenMP/include
            ${CMAKE_SOURCE_DIR}/src/MPC_OpenMP/src
    )
    target_link_libraries(mpcmpi PRIVATE mpcomp)
endif()

# Link dependencies
target_link_libraries(mpcmpi
    PUBLIC
        mpclowcomm
    PRIVATE
        ${CMAKE_RT_LIBS}
        ${CMAKE_DL_LIBS}
        m
        Threads::Threads
)

if(MPCALLOC_FOUND)
    target_link_directories(mpcmpi PRIVATE ${MPCALLOC_LIBRARY_DIRS})
    target_link_libraries(mpcmpi PRIVATE ${ALLOC_LIBS_CMAKE})
endif()

if(MPC_ENABLE_THREADS)
    target_link_libraries(mpcmpi PRIVATE mpcthread)
endif()

# Set output name
set_target_properties(mpcmpi PROPERTIES
    OUTPUT_NAME mpcmpi
    VERSION ${MPC_VERSION}
    SOVERSION ${MPC_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
)

# Install library
install(TARGETS mpcmpi
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Install public headers
install(FILES
    include/mpc_mpi_comm_lib.h
    include/mpc_mpi.h
    include/mpc_mpi_fortran.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mpcframework
)
