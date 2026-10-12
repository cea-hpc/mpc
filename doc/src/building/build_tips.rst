=================
Installation tips
=================

Modular MPC
-----------

MPC can also be installed directly using CMake (see the ``-LH`` option of
``cmake`` for the available cache variables), in this later case loading and
installing dependencies is to be done manually.

MPC can build with the following REQUIRED dependencies:

- OpenPA (``spack install openpa`` or `from source <https://github.com/pmodels/openpa/>`_)
- HWLOC (``spack install hwloc`` or `from source <https://www.open-mpi.org/software/hwloc/>`_)
- A launcher:
   - Hydra (``spack install hydra`` or `with the sources <http://www.mpich.org/static/downloads/3.2/>`_)
   - PMI (requires libpmi.so ``--with-slurm / --with-pmi1``)
   - PMIx (requires libpmix.so ``--with-pmix``)

It is possible to choose between the following "modules" with ``--enable/disable-X``
(or ``-DMPC_ENABLE_X=ON|OFF`` when building with CMake directly):

- *lowcomm*: the base networking engine for MPC can be manipulated using the mpc_lowcomm.h header
- *threads*: user-level thread support (provides posix thread interface pthread.h, semaphore.h, ..)
- *openmp*: requires *threads* provides OpenMP support (GOMP and Intel ABIs)
- *mpi*: requires *lowcomm* provides the MPI 3.1 interface
- *fortran*: requires *mpi* provides the fortran 77,90,08 bindings for MPC
- *mpiio*: requires *mpi* provides the MPI IO interface (both Fortran and C), disabled with ``--disable-romio``
- *openshmem*: requires *lowcomm* provides the OpenSHMEM interface

For example, a lowcomm only version of MPC can be compiled with ``--disable-mpi --disable-threads``. The
CMake configuration summary (or the mpc_status command) should provide information relative to the enabled components.
