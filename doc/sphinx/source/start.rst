===============
Getting started
===============

Launching MPI Applications with MPC
-----------------------------------


Installing MPC
--------------

The Multi-Processor Computing (MPC) environment provides multiple commands that can be used to launch Message Passing Interface (MPI) applications. The main commands for launching MPI applications are :code:`mpirun` and :code:`mpcrun`. In order to have these commands available, MPC must be installed  by building the source code available on the mpc.hpcframework.com website.

::

    curl https://fs.paratools.com/mpc/mpcframework-4.1.0.tar.gz --output mpcframework-3.1.0.tar.gz


MPC will use apcc the mpc-compiler-additions (autopriv) as compiler in a general case. You can use the :code:`installmpc` script to launch a full installation of all external modules along with mpcframework. Make a temporary build directory for safer procedures.

::

    mkdir BUILD
    cd BUILD
    $MPCPATH/installmpc --prefix=$YOUR_PREFIX

(Replace MPCPATH with the location of your mpc clone).
You can find every available option for installmpc with the :code:`installmpc --help` command or :doc:`in the online documentation<installmpc>`

Alternatively you can install every external modules and launch a classic :code:`configure`, :code:`make`, :code:`make install`. Once installed, MPC provides a set of commands that can be used to launch MPI applications.


Launching MPI Applications
--------------------------

MPC provides two main commands for launching MPI applications: `mpirun` and `mpcrun`. The `mpirun` command is used to launch an MPI application with the  default settings, while the `mpcrun` command allows you to specify mpc-specific options for your MPI application.

Using MPC to launch MPI applications provides a convenient way to run parallel computations on high-performance computing (HPC) systems. With MPC, you can easily manage the execution of your MPI applications and take advantage of the features and capabilities provided by the HPC system.

Compiling and launching a program :code:`test.c` with mpc should start like this :

::

    mpc_cc test.c -o ./a.out
    mpcrun -n=1 -p=1 -c=1 ./a.out