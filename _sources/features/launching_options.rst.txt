==========================
Advanced Launching Options
==========================

----------------
Launcher options
----------------

Options passed to the launcher options should be compatible with the launch mode
chosen during *configure*. For more information you might read the
documentations of mpiexec and srun respectively for Hydra and Slurm.

- **Hydra**: If MPC is configured with Hydra, mpcrun should be used with
  ``-l=mpiexec`` argument. Note that this argument is used by default if not
  specified.
- **SLURM**: If MPC is configured with SLURM, mpcrun should be used with ``-l=srun``
  argument.

''''''''''''''''
Mono-process job
''''''''''''''''

In order to run an MPC job in a single process with Hydra, you should use on of
the following methods (depending on the thread type you want to use).

.. code-block:: console

    $ mpcrun -m=ethread      -n=4 hello_world
    $ mpcrun -m=ethread_mxn  -n=4 hello_world
    $ mpcrun -m=pthread      -n=4 hello_world

To use one of the above methods with SLURM, just add *-l=srun* to the command
line.

''''''''''''''''''''''''''''''''''
Multi-process job on a single node
''''''''''''''''''''''''''''''''''

In order to run an MPC job with Hydra in a two-process single-node manner with
the shared memory module enabled (SHM), you should use one of the following
methods (depending on the thread type you want to use). Note that on a single
node, even if the TCP module is explicitly used, MPC automatically uses the SHM
module for all process communications.

.. code-block:: console

    $ mpcrun -m=ethread      -n=4 -p=2 -net=tcp hello_world
    $ mpcrun -m=ethread_mxn  -n=4 -p=2 -net=tcp hello_world
    $ mpcrun -m=pthread      -n=4 -p=2 -net=tcp hello_world

To use one of the above methods with SLURM, just add ``-l=srun`` to the command
line. Of course, this mode supports both MPI and OpenMP standards, enabling the
use of hybrid programming. There are different implementations of inter-process
communications. A call to ``mpcrun --help`` details all the available
implementations.

'''''''''''''''''''''''''''''''''''
Multi-process job on multiple nodes
'''''''''''''''''''''''''''''''''''

In order to run an MPC job on two nodes with eight processes communicating with
TCP, you should use one of the following methods (depending on the thread type
you want to use). Note that on multiple nodes, MPC automatically switches to the
MPC SHared Memory module (SHM) when a communication between processes on the
same node occurs. This behavior is available with all inter-process
communication modules (TCP included).

.. code-block:: console

   $ mpcrun -m=ethread      -n=8 -p=8 -net=tcp -N=2 hello_world
   $ mpcrun -m=ethread_mxn  -n=8 -p=8 -net=tcp -N=2 hello_world
   $ mpcrun -m=pthread      -n=8 -p=8 -net=tcp -N=2 hello_world

Of course, this mode supports both MPI and OpenMP standards, enabling the use of
hybrid programming. There are different implementations of inter-process
communications and launch methods. A call to ``mpcrun --help`` detail all the
available implementations and launch methods.

'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Multi-process job on multiple nodes with process-based MPI flavor
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

MPC offers two flavors of its MPI implementation: process-based and thread-based.
Process-based implementation is similar to the behavior of OpenMPI or MPICH.
The flavor can be directly driven by the launch arguments.
To use MPC-MPI in process-based flavor, the number of MPI processes specified
in the launch command should be the same as the number of OS processes specified
(hence ``-n`` value should be the same as ``-p`` value).
The following examples launch eight MPI processes on two nodes, in process-based
flavor. Each node will end up with four OS processes, each of these OS processes
embedding a MPI process.

.. code-block:: console

   $ mpcrun -m=ethread      -n=8 -p=8 -net=tcp -N=2 -c=2 hello_world
   $ mpcrun -m=ethread_mxn  -n=8 -p=8 -net=tcp -N=2 -c=2 hello_world
   $ mpcrun -m=pthread      -n=8 -p=8 -net=tcp -N=2 -c=2 hello_world

Note that the ``-c`` argument specified the number of cores per OS process.
In the ebove examples, we can deduce that the nodes we want to work on have
at least 8 cores, with four OS processes per node, and two cores per OS
process.

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Multi-process job on multiple nodes with thread-based MPI flavor
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

MPC-MPI was developed to allow a thread-based flavor, e.g. inside a node,
MPI processes are threads. This flavor allows sharing constructs through
the shared memory. Communications between MPI processes on the node are just
data exchange in the shared memory, without having to go through the SHM module.
The following examples launch eight MPI processes on two nodes, in thread-based
flavor. Each node will end up with one OS process, each of these OS processes
embedding four MPI processes which are threads.

.. code-block:: console

   $ mpcrun -m=ethread      -n=8 -p=2 -net=tcp -N=2 -c=8 hello_world
   $ mpcrun -m=ethread_mxn  -n=8 -p=2 -net=tcp -N=2 -c=8 hello_world
   $ mpcrun -m=pthread      -n=8 -p=2 -net=tcp -N=2 -c=8 hello_world

Since the ``-c`` argument specifies the number of cores per OS process, and not
per MPI process, the user need to give a number of cores per OS process at least
equals to the number of MPI process per OS process to avoid oversubscribing.
In the examples above, we will have four MPI process per OS process and eight
cores per OS process, hence two cores per MPI process.

The examples above display an example of full thread-based: one OS process per node,
and all MPI processes on the node are threads. It is also possible to have a
mitigated behavior: several OS processes per node, and multiple MPI processes which
are thread in each of these OS processes.
The following examples show such a behavior:

.. code-block:: console

   $ mpcrun -m=ethread      -n=8 -p=4 -net=tcp -N=2 -c=4 hello_world
   $ mpcrun -m=ethread_mxn  -n=8 -p=4 -net=tcp -N=2 -c=4 hello_world
   $ mpcrun -m=pthread      -n=8 -p=4 -net=tcp -N=2 -c=4 hello_world

With these values for ``-N``, ``-n``, ``-p``, each node will end up with two OS processes
and four MPI processes. It implies that each OS process embeds two MPI processes
which are threads. Then, to have two cores per MPI process, it is necessary to
ask four cores per OS process.

-----------------
Launch with Hydra
-----------------

In order to execute an MPC job on multiple nodes using *Hydra*, you need to
provide the list of nodes in a *hosts* file and set the HYDRA_HOST_FILE
variable with the path to the file. You can also pass the host file as a
parameter of the launcher as follow:

.. code-block:: console

    $ mpcrun -m=ethread -n=8 -p=8 -net=tcp -N=2 --opt='-f hosts' hello_world

See `Using the Hydra Process Manager`_ for more information about hydra hosts file.

.. _Using the Hydra Process Manager: https://wiki.mpich.org/mpich/index.php/Using_the_Hydra_Process_Manager
