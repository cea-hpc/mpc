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
	git clone git@github.com:spack/spack.git
	echo ". $SCRATCHDIR/spack/share/spack/setup-env.sh" >> $HOME/.bashrc

Prepare Offline Bootstrap
-------------------------

.. code-block:: sh

	#
	# FIRST CHECK LOCAL SPACK IS THE SAME AS THE REMOTE ONE!
	#
	https://spack.readthedocs.io/en/latest/bootstrapping.html#creating-a-mirror-for-air-gapped-systems
	# On machine with the NET
	spack bootstrap mirror  --binary-packages \[XXX\]/spack_bootstrap_mirror
	# Copy spack_bootstrap_mirror to INTI
	scp -r ./spack_bootstrap_mirror \[XXXX\]
	# Add the bootstrap mirror
	spack bootstrap add --trust local-sources \[XXX\]/spack_bootstrap_mirror/metadata/sources
	spack bootstrap add --trust local-binaries \[XXX\]/spack_bootstrap_mirror/metadata/sources
	# Now trigger clingo install
	spack spec hdf5

Add MPC Repositories
--------------------

To do so you need access to `\~mpc`

.. code-block:: sh

	# Add MPC repository
	spack repo add ~mpc/MPC_Spack/mpc_spack_repo
	# Add MPC mirror (all deps for MPC and Clingo see below)
	spack mirror add mpc ~mpc/MPC_Spack/mpc_spack_mirror

Use With MPC
------------

By default MPC will try to load corresponding spack packages and they should be found when running a standard installmpc such as:

.. code-block:: sh

	srun --pty -N 1 -c 32 -p haswell  ../installmpc --enable-color --with-slurm --prefix=$SCRATCHDIR/mpcinst
