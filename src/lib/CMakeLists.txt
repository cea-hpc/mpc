# lib/CMakeLists.txt
# Builds libmpcframework - umbrella library that links all MPC libraries

# Collect all MPC libraries
set(ALL_MPC_LIBS mpcconfig)

if(MPC_ENABLE_THREADS)
    list(APPEND ALL_MPC_LIBS mpcthread)
endif()

if(MPC_ENABLE_OPENMP)
    list(APPEND ALL_MPC_LIBS mpcomp)
endif()

if(MPC_ENABLE_LOWCOMM)
    list(APPEND ALL_MPC_LIBS mpclowcomm)
endif()

if(MPC_ENABLE_MPI)
    list(APPEND ALL_MPC_LIBS mpcmpi)
endif()

# Create the umbrella shared library
add_library(mpcframework SHARED IMPORTED)

# Create an interface library that aggregates all dependencies
add_library(mpcframework_interface INTERFACE)
target_link_libraries(mpcframework_interface INTERFACE ${ALL_MPC_LIBS})

# Create an actual shared library as a wrapper
# We use a small source file to create the shared library
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/mpcframework.c" "/* MPC Framework umbrella library */\n")
add_library(mpcframework_lib SHARED "${CMAKE_CURRENT_BINARY_DIR}/mpcframework.c")
target_link_libraries(mpcframework_lib PUBLIC ${ALL_MPC_LIBS})
set_target_properties(mpcframework_lib PROPERTIES
    OUTPUT_NAME mpcframework
    VERSION ${MPC_VERSION}
    SOVERSION ${MPC_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
)

# Generate and install mpc.h
configure_file(
    "${CMAKE_SOURCE_DIR}/src/include/mpc.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/mpc.h"
    @ONLY
)

# Install library
install(TARGETS mpcframework_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Install headers
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/mpc.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mpcframework
)

if(MPC_ENABLE_THREADS)
    # Generate mpc_threads_config.h
    configure_file(
        "${CMAKE_SOURCE_DIR}/src/include/mpc_threads_config.h.in"
        "${CMAKE_CURRENT_BINARY_DIR}/mpc_threads_config.h"
        @ONLY
    )
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/mpc_threads_config.h"
        "${CMAKE_SOURCE_DIR}/src/include/mpc_main.h"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mpcframework
    )
endif()

if(MPC_ENABLE_MPI)
    install(FILES
        "${CMAKE_SOURCE_DIR}/src/include/mpi.h"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mpcframework
    )
endif()
