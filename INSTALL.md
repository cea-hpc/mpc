# Installing MPC {#installmpc}

Dependencies
------------

In order to be able to install MPC you will need the following dependencies:

- libibverbs (for Infiniband support)
- libportals4 (for Portals4 support)
- patch
- tar
- make
- gcc
- gfortran
- python
- g++
- libncurses-devel (if built with GDB)

Others dependencies linked to provided packages are included and directly managed
through the install script.

Installation Process
--------------------

```sh
tar xf MPC_$VERSION.tar.gz
cd MPC_$VERSION
mkdir BUILD
cd BUILD
../installmpc --prefix=$PREFIX -j8
```

The following options might be used:

* __--disable-mpc-gdb__ if you are no interested in debugging user-level threads with MPC's build in GDB
* __--disable-mpc-gcc__ if you are not intending on running MPI processes inside threads and therefore do not need the privatization support
* __--enable-color__ if MPC should be build with terminal color support

__WARNING__: if you intend to rely on SLURM you should build MPC as follows (by default MPC is build with hydra support):

```sh
../installmpc --prefix=$PREFIX -j8 --with-slurm
```

Sourcing MPC
------------

In order to start using MPC you have to first source it into your environment (*$PREFIX* is the prefix you have used for installing MPC):

```sh
source $PREFIX/mpcvars.sh
```

This done you should have access to the mpc commands:

MPC Commands
------------

* __mpc_cc__ the MPC privatizing compiler for C
* __mpc_cxx__ the MPC privatizing compiler for C++
* __mpc_f77__ the MPC privatizing compiler for F77
* __mprcun__ the equivalent of _mpirun_ with MPC's extended semantic
* __mpc_status__ a way to get the installed version of MPC
* __mpc_print_config__ the configuration printer for MPC
* __mpc_compiler_manager__ the compiler manager for MPC to add new compiler to an existing installation of MPC

