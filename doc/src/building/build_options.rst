===================
Compilation options
===================


Files location and external modules configuration
-------------------------------------------------

Unless building with sudo you should specify a prefix for the installation. Use
absolute paths to avoid any issue. For a quick install you can use these
commands :

.. code-block:: console

    $ mkdir $HOME/installmpc installmpc --prefix=$HOME/installmpc

In case you are building MPC offline you should place the external modules
sources in the BUILD directory or in the $HOME/.mpc-deps. You can get all
dependencies sources zipped archives with ``installmpc --download``. MPC
installed dependencies are the following :

- openpa
- Hydra
- OFI
- mpc-compiler-additions
- mpc-allocator
- libfabric



MPC will look for pre-installed modules although you can specify paths for
system modules. The option to specify a module path is ``--with-[module]=``.
The option to disable a module is ``--disable-[module]``

Additionally you can specify paths for :

- cuda
- opencl
- openacc
- tbb

MPC modules are installed in the same prefix as MPC. It is possible to
reconfigure modules and install them again if you have any specific needs.


Installation Process
--------------------

.. note::

   MPC can also be used in a modular fashion (component by component)
   if this is of interest please refer to section "Modular MPC" as the rest
   of this section covers the "bundle install" of MPC using ./installmpc

Once you downloaded the archive from the website, you may proceed as follows.
Note that by default, if you run the ``installmpc`` script directly into the
source directory, a ``build`` directory will be automatically created to avoid
squashing source and build altogether.

.. code-block:: console

	$ tar xf MPC_$VERSION.tar.gz
	$ cd MPC_$VERSION
	$ mkdir BUILD && cd BUILD
	$ ../installmpc --prefix=$PREFIX

The compiler will be detected by default a privatizing compiler is compiled. To
avoid any ambiguous detection, one can specify the specific compiler to use
through the ``--compiler`` option. For example, using such command with the Intel
compiler will avoid MPC to build the embedded patched GCC compiler.

MPC relies on a number of dependencies (openpa, hwloc...) and some may be
missing in the archive you downloaded (for archive size reasons). Considering
the current installation process can have an internet connection, on might
consider using the ``--download-missing-deps`` option. Any of the dependency can
be overridden with a custom, already installed version, anywhere on the system.
An option exist for each, similar to ``--with-<dep>=/path``. In these cases, it is
up to the user to ensure the compatibility between the external dependency and
MPC's requirement. A complete list of dependency versions can be found in
``utils/config.txt``. On the contrary, to forward extra arguments when building embedded
dependencies, one can use ``--<dep>-<arg>``. Here two examples for HWLOC support :

.. code-block:: sh

	$ ./installmpc --with-hwloc=/app/hwloc/
	$ ./installmpc --hwloc-disable-cuda # forwarded to hwloc configure as --disable-cuda

Beyond these basic options, some useful others are listed and detailed below :

* ``--disable-autopriv`` if you are not intending on running MPI processes inside
  threads and therefore do not need the privatization support
* ``--enable-debug`` to enable full debugging for MPC purpose. Will build
  MPC in debug mode *AND* will build the GDB support for MPC.
* ``--disable-prefix-check`` to confirm you want to erase any existing
  installation specified by $PREFIX. Not used by default to prevent accidental
  overrides.
* ``--enable-color`` if MPC should be build with terminal color support.

.. warning::

   if you intend to rely on SLURM you should build MPC as follows (by default MPC is build with hydra support):

.. code-block:: console

	$ ../installmpc --prefix=$PREFIX -j8 --with-slurm

Any MPC option (which can be found by running ``./configure --help``)
can be provided when invoking the ``installmpc`` script through the ``--mpc-option``
argument. Ex:

.. code-block:: console

	$ ./installmpc --prefix=$PREFIX -j8 --mpcframework="--enable-cma"
