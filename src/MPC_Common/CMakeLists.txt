# MPC_Common/CMakeLists.txt
# Builds libmpccommon (internal, not installed separately)

# Base sources
set(MPC_COMMON_SOURCES
    src/aio.c
    src/bit_array.c
    src/fifo.c
    src/hashtable.c
    src/io_helper.c
    src/rank.c
    src/debug.c
    src/sctk_no_alloc.c
    src/sctk_pm_json.c
    src/flags.c
    src/debugger.c
    src/elf_32.c
    src/spinlock.c
    src/base64.c
    src/recycler.c
    src/mpc_mempool.c
    src/progress.c
)

# Profiler sources (optional)
if(MPC_ENABLE_PROFILER)
    list(APPEND MPC_COMMON_SOURCES
        src/profiler/sctk_internal_profiler.c
        src/profiler/sctk_performance_tree.c
        src/profiler/sctk_profile_meta.c
        src/profiler/sctk_profiler_array.c
        src/profiler/sctk_profile_render.c
        src/profiler/sctk_profile_render_html.c
        src/profiler/sctk_profile_render_tex.c
        src/profiler/sctk_profile_render_text.c
        src/profiler/sctk_profile_render_xml.c
    )
endif()

# Generate headers from templates
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/mpc_common_types.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/mpc_common_types.h"
    @ONLY
)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/mpc_common_spinlock.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/mpc_common_spinlock.h"
    @ONLY
)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/mpc_keywords.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/mpc_keywords.h"
    @ONLY
)

# Generate elf_64.c from elf_32.c using gen_elf64 script
# The script reads elf_32.c from its own directory and outputs elf_64.c in CWD
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/src/elf_64.c"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/src"
    COMMAND ${CMAKE_COMMAND} -E chdir "${CMAKE_CURRENT_BINARY_DIR}/src" ${CMAKE_CURRENT_SOURCE_DIR}/src/gen_elf64
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/elf_32.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/gen_elf64"
    COMMENT "Generating elf_64.c from elf_32.c"
)
list(APPEND MPC_COMMON_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/src/elf_64.c")

# Create the library (OBJECT — matches autotools noinst_LTLIBRARIES)
add_library(mpccommon OBJECT ${MPC_COMMON_SOURCES})
set_target_properties(mpccommon PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Include directories (PUBLIC for propagation to consumers)
target_include_directories(mpccommon
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${OPA_INCLUDE_DIRS}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/MPC_Arch/include
        ${CMAKE_BINARY_DIR}/src/MPC_Arch/include
        ${CMAKE_SOURCE_DIR}/src/MPC_Config/include
        ${CMAKE_SOURCE_DIR}/src/include
        ${CMAKE_BINARY_DIR}/src/include
        ${CMAKE_BINARY_DIR}
)
if(MPCALLOC_FOUND)
    target_include_directories(mpccommon PUBLIC ${MPCALLOC_INCLUDE_DIRS})
endif()

if(MPC_ENABLE_PROFILER)
    target_include_directories(mpccommon
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src/MPC_MPI/src/profiler
    )
endif()

if(MPC_ENABLE_THREADS)
    target_include_directories(mpccommon
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src/MPC_Threads/src
            ${CMAKE_SOURCE_DIR}/src/MPC_Threads/include
            ${CMAKE_SOURCE_DIR}/src/MPC_Topology/include
    )
endif()

# Link dependencies (PUBLIC for include propagation, PRIVATE for compile-time only)
target_link_libraries(mpccommon
    PUBLIC
        mpcarch
    PRIVATE
        ${OPA_LIBRARIES}
)

if(UNW_FOUND)
    target_include_directories(mpccommon PRIVATE ${UNW_INCLUDE_DIRS})
    target_link_libraries(mpccommon PRIVATE ${UNW_LIBRARIES})
endif()

# Compile definitions
target_compile_definitions(mpccommon
    PRIVATE
        ${MPC_COMPILE_DEFINITIONS}
)



# Install public headers
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/include/mpc_common_types.h"
    include/mpc_common_debugger.h
    include/mpc_common_aio.h
    include/mpc_common_recycler.h
    include/mpc_common_bit.h
    include/uthash.h
    include/mpc_common_progress.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mpcframework
)
