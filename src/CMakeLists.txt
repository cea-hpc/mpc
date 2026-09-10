# MPC src/CMakeLists.txt
# Add subdirectories in dependency order

# Documentation (no build dependencies)
add_subdirectory(MPC_Doc)

# Always built
add_subdirectory(MPC_Arch)
add_subdirectory(MPC_Config)
add_subdirectory(MPC_Common)
add_subdirectory(MPC_Topology)
add_subdirectory(MPC_Launch)

# Conditional modules
if(MPC_ENABLE_THREADS)
    add_subdirectory(MPC_Threads)
endif()

if(MPC_ENABLE_OPENMP)
    add_subdirectory(MPC_OpenMP)
endif()

if(MPC_ENABLE_LOWCOMM)
    add_subdirectory(MPC_Lowcomm)
endif()

if(MPC_ENABLE_MPI)
    add_subdirectory(MPC_MPI)
endif()

if(MPC_ENABLE_OPENSHMEM)
    add_subdirectory(MPC_OpenSHMEM)
endif()

if(MPC_ENABLE_MPIIO)
    add_subdirectory(MPC_IO)
endif()

if(MPC_ENABLE_FORTRAN)
    add_subdirectory(MPC_Fortran)
endif()

# Final assembly
add_subdirectory(lib)
add_subdirectory(bin)
