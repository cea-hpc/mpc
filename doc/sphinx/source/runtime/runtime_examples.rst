Examples
========

Here are a few examples of using mpcrun:

Example 1:

::

	mpirun -N 4 -p 8 -n 16 -m pthread -net shm -l hydra ./my_mpi_app

This launches `my_mpi_app` with 4 nodes, each having 2 processes, and each process 
running 4 tasks. It uses pthreads for multithreading and shared memory for 
inter-process communication. The launcher is Hydra.

Example 2:

::

	mpcrun -dbg=gdb ./my_mpi_app

This launches `my_mpi_app` using GDB as the debugger.