# CHANGELOG


## Changes in **MPC 3.4.0**

**MPI**

- Optimizations and corrections of nonblocking collectives
- Bug fix on virtual topologies (MPI_Cart...)

**OpenMP**

- First OMPT support
- Bug fix on tasks with dependencies

**Network / communication layers**

- Support of MPI HW-enabled operations through Portals4 driver

**Privatization / compilers**

- MPC default privatizing compiler is now GCC 7.3.0

**Active Message**

- Integration of custom Active Message relying on gRPC approach

**Misc**

- Addition of this Changelog file
- New doc structure through Markdown
- Many other small optimizations and bug fixes


## Changes in **MPC 3.3.1**

**MPI**

- Message latency optimization
- Message progress with copy engine
- MPI wrappers (mpirun) bugfix
- NBCs, Collectives and Communicators bugfixes


**Misc**

- General source-code cleanup


## Changes in **MPC 3.3.0**

**MPI** 
        
- C/R support (Shmem, TCP & IB handlings) 
- Collective algorithms optimizations 
- NBC communication progress improvements
- Shared-memory shortcut set qfor intra-node comms

**OpenMP** 
        
- Support for OMP_PLACES 
- Tasking bug fixes 
- OMPT Stabilization
    
**Network / communication layers**

- Full rewrite for the Portals 4 driver (only process-mode)
    

**Privatization / compilers** 
        
- GCC 7.2.0 compatibility (not set as default)

**Miscs** 

- Spack recipes for MPC, and patched GCC 
- Fix for older Autotools 
- Process-mode performance optimizations 
- mpc_compiler_manager optimizations and Fortran bug fixes 
- Improve installmpc procedure when GCC is not required 
- Docker recipes for CentOS/Debian 
- New mpcrun options related to placement debugging 
- Various bug fixes 
- Documentation


## Changes in **MPC 3.2.0**

**MPI**

- Support for MPI RMA (3.1) 
- MPI-T Support 
- Fortran 2008 modules 
- Shared-memory optimization of collectives 
- Stabilization of the NBCs 
- Several bug-fixes     

**OpenMP**
 
- Support for the GOMP ABI 
- Support for OMP tasks with dependencies 
- OMPT Support 
- Several bug-fixes 
    

**Privatization / compilers**
        
- Support for privatized CUDA contexts and privatization of Cuda programs using a dedicated compiler wrappers (mpc_nvcc) 
- Fix support for optimized TLS with icc 
- Update to GCC 6.2 as the default privatizing compiler 
- Initial support for ARM 
- Several bug-fixes 
    
**Miscs** 
        
- Add a process mode for the installation (–mpc-process-mode) 
- On the fly Fortran module generation 
- Enhance compiler manager and install management 
- Add an “mpc_cleaner” command 
- For now TBB has been disabled by default



## Changes in **MPC 3.1.0**

**OpenMP**
- Major bugfixing in OpenMP interface

**Privatization / compilers**

- Rewrite of the TLS/HLS support
- Support for all linker level optimizations
- Definition of new C and C++ keywords
- Outlining of the exTLS library separating TLS from MPC
- Full support for global variable privatization in C
- Support for dynamic intializers in C to handle some TLS edge cases

**Misc**

- New compiler selector (from command line)



## Changes in **MPC 3.0.0**

**MPI**

- Integration of ROMIO and MPI-IO
- Integration of Non-Blocking Collectives (NBC) (excluding IO)
- Add Fortran90 support (.mod)
- Support for heterogeneous data-types in collectives
- Collectives on inter-communicators and IN_PLACE related fixes
- Various improvements on data-types and collectives
- Request management in the MPI interface was optimized
- Fixes on MPI topologies
- Various Bugfixes
    
**OpenMP**

- Fixes in the Intel OpenMP interface
- Corrections on topology support

**Network / communication layers**

- Generic Multi-rail support with gates (from XML configuration file)
- Support for Portals 4
- Support for SHM
- Addition of a generic device detection engine with distance matrix (HWLOC-based)
- Improved topological polling in Infiniband (from device detection)
- Outline of a « low-level » communication interface (cont. p2p messages and RDMA)
- Add low-level RDMA (for IB and Portals) and implement emulated calls

**Misc**

- Support for a privatized version of Getopt
- Internal implementation of asynchronous IO for old libc (AIO threads are launched with small stacks)
- Add support for a library mode for embedding inside another MPI runtime



## Changes in **MPC 2.5.2**

**MPI**

- Extended data-type support (up to MPI 3.0)
- Optimized data-types (flattening and reuse)
- Extended Generic Request and extended generic request class support
- External32 data-representation support
- MPI_Info support
- Several fixes to the Fortran interface
- Various bugfixes

**OpenMP**
    
- Add support for Intel(r) OpenMP ABI (run OpenMP applications compiled with ICC)



## Changes in **MPC 2.5.1**

**OpenMP**

- OpenMP optimizations

**Misc**

- New build system
- Xeon-Phi Support
- Cross compilation support
- Various Bugfixes


##Changes in **MPC 2.5.0**

**MPI**

- Performance optimizations

**OpenMP**

- New OpenMP runtime (NUMA optimizations)
- Performance optimizations

**Privatization / compilers**

- Patched GCC 4.8.0 with privatization of global objects
    
**Misc**

- Many bug fixes



## Changes in **MPC.2.4.1**

**Allocator**
- New memory allocator
 

**Misc**

- Bug fixes
- Performance optimizations



## Changes in **MPC 2.4.0**

**MPI**

- Collaborative polling
 

**Privatization / compilers**

- Enhancement of automatic privatization

**Misc**
    
- Bug fixes
- Performance optimizations



## Changes in **MPC 2.3.1**

**Privatization / compilers**
- Bug fixes in HLS support



## Changes in **MPC 2.3.0**

**MPI**

- Bug fixes with MPI programming model

**OpenMP**

- Bug fixes with OpenMP programming model

**Privatization / compilers**

- HLS (Hierarchical Local Storage) support

**Misc**

- Scalable and flexible launcher based on HYDRA
- Bug fixes in the installation process



## Changes in **MPC 2.2.0**

**MPI**
 
- Bug fixes with MPI programming model

**OpenMP**

- Bug fixes with OpenMP programming model

**Network / communication layers**

- New InfiniBand support

**Privatization / compilers**

- Extended TLS support

**Misc**

- Bug fixes in the installation process


## Changes in **MPC 2.1.0**

**MPI**

- Bug fixes with MPI programming model

**OpenMP**

- Bug fixes with OpenMP programming model

**Network / communication layers**

- SHM module (shared memory communications between processes) 

**Misc**

- Bug fixes in the installation process



## Changes in **MPC 1.1**

**MPI**

- MPI-compliant API (MPI 1.3)
- MPI_Cancel support
- MPI_THREAD_MULTIPLE support

**OpenMP**

- First OpeMP support (OpenMP 1.3)

**Network / communication layers**

- Full Infiniband support

**Debug**

- Debugger support with a patched version of GDB

**Misc**

- Small bug fixes
