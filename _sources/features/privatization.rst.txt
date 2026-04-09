=============
Privatization
=============

**What is Privatization?**
Privatization, as used in this context, refers to the process of taking global variables and making them local to a specific task or thread within a shared-memory context. This allows for multiple tasks or threads to run in parallel, each with their own instance of the variable, without affecting other tasks or threads.

**Why is Privatization Necessary?**
In traditional MPI programming models, global variables are shared among all processes, which can lead to data replication and memory issues when running on many-core architectures. Privatization helps alleviate these issues by making each task or thread have its own copy of the variable, reducing the need for inter-process communication and improving performance.

**How is Privatization Achieved?**
Privatization in MPC is achieved through compiler-based transformations that extend the Thread-Local Storage (TLS) model. The `__task` keyword is introduced to create a new level between thread-local and process-level variables, allowing tasks to have their own context and share data with other tasks.

**Compiling a Privatized Program**

Privatization in MPC can be achieved by compiling a program with `mpc_cc`, a compiler wrapper
similar to `mpicc`. This allows developers to utilize privatization features while still
benefiting from the flexibility and scalability of MPI.

To compile a privatized program using `mpc_cc`, you can use this line:

.. code-block:: bash

    mpc_cc -fmpc-privatize myprogram.c -o ./a.out

the :code:`-fmpc-privatize` is not required since privatization is activated by default in MPC unless mpc-compiler-additions is disabled on build. To disable privatization you can use :code:`-fno-mpc-privatize`. Let's try with a C code that would be problematic on thread-based runtime without privatization:

.. code-block:: c

    #include <stdio.h>
    #include <mpi.h>

    int rank;

    int main(int argc, char **argv){
        MPI_Init(&argc, &argv);
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        // here rank is not mixed up when running thread-based
        return 0;
    }

The previous compilation command line ensures the following code behaves properly when launched with the following mpcrun options :

.. code-block:: bash

    mpcrun -n=2 -p=1 ./a.out
