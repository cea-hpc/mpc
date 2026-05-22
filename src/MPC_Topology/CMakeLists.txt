# MPC_Topology/CMakeLists.txt
# Builds libmpctopology (internal, not installed separately)

set(MPC_TOPOLOGY_SOURCES
    src/topology.c
    src/topology_device.c
    src/topology_render.c
)

# Create the library
add_library(mpctopology SHARED ${MPC_TOPOLOGY_SOURCES})

# Include directories
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

# Link dependencies
target_link_libraries(mpctopology
    PUBLIC
        mpccommon
        mpcconfig
        ${HWLOC_LDFLAGS}
        ${OPA_LDFLAGS}
)

# Compile definitions
target_compile_definitions(mpctopology
    PRIVATE
        ${MPC_COMPILE_DEFINITIONS}
)

# Set output name
set_target_properties(mpctopology PROPERTIES
    OUTPUT_NAME mpctopology
    POSITION_INDEPENDENT_CODE ON
)

# Headers are noinst (internal only, not installed)
