===============
Getting started
===============

.. title:: Getting started

Installation guide
==================

This page documents the standard installation process for MPC. First, you may download a tarball 
from our `download page <https://france.paratools.com/mpc/releases/>`_. MPC embeds some of its 
dependencies for ease of use and to simplify the installation process. The following main 
dependencies are embedded:

- A patched GCC for automatic privatization
- A GCC plugin to support dynamic initializers

Prerequisites
-------------

MPC has a few dependencies which must be installed prior to running the installation script.

.. list-table::
   :widths: 25 75
   :header-rows: 1

   * - Dependency
     - Installing on CentOS, Debian/Ubuntu, Fedora, and ArchLinux
   * - patch command
     - `yum install -y patch` or `apt-get install -y patch` or `yum install -y gcc-patch` or `pacman -Su patch`
   * - C/C++ compiler (e.g. gcc and g++)
     - `yum install -y gcc gcc-c++` or `apt-get install -y gcc g++` or `yum groupinstall "Development Tools"` or `pacman -Su group base-devel`
   * - Fortran compiler (e.g. GFORTRAN)
     - `yum install -y gfortran` or `apt-get install -y gfortran` or `yum install -y  gcc-gfortran` or `pacman -Su gcc-fortran`
   * - The make command
     - Installed by default on most distributions
   * - Tar and Bzip commands
     - `yum install -y tar bzip2` or `apt-get install -y tar bzip2` or `yum groupinstall "Development Tools"` or `pacman -Su tar bzip2`
   * - pkg-config
     - `yum install -y pkg-config` or `apt-get install -y pkg-config` or `yum install -y gcc-config` or `pacman -Su pkg-config`
   * - curl
     - `yum install -y curl` or `apt-get install -y curl` or `yum install -y httpd-tools` or`pacman -Su curl`
   * - python3 (for CentOS and Debian/Ubuntu) or python (for Fedora and ArchLinux)
     - `yum install -y python3` or `apt-get install -y python3` or `yum groupinstall "Development Tools"" (Fedora: `yum install python3`) or `pacman -Su python`
   * - texinfo
     - `yum install -y texinfo` or `apt-get install -y texinfo` or `yum install -y texlive texlive-texinfo` or `pacman -Su texinfo`
   * - diffutils and file
     - Installed by default on most distributions

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

MPC relies on an unified installation script, `installmpc`'. You can run it from the extracted 
directory.

.. code-block:: bash

   # Run the install script
   mkdir BUILD && cd BUILD
   ../installmpc --prefix=<MPC_INSTALL_DIR> -j8

The `--prefix` option sets the installation prefix directory, and the `-j8` option enables 8 
parallel jobs during the build process.

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


Launching MPI Applications
--------------------------

MPC provides two main commands for launching MPI applications: `mpirun` and `mpcrun`. The `mpirun` command is used to launch an MPI application with the  default settings, while the `mpcrun` command allows you to specify mpc-specific options for your MPI application.

Using MPC to launch MPI applications provides a convenient way to run parallel computations on high-performance computing (HPC) systems. With MPC, you can easily manage the execution of your MPI applications and take advantage of the features and capabilities provided by the HPC system.

Compiling and launching a program :code:`test.c` with mpc should start like this :

::

    mpc_cc test.c -o ./a.out
    mpcrun -n=1 -p=1 -c=1 ./a.out

