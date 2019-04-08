# Installing MPC {#installmpc}

Dependencies
------------

In order to be able to install MPC you will need the following dependencies:

- `patch`
- `tar` / `bzip2` / `gunzip`
- `basename` / `dirname`
- `which`
- `make`
- a base compiler (`gcc`, `icc`...)
- `python` (can be optional)`
- `libncurses-devel` (to have full GDB support)
- `libibverbs` (for Infiniband support)
- `libportals4` (for Portals4 support)

Most of them are available on every systems, but fresh new OS installations can
not provide them by default. Be sure to have the mandatory ones before
continuing. At least one compiler is required, the detection should be done by
the installation process, In case it fails, please consult the documentation for
expliciting the compiler through the dedicated option. Others dependencies
linked to provided packages are included and directly managed through the
install script.

Installation Process
--------------------

Once you downloaded the archive from the website, you may proceed as follows.
Note that by default, if you run the `installmpc` script directly into the
source directory, a `build` directory will be automatically created to avoid
squashing source and build altogether.

	$ tar xf MPC_$VERSION.tar.gz
	$ cd MPC_$VERSION 
	$ mkdir BUILD && cd BUILD
	$ ../installmpc --prefix=$PREFIX -j8

It is higly recommended to provide the `--prefix` option to avoid MPC to be
installed in system path (like /usr, or /usr/local). Because MPC redefines many
system headers, such an omission would erase them definitively. To avoid such a
pain, please keep using the `--prefix` option.

The compiler will be detected by default, looking for Intel and then GCC. To
avoid any ambiguous detection, one can specify the specific compiler to use
through the `--compiler` option. For example, using such command with the Intel
compiler will avoid MPC to build the embedded patched GCC compiler.

MPC relies on a number of dependencies (libxml, hwloc...) and some may be
missing in the archive you downloaded (for archive size reasons). Considering
the current installation process can have an internet connection, on mighyt
consider using the `--download-missing-deps` option. Any of the dependency can
be overriden with a custom, already installed version, anywhere on the system.
An option exist for each, similar to `--with-<dep>=/path`. In these cases, it is
up to the user to ensure the compatibility between the external dependency and
MPC's requirement. A complete list of dependency versions can be found in
`config.txt`. On the contrary, to forward extra arguments when building embedded
dependencies, one can use `--<dep>-<arg>`. Here two examples for HWLOC support :

	$ ./installmpc --with-hwloc=/app/hwloc/
	$ ./installmpc --hwloc-disable-cuda #forwared to hwloc configure as
--disable-cuda

Beyond these basic options, some useful others are listed and detailed below :

* `--disable-mpc-gcc` if you are not intending on running MPI processes inside
  threads and therefore do not need the privatization support
* `--enable-mpc-debug` to enable full debugging for MPC purpose. Will build
  MPC in debug mode *AND* will build the GDB support for MPC.
* `--disable-check-install` to confirm you want to erase any existing
  installation specified by $PREFIX. Not used by default to prevent accidental
  overrides.
* `--enable-color` if MPC should be build with terminal color support.

__WARNING__: if you intend to rely on SLURM you should build MPC as follows (by
default MPC is build with hydra support):

	$ ../installmpc --prefix=$PREFIX -j8 --with-slurm

Any MPC option (which can be found by running `./mpcframework/configure --help`)
can be provided when invoking the `installmpc` script through the `--mpc-option`
argument. Ex:

	$ ./installmpc --prefix=$PREFIX -j8 --mpc-option="--disable-MPC_Allocator"

Sourcing MPC
------------

In order to start using MPC you have to first source it into your environment.
If we consider *$PREFIX* as the prefix to your MPC installation, You can proceed
like :

	# for any bash, zsh, dash shells
	$ source $PREFIX/mpcvars.sh
	# for csh and tcsh
	$ source $PREFIX/mpcvars.csh

It will load in your current environment the whole MPC setup, impling a
modification of the following :
* __PATH__
* __LD_LIBRARY_PATH__
* __MANPATH__

By setting the **PATH** variable, the following commands will then be accessible
from the shell :

* `mpc_cc`: the compiler for C
* `mpc_cxx`: the compiler for C++
* `mpc_f77`: the compiler for F77
* `mpc_f90`: the compiler for F90
* `mpc_f08`: the compiler for F08
* `mpc_icc`: Intel-based compiler for C. You can direcly use `mpc_cc` with Intel
  if MPC has been built with Intel. This particular commands is only intended to
  use MPC with Intel compiler while it hasn't be compiled against this compiler.
* `mpc_icpc`: Intel-based compiler for C++. Same as above
* `mpc_ifort`: Intel-based compiler for Fortran. Same as above. Thanks to MPC's
  dynamic compiler support, Fortran modules included in final binaries can be
  rebuilt on the fly. Thus, there is no restrictions to use GCC-based MPC with
  an Intel compiler in Fortran.
* `mpc_compiler_manager` the compiler manager for MPC to add new compiler to
  an existing installation of MPC. Will give the capability to load a different
  compiler than the one which built MPC for your application (as long as it is
  C ABI compliant (for instance, an integer is always X bytes, indifferently
  from the compiler))
* `mprcun` `mpirun` with MPC's extended semantic
* `mpc_status` a way to get the installed version of MPC. The `mpcrun --version`
  command will give you the launcher version.
* `mpc_print_config` the configuration printer for MPC
* Any mpi\* derivative from the commands above (`mpicc`, `mpicxx`, `mpif*`,
  `mpirun`...)

