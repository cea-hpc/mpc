# MPC_Fortran/src/CMakeLists.txt
# Code generation pipeline and libmpcfortran library

# Require Python for code generation (matches autotools requirement)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Set Fortran module output directory so .mod files go to build dir
set(CMAKE_Fortran_MODULE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

# ==================================================================
# Step 1: Build gen_constants executable and generate constants.json
# ==================================================================
add_executable(gen_constants gen_constants.c)
target_compile_definitions(gen_constants PRIVATE ${MPC_COMPILE_DEFINITIONS})
target_include_directories(gen_constants PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src/MPC_Lowcomm/include
    ${CMAKE_SOURCE_DIR}/src/MPC_MPI/include
    ${CMAKE_SOURCE_DIR}/src/MPC_Common/include
    ${CMAKE_BINARY_DIR}/src/MPC_Common/include
    ${CMAKE_SOURCE_DIR}/src/include
    ${CMAKE_BINARY_DIR}/src/include
    ${CMAKE_SOURCE_DIR}/src/MPC_Arch/include
    ${CMAKE_BINARY_DIR}/src/MPC_Arch/include
    ${CMAKE_SOURCE_DIR}/src/MPC_Config/include
    ${CMAKE_SOURCE_DIR}/src/MPC_Topology/include
    ${CMAKE_SOURCE_DIR}/src/MPC_Launch/include
)
# TODO: should fix gen_constants.c so MPC_Fortran can build without MPC_IO headers
#if(MPC_ENABLE_MPIIO)
    target_include_directories(gen_constants PRIVATE
        ${CMAKE_BINARY_DIR}/src/MPC_IO/include
        ${CMAKE_SOURCE_DIR}/src/MPC_IO/include
    )
#endif()
target_link_libraries(gen_constants PRIVATE mpcconfig mpclowcomm mpcmpi)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/constants.json"
    COMMAND gen_constants > constants.json
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS gen_constants
    COMMENT "Generating constants.json"
)

# ==================================================================
# Step 2: Preprocess preparsed_mpih.c -> preparsed_mpih.dat
# ==================================================================
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/preparsed_mpih.dat"
    COMMAND ${CMAKE_C_COMPILER} -E ${CMAKE_C_FLAGS}
        -I${CMAKE_SOURCE_DIR}/src/MPC_MPI/include
        -I${CMAKE_SOURCE_DIR}/src/include
        -I${CMAKE_BINARY_DIR}/src/include
        -I${CMAKE_SOURCE_DIR}/src/MPC_Common/include
        -I${CMAKE_BINARY_DIR}/src/MPC_Common/include
        -I${CMAKE_SOURCE_DIR}/src/MPC_Arch/include
        -I${CMAKE_BINARY_DIR}/src/MPC_Arch/include
        -I${CMAKE_SOURCE_DIR}/src/MPC_Lowcomm/include
        -I${CMAKE_SOURCE_DIR}/src/MPC_Config/include
        -I${CMAKE_SOURCE_DIR}/src/MPC_Topology/include
        -I${CMAKE_SOURCE_DIR}/src/MPC_Launch/include
        -I${CMAKE_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/preparsed_mpih.c -o preparsed_mpih.dat
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS preparsed_mpih.c
    COMMENT "Preprocessing preparsed_mpih.c"
)

# ==================================================================
# Step 3: Run genmod.py to generate Fortran interface files
# ==================================================================
set(MPCFORT_GENERATED_FILES
    "${CMAKE_CURRENT_BINARY_DIR}/mpi_constants.f90"
    "${CMAKE_CURRENT_BINARY_DIR}/mpi_f08_c.f90"
    "${CMAKE_CURRENT_BINARY_DIR}/mpi_f08_constants.f90"
    "${CMAKE_CURRENT_BINARY_DIR}/mpi_f08_ctypes.f90"
    "${CMAKE_CURRENT_BINARY_DIR}/mpi_f08.f90"
    "${CMAKE_CURRENT_BINARY_DIR}/mpi_f08_types.f90"
    "${CMAKE_CURRENT_BINARY_DIR}/mpi.f90"
    "${CMAKE_CURRENT_BINARY_DIR}/mpi_sizeofs.f90"
    "${CMAKE_CURRENT_BINARY_DIR}/mpi_base.f90"
    "${CMAKE_CURRENT_BINARY_DIR}/mpif.h"
)

add_custom_command(
    OUTPUT ${MPCFORT_GENERATED_FILES}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/genmod.py
            -m "${CMAKE_CURRENT_BINARY_DIR}/preparsed_mpih.dat"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/genmod.py"
            "${CMAKE_CURRENT_BINARY_DIR}/preparsed_mpih.dat"
            "${CMAKE_CURRENT_BINARY_DIR}/constants.json"
            "${CMAKE_CURRENT_SOURCE_DIR}/mpi_interface.json"
            "${CMAKE_CURRENT_SOURCE_DIR}/typesc2f.json"
            "${CMAKE_CURRENT_SOURCE_DIR}/sizeofs.json"
            "${CMAKE_CURRENT_SOURCE_DIR}/MPIf08_types.json"
            "${CMAKE_CURRENT_SOURCE_DIR}/MPIf08_ctypes.json"
    COMMENT "Generating Fortran interface files with genmod.py"
)

# ==================================================================
# Step 4: Assemble mpi_module.f90 and mpi08_module.f90
# ==================================================================
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/mpi_module.f90"
    COMMAND ${CMAKE_COMMAND} -E cat
        "${CMAKE_CURRENT_BINARY_DIR}/mpi_sizeofs.f90"
        "${CMAKE_CURRENT_BINARY_DIR}/mpi_constants.f90"
        "${CMAKE_CURRENT_BINARY_DIR}/mpi_base.f90"
        "${CMAKE_CURRENT_BINARY_DIR}/mpi.f90"
        "${CMAKE_CURRENT_SOURCE_DIR}/predef_types.f90"
        > "${CMAKE_CURRENT_BINARY_DIR}/mpi_module.f90"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/mpi_sizeofs.f90"
            "${CMAKE_CURRENT_BINARY_DIR}/mpi_constants.f90"
            "${CMAKE_CURRENT_BINARY_DIR}/mpi_base.f90"
            "${CMAKE_CURRENT_BINARY_DIR}/mpi.f90"
            "${CMAKE_CURRENT_SOURCE_DIR}/predef_types.f90"
    COMMENT "Assembling mpi_module.f90"
)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/mpi08_module.f90"
    COMMAND ${CMAKE_COMMAND} -E cat
        "${CMAKE_CURRENT_BINARY_DIR}/mpi_f08_ctypes.f90"
        "${CMAKE_CURRENT_BINARY_DIR}/mpi_f08_types.f90"
        "${CMAKE_CURRENT_BINARY_DIR}/mpi_f08_constants.f90"
        "${CMAKE_CURRENT_BINARY_DIR}/mpi_f08_c.f90"
        "${CMAKE_CURRENT_BINARY_DIR}/mpi_f08.f90"
        "${CMAKE_CURRENT_SOURCE_DIR}/predef_types_08.f90"
        > "${CMAKE_CURRENT_BINARY_DIR}/mpi08_module.f90"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/mpi_f08_ctypes.f90"
            "${CMAKE_CURRENT_BINARY_DIR}/mpi_f08_types.f90"
            "${CMAKE_CURRENT_BINARY_DIR}/mpi_f08_constants.f90"
            "${CMAKE_CURRENT_BINARY_DIR}/mpi_f08_c.f90"
            "${CMAKE_CURRENT_BINARY_DIR}/mpi_f08.f90"
            "${CMAKE_CURRENT_SOURCE_DIR}/predef_types_08.f90"
    COMMENT "Assembling mpi08_module.f90"
)

# ==================================================================
# Step 5: Build libmpcfortran shared library
# ==================================================================
add_library(mpcfortran SHARED
    "${CMAKE_CURRENT_BINARY_DIR}/mpi08_module.f90"
    "${CMAKE_CURRENT_BINARY_DIR}/mpi_module.f90"
    mpc_fortran.c
    mpc_fortran_helpers.c
    omp.f90
)

target_compile_definitions(mpcfortran PRIVATE ${MPC_COMPILE_DEFINITIONS})

target_include_directories(mpcfortran PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src/MPC_Lowcomm/include
    ${CMAKE_SOURCE_DIR}/src/include
    ${CMAKE_BINARY_DIR}/src/include
    ${CMAKE_SOURCE_DIR}/src/MPC_IO/include
    ${CMAKE_BINARY_DIR}/src/MPC_IO/include
    ${CMAKE_SOURCE_DIR}/src/MPC_MPI/include
    ${CMAKE_SOURCE_DIR}/src/MPC_Common/include
    ${CMAKE_BINARY_DIR}/src/MPC_Common/include
    ${CMAKE_SOURCE_DIR}/src/MPC_Arch/include
    ${CMAKE_BINARY_DIR}/src/MPC_Arch/include
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(mpcfortran PRIVATE mpcmpi)
if(MPC_ENABLE_MPIIO)
    target_link_libraries(mpcfortran PRIVATE mpcio)
endif()

set_target_properties(mpcfortran PROPERTIES
    VERSION ${MPC_VERSION}
    SOVERSION ${MPC_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
)

# ==================================================================
# Step 6: Install library and Fortran module files
# ==================================================================
install(TARGETS mpcfortran
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Install .mod files generated by Fortran compilation + mpif.h
# These are build artifacts so we use install(CODE) to copy at install time
install(CODE "
    file(INSTALL
        DESTINATION \"\${CMAKE_INSTALL_PREFIX}/include/mpcframework/fortran\"
        FILES
        \"${CMAKE_CURRENT_BINARY_DIR}/mpi.mod\"
        \"${CMAKE_CURRENT_BINARY_DIR}/mpi_base.mod\"
        \"${CMAKE_CURRENT_BINARY_DIR}/mpi_constants.mod\"
        \"${CMAKE_CURRENT_BINARY_DIR}/mpi_f08.mod\"
        \"${CMAKE_CURRENT_BINARY_DIR}/mpi_f08_c.mod\"
        \"${CMAKE_CURRENT_BINARY_DIR}/mpi_f08_constants.mod\"
        \"${CMAKE_CURRENT_BINARY_DIR}/mpi_f08_ctypes.mod\"
        \"${CMAKE_CURRENT_BINARY_DIR}/mpi_f08_types.mod\"
        \"${CMAKE_CURRENT_BINARY_DIR}/mpi_sizeofs.mod\"
        \"${CMAKE_CURRENT_BINARY_DIR}/omp_lib.mod\"
        \"${CMAKE_CURRENT_BINARY_DIR}/omp_lib_kinds.mod\"
        \"${CMAKE_CURRENT_BINARY_DIR}/mpif.h\"
    )
")
