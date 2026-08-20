# MPC_OpenSHMEM/CMakeLists.txt
# Builds libmpcopenshmem shared library

# OpenSHMEM sources
set(MPC_OPENSHMEM_SOURCES src/mpc_openshmem.c src/mpc_openshmem_elf.c src/mpc_openshmem_memory.c)

# Create the shared library
add_library(mpcopenshmem SHARED ${MPC_OPENSHMEM_SOURCES})

# Include directories
target_include_directories(
    mpcopenshmem
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/include
        ${CMAKE_BINARY_DIR}/src/include
        ${CMAKE_SOURCE_DIR}/src/MPC_Common/include
        ${CMAKE_BINARY_DIR}/src/MPC_Common/include
        ${CMAKE_SOURCE_DIR}/src/MPC_Arch/include
        ${CMAKE_BINARY_DIR}/src/MPC_Arch/include
        ${CMAKE_SOURCE_DIR}/src/MPC_Config/include
        ${CMAKE_SOURCE_DIR}/src/MPC_Threads/include
        ${CMAKE_SOURCE_DIR}/src/MPC_Topology/include
        ${CMAKE_SOURCE_DIR}/src/MPC_Lowcomm/include
        ${CMAKE_BINARY_DIR}
)

# Link dependencies
target_link_libraries(mpcopenshmem PRIVATE mpclowcomm ${CMAKE_RT_LIBS})

# Set output name
set_target_properties(
    mpcopenshmem
    PROPERTIES
        OUTPUT_NAME mpcopenshmem
        VERSION ${MPC_VERSION}
        SOVERSION ${MPC_VERSION_MAJOR}
        POSITION_INDEPENDENT_CODE ON
)

# Install library
install(TARGETS mpcopenshmem LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Install public headers
install(FILES include/shmem.h include/pshmem.h include/shmemx.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mpcframework)
