
Runtime tips
============

Running a debugger (like gdb) in a separate terminal for each MPI process
-------------------------------------------------------------------------

You may prefix your application name (ex: `./a.out`) with terminal emulator commands. Then, you may provide to the emulator, the GDB command to run. The `-ex 'r'` argument below let gdb starts immediately without waiting for input:

	mpcrun -N=4 -n=4 -p=4 -c=1 xterm -e gdb -ex 'r' ./a.out

or (example on Skylakes)

.. code-block:: sh

	ccc_mprun -N 1 -n 1 -c 24 -x -p sklb -E "--x11=all" xterm -e gdb -ex 'r' --args ./a.out

It is even better to rely on GDB script. Such script contain, multiple lines, each of them being directly injected to GDB prompt. For instance, a script to insert a breakpoint each time `sctk_malloc` is called would look like:

.. code-block:: sh

	set pagination off
	set logging file file.out
	set logging on
	set breakpoint pending on
	break sctk_malloc
		bt
		continue
	end
	run
	set logging off
	quit

Sometimes, it may be interesting to debug only a single process (without knowing in advance which one). For instance, consider the situation where you want to investigate each time a function call `sctk_network_notify_idle_message()`. Putting a breakpoint seen by any process would be a nightmare. To circumvent this, an MPC macro named `GDB_BREAKPOINT()` can be used to inject a livelock in  MPC codebase (you may use it to fit your needs). When GDB reaches this livelock, it will have no other solution that to wait for your Ctrl+C. The output will explain you how to remove the livelock and start a step-by-step debugging from there. The advantage here is the control you have to set your breakpoint. You may only set it for a subset of processes. Once the livelock occur, You may run the debugger only for livelocked processes. To attach an existing process, use `gdb attach` when logged onto the node. 

But here is a better solution thanks to GDB server capability. Run your program similarly to this :

.. code-block:: sh

	mpcrun ... gdbserver host:8080 ./a.out


Then, consider a script like this named `gdb.sh`:

.. code-block:: sh

	nodename="inti"
	nodeset="1000 10001 10003 10004"
	for i in $nodeset; do
	cat<<EOF > R$i
	target remote $nodename$i:8080
	continue
	EOF
		xterm -e gdb --command R$i &
	done


It will start an XTerm session for each process, each running on a separate compute node. You may then interact without SSH-ing. Another solution is also to retrieve the incriminated process (and its hostname) and run the gdb command directly from prompt, allowing you to debug one MPI process at a time !