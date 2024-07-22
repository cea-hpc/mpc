Installation tips
=================

Rebuild & install a package from MPC toolchain without re-running the whole `installmpc` script
-----------------------------------------------------------------------------------------------

Each MPC dependency is decompressed under `$BUILD_DIR/$host/$target`. Each sudirectory is built through its own `configure` file with `./configure --prefix=... && make && make install` (some can have a `build` directory instead of building directly into sources, like GCC & Binutils). For a quick edit to a dependency witout re-running the whole installation process, simply run, in the **dependency root directory**:

	make install

This will just rebuild the dependency. To go further with rebuilding part of the toolchain

Resuming `installmpc` process after interruption
------------------------------------------------

From the buil directory, re-run the installation script. Please take care to keep the same prefix. You can also run the `make` command from build directory. But note that this will avoid final checks and compiler manager setup to occur (located at the end of `installmpc` script)

Enabling "debug" mode for MPC
-----------------------------

Multiple ways:

* argument `--enable-debug` to `installmpc` to enable debug symbols and debug messages (`sctk_info`...)

* argument `--enable-gdb` to enable MPC_Debugger (= the patched GDB)

* MPC-specific options through `--mpc-option` to `installmpc`, to select with
  finer grain what should be enabled. These option are forwarded to MPC
  configure "as is".

* If MPC is already installed and you still have the build, a `reconfigure`
  script in `$BUILDIR/$host/$target/mpcframework` lets you re-run the MPC configuration script.
  Any given option is appended (to the ones received when the first installation
  has been made). This allows you, for instance, to enable debug afterward. Do
  not forget to run `make && make install` after reconfiguring for changes to
  take effect.