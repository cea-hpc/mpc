Module MPC_Fault_Tolerance
==========================

SUMMARY:
--------

In this module, you will find the checkpoint/restart implementation for MPC.
It is actually relying exclusivelry on DMTCP but can be extented to other
implementation through macros.

CONTENTS:
---------
* **include/**  : Main header files used by other MPC modules
* **src/**      : Source files for the fault-tolerance module 

COMPONENTS:
-----------

The C/R system is currently relying on DMTCP. The purpose of DMTCP can be found
[here](http://dmtcp.sourceforge.net/). The version used when the solution was
integrated is the 2.5.1 and has been slightly modified (see patch in extern-deps) to
be compliant with HPC and MPC in a more efficient way.

C/R has to be activated through `--enable-mpc-ft` option to installmpc. This
will build DMTCP, HBICT and enable required flags to build MPC with C/R support

mpcrun will support two options:
* `--checkpoint`: Enable C/R for the application to run. This will run a
  dedicated coordinator and activate a `dmtcp_launch` wrap on each process.
* `--restart`: mpcrun will ignore (almost) all other options and will run
  `./dmtcp_start_script.h` to restart the application (default path can be
  changed by using `--restart=<path>`). Be aware that DMTCP will check that the
  current job allocation will match with the allocated-one when the application
  was running.

### CHECKPOINT

A new MPI function: MPIX_Checkpoint(MPIX_Checkpoint_state). The argument will
contain the application state after the call, which can be CHECKPOINT,RESTART or
IGNORED (disabled C/R). This high-level call is in charge of calling this FT
module only once per process. The FT module exposes 7 functions to checkpoint : 

1. sctk_ft_init: called once, it will configure DMTCP callbacks
2. sctk_ft_checkpoint_init: initialize a new checkpoint procedure. This will
   ensure that the application enter a critical C/R section.
3. sctk_ft_checkpoint_prepare: Prepare the checkpoint. As an example, it is
   calling disconnection protocols for network not supporting DMTCP interrupts.
   This cannot be done during the init() becasue upper-layer has to do some
   things between them (synchronisations).
4. sctk_ft_checkpoint: emit a checkpoint request to DMTCP coordinator. This
   should be invoked only once for the whole application. The MPI layer will
   ensure that. Other processes will call the next function
5. sctk_ft_wait: wait for checkpoint completion (per-process basis)
6. sctk_ft_checkpoint_finalize: Complete the current checkpoint. As an example,
   it will re-enable closed networks.
7. sctk_ft_finalize: relase C/R resources

To let developers create critical sections where the library should not be
interrupted, two calls `sctk_ft_{enable,disable}` are available.

### RESTART

The restart process does not differ from a post-checkpoint procedure, as
depicted by the list above. The difference is that `sctk_ft_wait` will return
a RESTART status instead of CHECKPOINT.
