# Module MPC_OpenSHMEM

## Summary

This module is a partial implementation of the [OpenSHMEM specification](http://openshmem.org/site/Specification).
Its primary usecase is to explore the viability of a One-Sided Communications API over the new generation *BXIv3* interconnects.
Mainly, we want to have a lightweight RMA module within MPC.

## Contents

- **include/** : Headers used as interface with other modules
- **src/** : Module main source files

## Components

The following list of components is the minimal requirements to use this module as a backend of [Kokkos Remote Spaces](https://github.com/kokkos/kokkos-remote-spaces)
which we aimed to provide

- [ ] **General Purpose**
  - [ ] Initialization and Finalization routines and constants
  - [ ] Queries on the ranks (size, number...)
- [ ] **Memory Management**
  - [ ] Allocation and liberation of symmetric heap memory routines
- [ ] **One-Sided Blocking Communications**
  - [ ] One-Sided Put
  - [ ] One-Sided Get
- [ ] **Atomic communications**
  - [ ] Atomic Fetch
  - [ ] Atomic Fetch + Op
  - [ ] Atomic Op
  - [ ] Atomic Set
  - [ ] Atomic Swap
  - [ ] Atomic Compare & Swap
- [ ] **Synchronization**
  - [ ] Global barrier
