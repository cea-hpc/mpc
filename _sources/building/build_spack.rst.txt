Using spack
===========

Preamble
--------

You should be able to use the basics of spack to install MPC using this guide.
For extra help on Spack, you can check out `the official Spack documentation
<https://spack.readthedocs.io/en/latest/>`_.

Initial Deploy
--------------

.. code-block:: sh

	cd $SCRATCHDIR
	git clone https://github.com/spack/spack.git
	echo ". $SCRATCHDIR/spack/share/spack/setup-env.sh" >> $HOME/.bashrc

Prepare Offline Bootstrap
-------------------------

To bootstrap Spack itself on a machine without network access, first create a
mirror on a connected machine (see `the Spack bootstrapping documentation
<https://spack.readthedocs.io/en/latest/bootstrapping.html#creating-a-mirror-for-air-gapped-systems>`_
for details), then copy it to the target machine and register it:

.. code-block:: sh

	# On a machine with network access
	spack bootstrap mirror --binary-packages /path/to/spack_bootstrap_mirror
	# Copy the mirror to the offline machine
	scp -r ./spack_bootstrap_mirror user@offline-machine:/path/to/
	# Add the bootstrap mirror
	spack bootstrap add --trust local-sources /path/to/spack_bootstrap_mirror/metadata/sources
	spack bootstrap add --trust local-binaries /path/to/spack_bootstrap_mirror/metadata/sources
	# Now trigger clingo install
	spack spec hdf5

Add MPC Repositories
--------------------

You first need to retrieve the MPC spack repository and its dependency mirror
from the MPC team, then register both:

.. code-block:: sh

	# Add MPC repository
	spack repo add /path/to/mpc_spack_repo
	# Add MPC mirror (all dependencies for MPC)
	spack mirror add mpc /path/to/mpc_spack_mirror

Use With MPC
------------

By default MPC will try to load corresponding spack packages and they should be found when running a standard installmpc such as:

.. code-block:: sh

	srun --pty -N 1 ../installmpc --enable-color --with-slurm --prefix=$SCRATCHDIR/mpcinst
