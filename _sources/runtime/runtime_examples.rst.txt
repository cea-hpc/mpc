Examples
========

Here are a few examples of using mpirun:

Simple run with ``mpirun``:

.. code-block:: console

	$ mpirun -N 4 -p 8 -n 16 -m pthread -net shm -l hydra ./my_mpi_app

This launches ``my_mpi_app`` with 4 nodes, each having 2 processes, and each process
running 4 tasks. It uses pthreads for multithreading and shared memory for
inter-process communication. The launcher is Hydra.

Debug with GDB:

.. code-block:: console

	$ mpirun -N 1 -p 4 -n 4 gdb -ex 'r' ./my_mpi_app

This launches ``my_mpi_app`` using GDB as the debugger.
