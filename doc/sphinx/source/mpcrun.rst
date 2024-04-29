mpcrun documentation
====================

Usage 

::

    mpcrun [option] [--] binary [user args]

Launch Configuration
--------------------

	:code:`-N=n,--node-nb=n`                Total number of nodes
	:code:`:code:`-p=n, --process-nb=n`            Total number of UNIX processes
	:code:`-n=n, --task-nb=n`               Total number of tasks
	:code:`-c=n, --cpu-nb=n`                Number of cpus per UNIX process
	:code:`--cpu-nb-mpc=n`                  Number of cpus per process for the MPC process
	:code:`--enable-smt`                    Enable SMT capabilities (disabled by default)
	:code:`--mpmd`                          Use mpmd mode (replaces binary)

Multithreading
--------------

	:code:`-m=n, --multithreading=n`        Define multithreading mode
                                        modes: pthread ethread_mxn pthread_ng ethread_mxn_ng ethread ethread_ng

Network
-------

	:code:`-net=n, --net=, --network=n`     Define Network mode

	Configured CLI switches for network configurations (default: tcpshm):

	- shm:
		* tbsmmpi
		* shmmpi

	- tcpshm:
		* tbsmmpi
		* shmmpi
		* tcpofirail

	- tcp:
		* tbsmmpi
		* tcpofirail

	- verbs:
		* tbsmmpi
		* verbsofirail

	- verbsshm:
		* tbsmmpi
		* shmmpi
		* verbsofirail

Launcher
--------

	:code:`-l=n, --launcher=n`              Define launcher

	Available launch methods (default is hydra):
	   - hydra
	   - none
	   - none_mpc-gdb
	   - salloc_hydra

	:code:`--opt=<options>`                 Launcher specific options
	:code:`--launchlist`                    Print available launch methods
	:code:`--config=<file>`                 Configuration file to load.
	:code:`--profiles=<p1,p2>`              List of profiles to enable in config.


Informations
------------

	:code:`-h, --help`                      Display this help
	:code:`--show`                          Display command line

	:code:`-v,-vv,-vvv,--verbose=n`         Verbose mode (level 1 to 3)
	:code:`--verbose`                       Verbose level 1
	:code:`--graphic-placement`             Output a xml file to visualize thread placement and topology for each compute node
	:code:`--text-placement`                Output a txt file to visualize thread placement and their infos on the topology for each compute node

Debugger
--------

	:code:`--dbg=<debugger_name>` to use a debugger