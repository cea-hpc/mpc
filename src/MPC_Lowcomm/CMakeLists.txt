# MPC_Lowcomm/CMakeLists.txt
# Builds libmpclowcomm shared library

# Core sources
set(MPC_LOWCOMM_SOURCES
    src/coll.c
    src/comm.c
    src/checksum.c
    src/ft.c
    src/group.c
    src/communicator.c
    src/shm_coll.c
    src/lowcomm_config.c
    src/alloc_mem.c
    src/lowcomm.c
    src/msg_copy.c
    src/lowcomm_thread.c
    src/mqd.c
    src/datatypes_common.c
    src/rail.c
    src/reorder.c
    src/endpoint.c
    src/pset.c
    src/handle_ctx.c
)

# Workshare (optional)
if(MPC_ENABLE_WORKSHARE)
    list(APPEND MPC_LOWCOMM_SOURCES src/workshare.c)
endif()

# Control plane
list(APPEND MPC_LOWCOMM_SOURCES
    src/cplane/auth.c
    src/cplane/peer.c
    src/cplane/proto.c
    src/cplane/port.c
    src/cplane/monitor.c
    src/cplane/set.c
    src/cplane/uid.c
    src/cplane/name.c
)

# LCP core
list(APPEND MPC_LOWCOMM_SOURCES
    src/lcr/rails/tbsm/tbsm.c
    src/lcp/core/lcp_context.c
    src/lcp/core/lcp_ep.c
    src/lcp/core/lcp_progress.c
    src/lcp/core/lcp_datatype.c
    src/lcp/core/lcp_manager.c
    src/lcp/core/lcp_task.c
    src/lcp/core/lcp_request.c
    src/lcp/core/lcp_mem.c
)

# LCP tag
list(APPEND MPC_LOWCOMM_SOURCES
    src/lcp/tag/lcp_tag.c
    src/lcp/tag/lcp_send.c
    src/lcp/tag/lcp_tag_offload.c
    src/lcp/tag/lcp_tag_match.c
    src/lcp/tag/lcp_pending.c
    src/lcp/tag/lcp_recv.c
    src/lcp/tag/lcp_probe.c
)

# LCP rndv and AM
list(APPEND MPC_LOWCOMM_SOURCES
    src/lcp/rndv/lcp_rndv.c
    src/lcp/am/lcp_eager.c
    src/lcp/am/lcp_am.c
)

# LCP RMA
list(APPEND MPC_LOWCOMM_SOURCES
    src/lcp/rma/lcp_rma.c
    src/lcp/rma/lcp_ato.c
    src/lcp/rma/lcp_ato_sw.c
)

# LCR component + SHM rail
list(APPEND MPC_LOWCOMM_SOURCES
    src/lcr/lcr_component.c
    src/lcr/rails/shm/mpc_shm.c
)

# TCP rail
list(APPEND MPC_LOWCOMM_SOURCES
    src/lcr/rails/tcp/tcp.c
    src/lcr/rails/tcp/tcp_toolkit.c
)

# Active messages (optional, requires libffi)
if(MPC_HAVE_AM)
    list(APPEND MPC_LOWCOMM_SOURCES src/am.c)
endif()

# Portals4 rail (optional)
if(PORTALS4_DRIVER_ENABLED)
    list(APPEND MPC_LOWCOMM_SOURCES
        src/lcr/rails/ptl/ptl_ep.c
        src/lcr/rails/ptl/ptl_iface.c
        src/lcr/rails/ptl/ptl_recv.c
        src/lcr/rails/ptl/ptl_rd.c
        src/lcr/rails/ptl/ptl_rma.c
    )
    if(PORTALS4_CONTROL_FLOW_ENABLED)
        list(APPEND MPC_LOWCOMM_SOURCES src/lcr/rails/ptl/ptl_tk.c)
    endif()
endif()

# OFI rail (optional)
if(OFI_DRIVER_ENABLED)
    list(APPEND MPC_LOWCOMM_SOURCES
        src/lcr/rails/ofi/mpc_ofi_request.c
        src/lcr/rails/ofi/mpc_ofi_helpers.c
        src/lcr/rails/ofi/mpc_ofi_dns.c
        src/lcr/rails/ofi/mpc_ofi_domain.c
        src/lcr/rails/ofi/mpc_ofi_context.c
        src/lcr/rails/ofi/mpc_ofi.c
    )
endif()

# Create the shared library
add_library(mpclowcomm SHARED ${MPC_LOWCOMM_SOURCES})


# Include directories
target_include_directories(mpclowcomm
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
        ${CMAKE_SOURCE_DIR}/src/MPC_Launch/include
        ${CMAKE_SOURCE_DIR}/src/MPC_Threads/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/lcr/rails/tcp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/lcr/rails/shm
        ${CMAKE_CURRENT_SOURCE_DIR}/src/cplane
        ${CMAKE_CURRENT_SOURCE_DIR}/src/lcr/rails/tbsm
        ${CMAKE_CURRENT_SOURCE_DIR}/src/lcp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/lcr
        ${CMAKE_BINARY_DIR}
        ${OPA_INCLUDE_DIRS}
)

if(MPC_ENABLE_THREADS)
    target_include_directories(mpclowcomm
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src/MPC_Threads/src
            ${CMAKE_SOURCE_DIR}/src/MPC_Threads/include
            ${CMAKE_SOURCE_DIR}/src/MPC_Topology/include
    )
endif()

if(MPC_ENABLE_MPI)
    target_include_directories(mpclowcomm
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src/MPC_MPI/include
    )
endif()

if(PORTALS4_DRIVER_ENABLED)
    target_include_directories(mpclowcomm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/lcr/rails/ptl)
endif()

if(OFI_DRIVER_ENABLED)
    target_include_directories(mpclowcomm PRIVATE ${OFI_INCLUDE_DIRS})
endif()

if(FFI_FOUND)
    target_include_directories(mpclowcomm PRIVATE ${FFI_INCLUDE_DIRS})
endif()

# Link dependencies
# Link OBJECT targets for include dirs/compile defs, embed OBJECTS for actual code
target_link_libraries(mpclowcomm
    PRIVATE
        mpclaunch
        mpctopology
        mpccommon
        mpcconfig
        m
        ${CMAKE_DL_LIBS}
        ${CMAKE_RT_LIBS}
        ${OPA_LIBRARIES}
        ${HWLOC_LIBS_CMAKE}
        Threads::Threads
)
target_sources(mpclowcomm PRIVATE
    $<TARGET_OBJECTS:mpclaunch>
    $<TARGET_OBJECTS:mpctopology>
    $<TARGET_OBJECTS:mpccommon>
)
target_link_options(mpclowcomm PRIVATE ${OPA_LDFLAGS} ${HWLOC_LDFLAGS})
if(MPCALLOC_FOUND)
    target_link_libraries(mpclowcomm PRIVATE ${ALLOC_LIBS_CMAKE})
endif()

if(MPC_LAUNCHER STREQUAL "PMIX")
    target_link_libraries(mpclowcomm PRIVATE ${PMIX_LIBRARIES})
elseif(MPC_LAUNCHER STREQUAL "PMI1")
    target_link_libraries(mpclowcomm PRIVATE ${PMI_LIBRARIES})
endif()

if(FFI_FOUND AND MPC_HAVE_AM)
    target_link_libraries(mpclowcomm PRIVATE ${FFI_LIBRARIES})
endif()

if(OFI_DRIVER_ENABLED)
    target_link_libraries(mpclowcomm PRIVATE ${OFI_LIBRARIES})
endif()

if(PORTALS4_DRIVER_ENABLED)
    target_link_libraries(mpclowcomm PRIVATE ${PORTALS_LIBRARIES})
endif()

if(PORTALS4_DRIVER_ENABLED)
    target_compile_definitions(mpclowcomm PRIVATE PORTALS4_DRIVER_ENABLED=1)
endif()
if(OFI_DRIVER_ENABLED)
    target_compile_definitions(mpclowcomm PRIVATE OFI_DRIVER_ENABLED=1)
endif()
if(MPC_HAVE_AM)
    target_compile_definitions(mpclowcomm PRIVATE MPC_HAVE_AM=1)
endif()

# Set output name
set_target_properties(mpclowcomm PROPERTIES
    OUTPUT_NAME mpclowcomm
    VERSION ${MPC_VERSION}
    SOVERSION ${MPC_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
)

# Install library
install(TARGETS mpclowcomm
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Install public headers
install(FILES
    include/mpc_lowcomm.h
    include/mpc_lowcomm_rdma.h
    include/mpc_lowcomm_communicator.h
    include/mpc_lowcomm_group.h
    include/mpc_lowcomm_types.h
    include/mpc_lowcomm_def.h
    include/mpc_lowcomm_handle_ctx.h
    include/mpc_lowcomm_mqd.h
    include/mpc_lowcomm_datatypes.h
    include/lcp.h
    include/lcp_def.h
    include/lowcomm_config.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mpcframework
)
if(MPC_HAVE_AM)
    install(FILES
        include/mpc_lowcomm_am.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mpcframework
    )
endif()
