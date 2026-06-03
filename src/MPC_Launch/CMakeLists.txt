# MPC_Launch/CMakeLists.txt
# Builds libmpclaunch (internal, not installed separately)

set(MPC_LAUNCH_SOURCES
    src/main.c
    src/libc_main.c
    src/mpc_launch.c
    src/pmi.c
    src/shm.c
)

# Hydra launcher sources (conditional)
if(MPC_LAUNCHER STREQUAL "HYDRA")
    list(APPEND MPC_LAUNCH_SOURCES
        src/hydra_client/safestr2.c
        src/hydra_client/simple_pmi.c
        src/hydra_client/simple_pmiutil.c
    )
    # Hydra client requires standard library defines
    set_source_files_properties(
        src/hydra_client/safestr2.c
        src/hydra_client/simple_pmi.c
        src/hydra_client/simple_pmiutil.c
        PROPERTIES
        COMPILE_DEFINITIONS "HAVE_STDLIB_H=1;HAVE_UNISTD_H=1;HAVE_SYS_SOCKET_H=1"
    )
endif()

# Create the library (OBJECT — matches autotools noinst_LTLIBRARIES)
add_library(mpclaunch OBJECT ${MPC_LAUNCH_SOURCES})
set_target_properties(mpclaunch PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Include directories (PUBLIC for propagation to consumers)
target_include_directories(mpclaunch
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/MPC_Common/include
        ${CMAKE_BINARY_DIR}/src/MPC_Common/include
        ${CMAKE_SOURCE_DIR}/src/MPC_Topology/include
        ${CMAKE_SOURCE_DIR}/src/MPC_Arch/include
        ${CMAKE_BINARY_DIR}/src/MPC_Arch/include
        ${CMAKE_SOURCE_DIR}/src/include
        ${CMAKE_BINARY_DIR}/src/include
        ${CMAKE_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/src/MPC_Config/include
        ${CMAKE_SOURCE_DIR}/src/MPC_Config/src/generated
        ${OPA_INCLUDE_DIRS}
)

if(MPC_LAUNCHER STREQUAL "HYDRA")
    target_include_directories(mpclaunch
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/hydra_client
    )
endif()

if(MPC_ENABLE_THREADS)
    target_include_directories(mpclaunch
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src/MPC_Threads/src
            ${CMAKE_SOURCE_DIR}/src/MPC_Threads/include
            ${CMAKE_SOURCE_DIR}/src/MPC_Topology/include
    )
endif()

# Link dependencies (PUBLIC for include propagation)
target_link_libraries(mpclaunch
    PUBLIC
        mpctopology
    PRIVATE
        ${CMAKE_DL_LIBS}
        ${CMAKE_RT_LIBS}
        ${OPA_LIBRARIES}
        Threads::Threads
)

if(MPC_LAUNCHER STREQUAL "PMIX")
    target_include_directories(mpclaunch PRIVATE ${PMIX_INCLUDE_DIRS})
    target_link_libraries(mpclaunch PRIVATE ${PMIX_LINK_LIBRARIES})
elseif(MPC_LAUNCHER STREQUAL "PMI1")
    target_include_directories(mpclaunch PRIVATE ${PMI_INCLUDE_DIRS})
    target_link_libraries(mpclaunch PRIVATE ${PMI_LINK_LIBRARIES})
endif()

if(MPC_LAUNCHER STREQUAL "HYDRA")
    target_compile_definitions(mpclaunch PRIVATE HYDRA_LAUNCHER_ENABLED=1)
endif()

# Install public headers
install(FILES
    include/mpc_launch_pmi.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mpcframework
)
