===============
Getting started
===============

.. title:: Getting started

-----
TL;DR
-----

Download, install and source the latest version:

.. code-block:: bash

   INSTALL_PREFIX=$PWD/INSTALL
   curl -k https://france.paratools.com/mpc/releases/mpcframework-4.2.0.tar.gz
   tar xf mpcframework-4.2.0.tar.gz && cd mpcframework-4.0.0
   mkdir BUILD && cd BUILD
   ../installmpc --prefix=${INSTALL_PREFIX}
   source ${INSTALL_PREFIX}/mpcvars.sh


-------------
Prerequisites
-------------

MPC has a few dependencies which must be installed prior to running the
installation script. Most of them are available on most systems.

.. code-block:: bash

    # Install dependencies on Centos
    yum install -y gcc gcc-gfortran gcc-c++ patch make cmake automake libtool binutils bzip2 pkg-config curl python36 texinfo diffutils file
    # Install dependencies on Debian / Ubuntu
    apt install -y gcc g++ gfortran patch make cmake automake libtool binutils bzip2 pkg-config curl python3
    # Install dependencies on Fedora
    yum install -y gcc gcc-gfortran gcc-c++ patch make cmake bzip2 pkg-config findutils texinfo diffutils file
    # Install dependencies on ArchLinux
    pacman -Su gcc gcc-fortran patch make cmake bzip2 pkg-config python diffutils perl-podlators automake libtool binutils

Others dependencies are embedded with MPC source distribution and will
downloaded/installed automatically.

-------------------
Get MPC Source code
-------------------

Two ways to retrieve MPC sources:
* From a release (`release page <https://mpc.hpcframework.com/download>`)
* From the Git repository on `Github <https://github.com/cea-hpc/mpc>`

.. code-block:: bash

   # Get the last tarball
   curl -k https://france.paratools.com/mpc/releases/mpcframework-4.2.0.tar.gz
   git clone https://github.com/cea-hpc/mpc.git
   
   # Extract the tarball
   tar xf mpcframework-4.2.0.tar.gz

------------
Building MPC
------------

To ease the installation process of MPC and its dependencies, an unified
installation script is provided , ``installmpc``. You can run it from the extracted
directory. Note that if MPC has been cloned from the Git repository,
``autoconf`` must be called beforehand to generate proper installation
architecture (Makefile.in & configure scripts). From a release distribution,
these files should be already provided.

.. code-block:: bash
   
   # considering this installation prefix (to change accordingly)
   INSTALL_PREFIX=$PWD/INSTALL
   # run if necessary
   ./autogen.sh
   # Run the install script
   mkdir BUILD && cd BUILD
   ../installmpc --prefix=${INSTALL_PREFIX}

This will build MPC and its dependencies and may take a while, especially if the
*thread-based support is enabled (by defaults). To build MPC only in process
mode, one may uses the ``--process-mode`` to the ``installmpc`` script. MPC
supports Spack to reuse already installed packages to shorten
the time to build. This behavior is the default and can be customized with
``--enable/disable-spack``. Considering Spack is loaded to the current shell,
MPC will try to find if deps (hwloc, openpa, libxml2, mpc-compiler-additions...)
before building them. Additionally, MPC can install required dependencies
through Spack to improve future builds by reusing them with
``--enable/disable-spack-build``. This capability is disabled by default to
avoid slowing down Spack installation with undesired packages.

.. caution::

   Installation prefix should be set to avoid "default" installation path  (usually under ``/usr/local``) as MPC provides redefinition of few system headers (like ``pthread.h`` to intercept applications). It could lead to issues when MPC is not loaded. Additionaly, MPC should be installed in a non-root environment.

---------
Using MPC
---------

To load MPC in the current environment, a sourcing script is made available in
the install prefix:

.. code-block::

   source ${INSTALL_PREFIX}/mpcvars.sh

''''''''''''''''''''''
Compiling applications
''''''''''''''''''''''

MPC provides mutiples entry-points to build applications. Beyond the classical
``mpicc``, ``mpicxx/mpic++`` and ``mpif*`` scripts, MPC comes with its own sets
of compiler scripts named after the language it targets: ``mpc_cc``, ``mpc_cxx``
& ``mpc_f77`` (common wrapper for any Fortran standard).

The main difference between these two sets of scripts can be identified when MPC
is built in *thread-based* mode: ``mpi*`` scripts **does not** privatize
applications by default with a *thread-based* installation, while ``mpc_*``
script **will privatize** by default. This is intended to transparently capture
applications assuming ``mpi*`` scripts without silently attempting to privatize
them. This way, applications built with "conventional" MPI wrappers can be
supported. To conveniently enable privatization without propagating any
compilation flag, it can be done through environment variables:

.. code-block:: bash

   # considering a "thread-based" installation
   # compilation with automatic privatization
   mpc_cc main.c
   # compilation WITHOUT automatic privatization
   mpicc main.c

   # All the lines below are equivalent and enables automatic privatization
   mpc_cc main.c
   mpc_cc -fmpc-privatize main.c
   mpicc -fmpc-privatize main.c
   MPI_PRIV=1 mpicc main.c

Without providing any further option, MPC is installed with the
*thread-based MPI* support, implying multiple MPI processes can live within the
same UNIX process. The privatization is implied and any applications 

.. note::
   MPC can be installed in *process-based* configuration by using the ``--process-mode`` option the the ``installmpc`` script.


''''''''''''''''''''
Running applications
''''''''''''''''''''

MPC provides two entry points for launching applications. The regular ``mpirun``
and ``mpcrun``. The latter gives more control on many aspects of running in
complex environments. Most of runtime options are available as configuration
environment variables, most scenarios have a equivalent in both scripts.

.. note::
   While ``mpcrun`` is a Bash-based Shell script, ``mpirun`` is a Python script.

The strength of MPC in thread-based mode is to allow MPI processes to run inside
regular UNIX processes. For this purpose, an extra set of options is added when
defining such parameter is required. Both must be set when running an
application. With ``mpirun``, one can be extrapolated from the other:

.. code-block:: bash

   # Two MPI processes, two UNIX processes
   mpirun -np 2 ./a.out

   # Two MPI processes, one single UNIX processes
   #considering ./a.out as a privatized application
   mpirun -np 2 -p 1 ./a.out
   mpirun -n 2  -p 1 ./a.out
   mpcrun -n=2  -p=1 ./a.out

   # Running on multiple nodes, one UNIX process per node of two MPI processes each
   mpirun -N 2 -n 4 -p 2 ./a.out

   # restrict number of cores bound to EACH process
   # Note:
   # * PpN = '-p' / '-N' = number of processes per node
   # the number of cores required per node (PpN * '-c') must not exceed the 
   # maximum number of cores on a single node
   mpirun -N 2 -n 4 -p 2 -c 4 ./a.out

.. note::
   MPC is configured to run with the launcher detected during the installation phase. To list available launchers (Hydra, SLURM...): ``mpcrun --launch_list``, to then be used with ``-l`` option.