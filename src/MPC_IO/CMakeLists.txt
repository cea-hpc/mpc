# MPC_IO/CMakeLists.txt
# Top-level MPC_IO CMake configuration

# Generate mpcio.h from template
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/mpio.h.in" "${CMAKE_CURRENT_BINARY_DIR}/include/mpio.h" @ONLY)

# Install mpcio.h header
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/mpio.h" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mpcframework)

# Build src
add_subdirectory(src)
