Sourcing MPC
============

In order to start using MPC you have to first source it into your environment.
If we consider *$PREFIX* as the prefix to your MPC installation, You can proceed like :

.. code-block:: sh

	# for any bash, zsh, dash shells
	$ source $PREFIX/mpcvars.sh

It will load in your current environment the whole MPC setup, impling a
modification of the following :

- ``__PATH__``
- ``__LD_LIBRARY_PATH__``
- ``__MANPATH__``

By setting the **PATH** variable, the following commands will then be accessible from the shell :

- ``mpc_cc``: the compiler for C
- ``mpc_cxx``: the compiler for C++
- ``mpc_f77``: the compiler for F77
- ``mpcrun`` with MPC's extended semantic
- ``mpc_status`` will provide the configuration information of this mpc instance.
- ``mpc_print_config`` the configuration printer for MPC
- Any mpi\* derivative from the commands above (``mpicc``, ``mpicxx``, ``mpif*``, ``mpirun``...)
