# MPC_IO/src/CMakeLists.txt
# Builds libmpcio.so

add_subdirectory(romio)

# libmpcio shared library
add_library(mpcio SHARED mpl.c mpir_ext.c)

target_include_directories(mpcio PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src/MPC_Common/include
    ${CMAKE_SOURCE_DIR}/src/include
    ${CMAKE_BINARY_DIR}/src/MPC_Arch/include
    ${CMAKE_SOURCE_DIR}/src/MPC_Arch/include
    ${CMAKE_SOURCE_DIR}/src/MPC_IO/include
    ${CMAKE_SOURCE_DIR}/src/MPC_MPI/include
    ${CMAKE_SOURCE_DIR}/src/MPC_Lowcomm/include
    ${CMAKE_SOURCE_DIR}/src/MPC_Threads/include
    ${CMAKE_BINARY_DIR}/src/include
    ${CMAKE_SOURCE_DIR}/src/MPC_Launch/include
    ${CMAKE_SOURCE_DIR}/src/MPC_Topology/include
    ${CMAKE_BINARY_DIR}/src/MPC_Common/include
    ${CMAKE_SOURCE_DIR}/src/MPC_Config/include
    ${CMAKE_BINARY_DIR}/src/MPC_IO/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/romio/mpi-io
    ${CMAKE_CURRENT_SOURCE_DIR}/romio/include
    ${CMAKE_CURRENT_BINARY_DIR}/romio/include
)

target_link_libraries(mpcio PRIVATE mpcmpi ${OPA_LIBRARIES} Threads::Threads)
target_link_options(mpcio PRIVATE ${OPA_LDFLAGS})
# Embed ALL romio/promio objects (matches autotools noinst_LTLIBRARIES behavior)
target_sources(mpcio PRIVATE
    $<TARGET_OBJECTS:romio>
    $<TARGET_OBJECTS:promio>
)

set_target_properties(mpcio PROPERTIES
    VERSION ${MPC_VERSION}
    SOVERSION ${MPC_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
)

# Install libmpcio.so
install(TARGETS mpcio
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
