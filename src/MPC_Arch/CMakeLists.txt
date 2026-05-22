# MPC_Arch/CMakeLists.txt
# Builds libmpcarch shared library

# Base sources
set(MPC_ARCH_SOURCES
    src/pause.c
    src/timer.c
)

# Context switch sources (architecture-specific)
if(MPC_ARCH_CONTEXT_SWITCH STREQUAL "x86_64")
    list(APPEND MPC_ARCH_SOURCES
        src/context/x86_64/getcontext.S
        src/context/x86_64/makecontext.c
        src/context/x86_64/setcontext.S
        src/context/x86_64/startcontext.S
        src/context/x86_64/swapcontext.S
    )
endif()

# Microtask sources (architecture-specific)
if(MPC_ARCH_MICROTASK_ARCH STREQUAL "x86_64")
    list(APPEND MPC_ARCH_SOURCES
        src/microtask/x86_64/z_Linux_asm.S
    )
elseif(MPC_ARCH_MICROTASK_ARCH STREQUAL "generic")
    list(APPEND MPC_ARCH_SOURCES
        src/microtask/generic/microtask.c
    )
endif()

# Generate mpc_arch.h from template
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/mpc_arch.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/mpc_arch.h"
    @ONLY
)

# Create the shared library
add_library(mpcarch SHARED ${MPC_ARCH_SOURCES})

# Include directories
target_include_directories(mpcarch
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/include
    PRIVATE
        ${OPA_INCLUDE_DIRS}
)

# Link dependencies
target_link_libraries(mpcarch
    PUBLIC
        ${OPA_LDFLAGS}
)

# Compile definitions
target_compile_definitions(mpcarch
    PRIVATE
        ${MPC_COMPILE_DEFINITIONS}
)

# Set output name to match autotools
set_target_properties(mpcarch PROPERTIES
    OUTPUT_NAME mpcarch
    VERSION ${MPC_VERSION}
    SOVERSION ${MPC_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
)

# Install library
install(TARGETS mpcarch
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Install headers
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/include/mpc_arch.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/mpc_arch_context.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mpcframework
)
