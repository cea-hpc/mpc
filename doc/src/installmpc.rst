===============================
Installmpc script documentation
===============================

Usage
-----

::

    ./installmpc [OPTION]... [VAR=VALUE]...

Defaults for the options are specified in brackets.

Note that depending on your build settings this script might download dependencies.
You may track issued commands using the "-v" flag.

Information
-----------

  :code:`--help|-h|-?` : Display this help and exit

  :code:`--version` : Report version number and exit

Build & Installation
--------------------

  :code:`--prefix=PREFIX` : Install architecture-independent files in PREFIX [/usr/local]

  :code:`--disable-prefix-check` : Force install of MPC over an existing install (not recommended)

  :code:`--download-missing-deps` : Ask MPC to download dependencies for offline install

  :code:`--verbose=1|2|3` : Level of verbosity

  :code:`-v|-vv|-vvv` : Level of verbosity

  :code:`-jN` : Allow N jobs at once (parallel install)

  :code:`--enable-lsp` : Use bear to generate a compile_commands.json file, used by Language Server Protocols
  
  :code:`--disable-lsp` : Do not generate a compile_commands.json file (default)

Features
--------

  :code:`--with-slurm[=prefix]` : Build MPC with PMI support (detect 1 and x)

  :code:`--with-pmi1[=prefix]` : Build MPC with PMI1 support

  :code:`--with-pmix[=prefix]` : Build MPC with PMIX support

  :code:`--process-mode` : Build MPC in process-mode (1 MPI process = 1 UNIX process)

  :code:`--{enable,disable}-mpc-ft` : Activate/Deactivate checkpoint/restart support

  :code:`--{enable,disable}-color` : Disable colors in display

  :code:`--enable-debug` : Enable maximum debug (symbols, messages)

Disable sub packages
--------------------

  :code:`--disable-ofi` : Disable libfabric support

  :code:`--disable-mpc-comp` : Disable MPC compiler additions support

  :code:`--disable-mpcalloc` : Disable MPC allocator

  :code:`--disable-fortran` : Disable fortran

  :code:`--disable-hydra` : Disable Hydra

  :code:`--disable-romio` : Disable ROMIO MPI-IO support

  :code:`--enable-ck` : Enable Concurrency-Kit (spack only) 

  :code:`--{enable,disable}-gdb` : Enable/Disable gdb

  :code:`--{enable,disable}-tbb` : Enable/Disable TBB

Specify system subpackages
----------------------------

  :code:`--with-mpc-comp=*` : Specify mpc compiler additions prefix on the system

  :code:`--with-openpa=*` : Specify openpa prefix on the system

  :code:`--with-hwloc=*` : Specify hwloc prefix on the system

  :code:`--with-cuda=*` : Specify Cuda prefix on the system

  :code:`--with-openacc=*` : Specify OpenACC prefix on the system (NIMPL)

  :code:`--with-opencl=*` : Specify OpenCL prefix on the system (NIMPL)

  :code:`--with-hydra=*` : Specify Hydra prefix on the system

  :code:`--with-tbb=*` : Specify TBB prefix on the system

  :code:`--with-{dmtcp,hbict}=*` : Specify DMTCP/HBICT prefix on the system

MPC Compiler Additions specifics (requires mpc-compiler-additions)
--------------------------------------------------------------------

  :code:`--gcc-version=X.X.X` : Compile mpc-compiler-additions with a given GCC version (default otherwise)

Spack Related Options
---------------------

  :code:`--enable-spack` : Allow MPC to use spack for locating depencies (default)

  :code:`--disable-spack` : Force MPC not to use spack to locate dependencies

  :code:`--enable-spack-build` : Allow MPC to build dependencies with spack

  :code:`--disable-spack-build` : Disallow MPC to build dependencies using spack (default)

  :code:`--disable-build` : Prevent MPC from compiling embedded version of its dependencies

  :code:`--force-spack-package=*` : Force a given version of dependencies loaded/installed with spack

                                            syntax is comma a separated list of DEP:SPACK descriptors for example

                                            openpa:/ffefdkk,mpc-compiler-additions:mpc-compiler-additions@2.0.0, ...

  :code:`--enable-spack-arch` : add architecture specifiers to spack descriptors (default)

  :code:`--disable-spack-arch` : do not pass architecture specifiers to spack descriptors

  :code:`--disable-generic-spack-build` : do not build generic spack packages (portable on target) (default)

  :code:`--enable-generic-spack-build` : build portable for target

  :code:`--force-spack-arch=*` : overrides architecture flag (passes it to packages for example x86_64_v2)

Options to transmit to subpackages (only if not using SPACK)
------------------------------------------------------------

  :code:`--mpc-comp=*` : Add options to mpc-compiler-additions' configure

  :code:`--gdb=*` : Add options to gdb configure

  :code:`--openpa=*` : Add options to openpa configure
  
  :code:`--hydra=*` : Add options to Hydra configure

  :code:`--hwloc=*` : Add options to hwloc configure

  :code:`--mpcalloc=*` : Add options to MPCALLOC configure

  :code:`--ofi=*` : Add options to OFI configure

  :code:`--dmtpc=*` : Add options to DMTCP/HBICT configure

  :code:`--mpcframework=*` : Add options to mpc framework configure