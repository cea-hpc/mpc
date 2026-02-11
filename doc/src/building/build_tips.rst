=================
Installation tips
=================

Modular MPC
-----------

MPC can also be installed directly using the configure script (see ``./configure --help``) in this later
case loading and installing dependencies is to be done manually.

MPC can build with the following REQUIRED dependencies:

- OpenPA (``spack install openpa`` or `from source <https://github.com/pmodels/openpa/>`_)
- HWLOC 1.11.x (``spack install hwloc`` or `using source <http://www.open-mpi.org/software/hwloc/v1.11/>`_)
- A launcher:
   - Hydra (``spack install hydra`` or `with the sources <http://www.mpich.org/static/downloads/3.2/>`_)
   - PMI (requires libpmi.so ``--with-slurm / --with-pmi1``)
   - PMIx (requires libpmix.so ``--with-pmix``)

It is possible to choose between the following "modules" with ``--enable/disable-X``:

- *lowcomm*: the base networking engine for MPC can be manipulated using the mpc_lowcomm.h header
- *threads*: user-level thread support (provides posix thread interface pthread.h, semaphore.h, ..)
- *openmp*: requires *threads* provides OpenMP support (GOMP and Intel ABIs)
- *mpi*: requires *lowcomm* provides the MPI 3.1 interface
- *fortran*: requires *mpi* provides the fortran 77,90,08 bindings for MPC
- *mpiio*: requires *mpi* provides the MPI IO interface (both Fortran and C)

For example, a lowcomm only version of MPC can be compiled with ``--disable-mpi --disable-threads``. The
configure (or the mpc_status command) should provide information relative to the enabled components.

.. note::

   when the configure succeeds once a reconfigure script is provided for convenience it can be used
   to pass new options to the configure. You may have to pass ``--disable-prefix-check`` to acknowledge that
   you may be reinstalling MPC in a non-empty prefix (at your own risks).
