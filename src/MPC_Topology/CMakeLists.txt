# MPC_Topology/CMakeLists.txt
# Builds libmpctopology (internal, not installed separately)

set(MPC_TOPOLOGY_SOURCES
    src/topology.c
    src/topology_device.c
    src/topology_render.c
)

# Create the library (OBJECT — matches autotools noinst_LTLIBRARIES)
add_library(mpctopology OBJECT ${MPC_TOPOLOGY_SOURCES})
set_target_properties(mpctopology PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Include directories (PUBLIC for propagation to consumers)
target_include_directories(mpctopology
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${HWLOC_INCLUDE_DIRS}
        ${OPA_INCLUDE_DIRS}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/MPC_Common/include
        ${CMAKE_BINARY_DIR}/src/MPC_Common/include
        ${CMAKE_SOURCE_DIR}/src/MPC_Arch/include
        ${CMAKE_BINARY_DIR}/src/MPC_Arch/include
        ${CMAKE_SOURCE_DIR}/src/MPC_Config/include
        ${CMAKE_SOURCE_DIR}/src/include
        ${CMAKE_BINARY_DIR}/src/include
        ${CMAKE_BINARY_DIR}
)

if(MPC_ENABLE_THREADS)
    target_include_directories(mpctopology
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src/MPC_Threads/src
            ${CMAKE_SOURCE_DIR}/src/MPC_Threads/include
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
endif()

# Link dependencies (PUBLIC for include propagation)
target_link_libraries(mpctopology
    PUBLIC
        mpccommon
        mpcconfig
    PRIVATE
        ${HWLOC_LDFLAGS}
        ${OPA_LDFLAGS}
)

# Headers are noinst (internal only, not installed)
