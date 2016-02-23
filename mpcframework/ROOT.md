Multi-Processor Computing (MPC) Framework Documentation
=======================================================

Introduction
------------

The MPC (Multi-Processor Computing) framework provides a unified parallel
runtime designed to improve the scalability and performances of applications
running on clusters of (very) large multiprocessor/multicore NUMA nodes.

MPC is available under the [CeCILL-C][CCC_LNK] license, which is a French
transposition of the LGPL and is fully LGPL-compatible.

MPC conforms to three standards :
  * [POSIX Threads][PTH_LNK]
  * [OpenMP 3.1][OMP_LNK]
  * [MPI 1.3][MPI_LNK]
  
All these standards can be mixed together in an efficient way, thanks to process
virtualization. MPC has been ported on x86, x86_64 with Linux and OpenSolaris
systems and supports TCP and InfiniBand interconnects.

Features
--------

### Thread Library ###

MPC comes with its own MxN thread library and POSIX Thread implementation. MxN
thread libraries provide lightweight user-level threads that are mapped to
kernel threads. One key advantage of the MxN approach is the ability to optimize
the user-level thread scheduler to create and schedule a very large number of
threads with a reduced overhead. The MPC thread scheduler provides a polling
method that avoids busy-waiting and keeps a high level of reactivity for
communications, even when the number of tasks is much larger than the number of
available CPU cores. Furthermore, collective communications are integrated into
the thread scheduler to enable efficient barrier, reduction and broadcast
operations.

### Memory Allocator ###

The MPC thread library comes with a thread-aware and NUMA-aware memory allocator
(malloc, calloc, realloc, free, memalign and posix_memalign). It implements a
per-thread heap to avoid contention during allocation and to maintain data
locality on NUMA nodes. Each new data allocation is first performed by a
lock-free algorithm on the thread private heap. If this local private heap is
unable to provide a new memory block, the requesting thread queries a large page
to the second-level global heap with a synchronization scheme. A large page is a
parametrized number of system pages. Memory deallocation is locally performed in
each private heap. When a large page is totally free, it is returned to the
second-level global heap with a lock-free method. Pages in second-level global
heap are virtual and are not necessarily backed by physical pages. On a same
node, memory pages freed by a thread are provided to new allocations of other
threads without any system call.

### Thread debugging ###

Support for debugging user-level MPC threads is provided thanks to an
implementation of the libthread_db and a patch to the GNU Debugger (GDB). It
allows to manage user-level threads in GDB and all GUIs based on GDB. It is also
compatible with SUN’s DBX debugger.

### OpenMP ###

MPC supports compilation and execution of C/C++ and Fortran OpenMP applications
thanks to its built-in OpenMP 3.1 runtime. This OpenMP implementation supports
GNU and Intel compilers. The main MPC package contains a patched version of GCC
(4.8.0) called MPC_GCC which is automatically installed when building MPC. The
OpenMP implementation is also compatible with Intel C/C++ and FORTRAN compilers
starting at version 15.0. Finally, the OpenMP runtime has been optimized to
efficiently support large NUMA architectures and hybrid MPI/OpenMP codes.

### Thread safety ###

Because MPC provides a thread-based MPI implementation, a mechanism is needed to
deal with global-variable sharing in the application. Such thread-safety issues
are managed with automatic privatization of global variables. This mechanism is
automatically appplied by the compiler for C/C++ and Fortran MPI codes through
the new -fmpc-privatize option. This new option is available in MPC_GCC
(provided with the MPC package) and Intel compilers (starting with version 15.0
for C/C++ and Fortran).

### Hierarchical Local Storage (HLS) ###

HLS is a set of directives in C, C++ and Fortran allowing the application
developer to share global variables across MPI tasks running on a same node. The
HLS extension can be used to reduce the memory footprint of MPI programs by
avoiding to duplicate data that are common to all MPI tasks. One typical use
case of HLS variables is a large table of physical constants. With the HLS
extension, only one table per node will be allocated instead of one table per
core (if there is one MPI task per core) thus reducing the memory consumption by
a factor equal to the number of cores per node.

### MPI ###

MPC’s implementation of MPI fully respects the MPI 1.3 standard. It also
provides an efficient MPI_THREAD_MULTIPLE support (MPI2 feature). MPC’s
communications are implemented in the following way: intra-node communications
involve two tasks in a unique process (MPC’s default mode uses one process per
node). These tasks use the optimized thread-scheduler polling method and
thread-scheduler integrated collectives to communicate with each other. As far
as inter-node communications are concerned, MPC uses direct access to the TCP or
InfiniBand interconnect. MPC provides performances close to MPICH2 or OpenMPI,
but with a much better support of hybrid programming models (e.g., MPI/PThreads,
MPI/OpenMP, …) and lower memory consumption.\cite{perache2008mpc}


Installation Guide
------------------

To avoid documentation duplication, please refer to our dedicated MD file
available [here][INSTALL_MD]

Getting Started
---------------
As previously said, "Getting Started" guide is provided as external resource and
is available [here][GETTING-STARTED_MD]

Credits
-------
The MPC framework is developed by the *Commissariat à l'Energie Atomique et aux
Energies Alternatives*, aliased by [CEA][CEA_LNK] which is a french
government-funded research organization, with a special focus on High
Performance Computing challenges. The list of maintainers and contributors is
available [here][MAINTAINERS_MD]

[CCC_LNK]: http://www.cecill.info/index.en.html "CeCILL-C License"
[PTH_LNK]: http://pubs.opengroup.org/onlinepubs/007904975/basedefs/pthread.h.html "POSIX Threads Standard"
[OMP_LNK]: http://www.openmp.org/mp-documents/OpenMP3.1.pdf "OpenMP 3.1 standard"
[MPI_LNK]: http://www.mpi-forum.org/docs/mpi-1.3/mpi-report-1.3-2008-05-30.pdf "MPI 1.3 Standard"
[CEA_LNK]: http://www.cea.fr/english-portal/ "CEA Home Page"

[INSTALL_MD]: ../../../INSTALL
[GETTING-STARTED_MD]: ../../../GETTING-STARTED
[MAINTAINERS_MD]: ../../../MAINTAINERS
