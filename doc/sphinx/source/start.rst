===============
Getting started
===============

.. title:: Getting started

Where to find MPC Documentation?
================================

MPC Documentation is a bit sparse and most of the current references one can have:

- The Official website for the whole project: (http://mpc.hpcframework.com/) 
- In-place documentation (within source code), under Doxygen formatting. Please note that this is currently in progress and not everything is fully documented. Feel free to contact one of the maintainers mentioned in the MAINTAINERS file.
- A complete documentation based on currently-written Doxygen AND Markdown files placed in modules that can be generated. Please consult the /doc/Doxyfile.in to customize the generation to the needs. Once ready, please run ./doc/gen_doc. Be sure to have Doxygen in your PATH along with Graphviz (not mandatory) for drawings. 

Installation guide
==================

This page documents the standard installation process for MPC. First, you may download a tarball from our `download page <https://france.paratools.com/mpc/releases/>`_. MPC embeds some of its dependencies for ease of use and to simplify the installation process. The following main  dependencies are embedded:

- A patched GCC for automatic privatization
- A GCC plugin to support dynamic initializers

Prerequisites
-------------

MPC has a few dependencies which must be installed prior to running the installation script.

.. code-block:: bash

    # Install dependencies on Centos
    yum install -y gcc gcc-gfortran gcc-c++ patch make cmake bzip2 pkg-config curl python36 texinfo diffutils file
    # Install dependencies on Debian / Ubuntu
    apt-get install -y gcc g++ gfortran patch make cmake bzip2 pkg-config curl python
    # Install dependencies on Fedora
    yum install -y gcc gcc-gfortran gcc-c++ patch make cmake bzip2 pkg-config findutils texinfo diffutils file
    # Install dependencies on ArchLinux
    pacman -Su gcc gcc-fortran patch make cmake bzip2 pkg-config python diffutils perl-podlators

Fast Installation
-----------------

.. code-block:: bash

   # Get the last tarball
   curl -k https://france.paratools.com/mpc/releases/mpcframework-4.0.0.tar.gz -o mpcframework-4.0.0.tar.gz
   # Extract the tarball
   tar xf mpcframework-4.0.0.tar.gz
   # Enter the source directory
   cd mpcframework-4.0.0
   # Run the install script
   mkdir BUILD && cd BUILD
   ../installmpc --prefix=<MPC_INSTALL_DIR> -j8
   # Source MPC in your environment
   source <MPC_INSTALL_DIR>/mpcvars.sh

Get MPC and Extract It
----------------------

You may get MPC from the `download page <https://france.paratools.com/mpc/releases/>`_ or directly
from your shell. You can then proceed to extract the tarball.

.. code-block:: bash

   # Get the last tarball
   curl -k https://france.paratools.com/mpc/releases/mpcframework-4.0.0.tar.gz -o mpcframework-4.0.0.tar.gz
   # Extract the tarball
   tar xf mpcframework-4.0.0.tar.gz

Install MPC
-----------

MPC relies on an unified installation script, `installmpc`'. You can run it from the extracted directory.

.. code-block:: bash

   # Run the install script
   mkdir BUILD && cd BUILD
   ../installmpc --prefix=<MPC_INSTALL_DIR> -j8

The `--prefix` option sets the installation prefix directory, and the `-j8` option enables 8 parallel jobs during the build process.

.. note::
   Replace `<MPC_INSTALL_DIR>` with a suitable path to the desired MPC installation directory. The recommended location is `/usr/local`.

For example, on CentOS or Debian/Ubuntu:

.. code-block:: bash

   # Set the installation prefix directory
   export MPC_INSTALL_DIR=/usr/local
   # Enter the source directory
   cd mpcframework-4.0.0
   # Run the install script
   mkdir BUILD && cd BUILD
   ../installmpc --prefix=$MPC_INSTALL_DIR -j8

On Fedora or ArchLinux:

.. code-block:: bash

   # Set the installation prefix directory
   export MPC_INSTALL_DIR=/usr/local/MPC
   # Enter the source directory
   cd mpcframework-4.0.0
   # Run the install script
   mkdir BUILD && cd BUILD
   ../installmpc --prefix=$MPC_INSTALL_DIR -j8


Compiling and launching MPI Applications
----------------------------------------

**mpc_cc**

MPC provides two main commands for launching MPI applications: `mpirun` and `mpcrun`. The `mpirun` command is used to launch an MPI application with the  default settings, while the `mpcrun` command allows you to specify mpc-specific options for your MPI application.

Using MPC to launch MPI applications provides a convenient way to run parallel computations on high-performance computing (HPC) systems. With MPC, you can easily manage the execution of your MPI applications and take advantage of the features and capabilities provided by the HPC system.

Compiling and launching a program :code:`test.c` with mpc should start like this :

::

    mpc_cc test.c -o ./a.out
    mpcrun -n=1 -p=1 -c=1 ./a.out

mpc_cc wraps ap-gcc which is a patched version of gcc allowing privatization. You can pass any option that gcc authorizes.

mpcrun is a multi-process launcher that uses Hydra and Slurm to launch MPI applications. This command provides various options for configuring the launch process, including node number, process number, task number, CPU number per UNIX process, and more.

**mpcrun**

To use `mpcrun`, simply specify the binary executable and any necessary user arguments : ``mpcrun -- binary [user args]``

For example ``mpcrun -- ./my_mpi_app 10000`` launches `my_mpi_app` with the argument `10000`.

For further explanations please refer to :doc:`the complete documentation of mpcrun<runtime>`
