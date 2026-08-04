# Module MPC_OpenSHMEM

## Summary

This module is a partial implementation of the [OpenSHMEM specification](http://openshmem.org/site/Specification).
Its primary usecase is to explore the viability of a One-Sided Communications API over the new generation *BXIv3* interconnects.
Mainly, we want to have a lightweight RMA module within MPC.

## Contents

- **include/** : Headers exported as standard interface
  - `shmem.h`: Standard public header containing the routines and constants of the library
  - `pshmem.h`: Standard profiling interface (used for redefinition of the `shmem.h` routines)
  - `shmemx.h`: Extension to the standard API (empty for now)
- **src/** : Module main source files
  - `mpc_openshmem.c`: Implementation of the profiling routines defined in `pshmem.h`
  - `mpc_openshmem_elf`: Definitions and implementation of the ELF parser to retrieve .bss and .data sections
  - `mpc_openshmem_memory`: Definitions and implementation of the stack allocator to manage the symmetric heap
  - `mpc_openshmem_internal.h`: Definitions of the common structures and routines
  - `mpc_openshmem_pweak.h`: Weak linkage of the standard API to the profiling one

## Components

The following list of components is the minimal requirements to use this module as a backend of [Kokkos Remote Spaces](https://github.com/kokkos/kokkos-remote-spaces)
which we aimed to provide

- [x] **General Purpose**
  - [x] Initialization and Finalization routines and constants
  - [x] Queries on the ranks (size, number...)
  - [x] Queries on the library (version, initialized...)
- [x] **Memory Management**
  - [x] Allocation and liberation of symmetric heap memory routines
  - [x] Reallocation
- [x] **One-Sided Blocking Communications**
  - [x] One-Sided Put
  - [x] One-Sided Get
- [ ] **One-Sided NON Blocking Communications**
  - [ ] Non-blocking Put
  - [ ] Non-blocking Get
- [x] **Atomic communications**
  - [x] Atomic Fetch
  - [x] Atomic Fetch + Op
  - [x] Atomic Op
  - [x] Atomic Set
  - [x] Atomic Swap
  - [x] Atomic Compare & Swap
  - [ ] Atomic bitwise operations
- [x] **Synchronization**
  - [x] Global barrier
  - [x] Flush, Quiet
  - [ ] Fence

The unmarked items should be simple to implement but are not required by Kokkos Remote Spaces.
