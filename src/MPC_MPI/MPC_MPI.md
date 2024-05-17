Module MPC_MPI
======================

SUMMARY:
--------

Module providing MPI compatibility layer, implementing the Message Passing
interface standard.

CONTENTS:
---------
* **include/** : Headers used as interface with other modules
* **src/**     : module main source files
* **mpc/**     : MPC-specific interface for MPI standard
* **mpi/**     : MPI-compatibility layer redirecting to MPC calls


COMPONENTS:
-----------

### Nonblocking collectives (NBC)

MPC provides two implementations for the support of nonblocking collectives: one based on progress threads and one based on Generic Request.

#### libNBC

<p>The first implementation provided by MPC for NBC support is based on the [libNBC library v.1.0.1](https://spcl.inf.ethz.ch/Research/Parallel_Programming/NB_Collectives/LibNBC/).
The libNBC library offers the possibility to spawn a progress thread to provide better overlap.</p>


###### How libNBC works
<p>A collective communication is a set of point-to-point communications and operations if needed. To realize the collective communications, all the ranks should realized all the steps defined by the chosen algorithm. Several aglortihms can be implemented depending on the collective communication called. Knowing the number of ranks in the provided communicators, the selected algorithm and its rank number, the current MPI process knows the exact sequence of steps (communications and operations) to do.

To store this sequence of steps, libNBC uses its own NBC_Schedule structure. This is a memory area allocated to the size of all combined steps and their argument.
For example, to perform a send, the size of the structure will be increased with the size of one NBC_Fn_type structure (which will receive the value to inform that the function to be called is a send) and with the size of one NBC_Args_send structure (which will store the arguments for this specific step).
All steps that can be performed in parallel can be launched without any synchro. If steps have dependencies (scan for example - receive -> compute -> send), barriers are inserted with the insertion of a Sched_barrier value.</p>



#### Generic requests




### Component #2
