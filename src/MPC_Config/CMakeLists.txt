# MPC_Config/CMakeLists.txt
# Builds libmpcconfig shared library

set(MPC_CONFIG_SOURCES src/loader.c src/conf_types.c src/mpc_conf.c src/utils.c)

# Private headers (not installed, used internally)
set(MPC_CONFIG_PRIVATE_HEADERS src/loader.h src/utils.h)

# Create the shared library
add_library(mpcconfig SHARED ${MPC_CONFIG_SOURCES})

# Include directories
target_include_directories(
    mpcconfig
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/../MPC_Common/include
)

# Link dependencies (dl for dlopen/dlsym used in mpc_conf.c)
target_link_libraries(mpcconfig PRIVATE ${CMAKE_DL_LIBS})

# Set output name
set_target_properties(
    mpcconfig
    PROPERTIES OUTPUT_NAME mpcconfig VERSION ${MPC_VERSION} SOVERSION ${MPC_VERSION_MAJOR} POSITION_INDEPENDENT_CODE ON
)

# Install library
install(TARGETS mpcconfig LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Install public headers
install(FILES include/mpc_conf.h include/mpc_conf_types.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mpcframework)

# Install default config file
install(FILES conf/default.conf DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/mpcframework)
