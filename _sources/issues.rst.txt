============
Known issues
============

This page intends to summarize at one place all known bugs one can encountered
when building MPC or using it as a compiler and/or a runtime. Scenarios
described here are known and should not lead to open a "bug" issue but a
discussion can be clearly opened to find a way with their resolution. Before
opening a new issue, please check if the use case does not already exist here.

.. toctree::
    :maxdepth: 2


---------------
Build & Install
---------------

Downstream-to-MPC packages fails with to use proper compiler
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

.. code-block::
    :linenos:
    :emphasize-lines: 1
    :caption: Typical Error Message

    error: cannot run C compiled programs


**Quickfix:** Run `mpc_cc` by hand to see actual output or browse incriminated
package `config.log` in build directory.

**Explanation:** MPC wrappers works improperly. Particularly here, the `mpc_cc`
command does not correctly. A quick way to test the wrapper while not having a
complete installation is to run the `mpc_env.sh` script located in your build
directory, aimed to reproduce MPC environment (like if you sourced the
`mpcvars.sh` directly). To do so, the script take 3 arguments. The two firsts
are host and target directory, the third being the actual command to load. For
instance: `./mpc_env.sh x86_64 x86_64 mpc_cc main.c`

The output of this command would help you to detect the error. Note you can also
directly source in your current environment the mpcframework environment by
sourcing `$PREFIX/$host/$target/bin/mpcvars.sh` where *host* and *target* are,
most of the time `x86_64`.

Common scenarii where this error can occur:
* Bad syntax in MPC wrappers (bug report)
* MPC re-installation in same prefix after an HOME link has been created. Delete
directory mentioned in MPC wrapper output


"mpcframework" fails with hwloc-related error
'''''''''''''''''''''''''''''''''''''''''''''


.. code-block::
    :linenos:
    :emphasize-lines: 1
    :caption: Typical Error Message

    Error: "HWLOC_..." undeclared


**Quickfix:** please ensure to rely on the same version as embedded
within MPC source base.

**Explanation:** Because MPC depends heavily on hwloc types, mismatching hwloc versions in your environment can lead to undefined symbols.

Error with system header not found when compiling GCC
'''''''''''''''''''''''''''''''''''''''''''''''''''''


.. code-block::
    :linenos:
    :emphasize-lines: 1
    :caption: Typical Error Message

    fatal error: sys/ustat.h: No such file or directory

**Quickfix**: disable the thread-based approach by providing `--disable-mpc-gcc`
from the `installmpc` script. Do not consider using `-fmpc-privatize` in such
scenario !

**Explanation**: Privatizing compiler coming with MPC is GCC 7.2.0 (currently)
and needs an header called `ustat.h`, deprecated for a long time. Recent glibc
(above 2.28) removed this system header, preventing GCC 7.2.0 to compile
correctly. This includes recent Ubuntu installation (equal or later than 18.04).
There is no solution to this currently, we are working on supporting a compiler
recent enough to dismiss this error.

-------------------------
MPC to build applications
-------------------------

Complain about already existing installation
''''''''''''''''''''''''''''''''''''''''''''


.. code-block::
    :linenos:
    :emphasize-lines: 1
    :caption: Typical Error Message

    A previous backup exists. Please remove <...> directory first!


**Quickfix:** (if you know what you're doing OR you never used the
compiler manager), you can safely remove the incriminated directory (with and
without `-backup` extension).

**Explanation:** MPC has been re-installed within the same prefix **AFTER** an
user-level compiler manager has been invoked. This use will create a home-based
configuration link (under `~/.mpcompil/ directory`). To detect re-installation
and prevent users to rely on outdated configuration, a fatal error is emitted
once a re-installation (=same prefix) is done while a local custom configuration
exist.

GCC compiler not found
''''''''''''''''''''''


.. code-block::
    :linenos:
    :emphasize-lines: 1
    :caption: Typical Error Message

    Compiler "<...>/mpc-gcc_620" not found


**Quickfix**: erase the whole installation directory along with the relevant
`~/.mpcompil` subdirectory (a complete list is available through `mpc_cleaner`).

**Explanation:** This nasty bug comes after a GCC release update. For a long
time, MPC relied on GCC 6.2.0. When decided to update to GCC 7.2.0, MPC
re-installation (=same prefix) leads to an error into compiler manager files as
old compiler path was still referenced. A fix has been made for that and is now
merged into the devel branch.


-----------------------
MPC to run applications
-----------------------


Unable to load MPC configuration
''''''''''''''''''''''''''''''''

.. code-block::
    :linenos:
    :emphasize-lines: 1
    :caption: Typical Error Message

    Error while loading MPC configuration files`

**Quickfix:** Try to run it by yourself and investigate the output. If you
edited the configuration (or used the `mpcrun --config` option), you have
improperly formatted the XML files.

**Explanation:** Error with running correctly the `mpc_print_config` command.
Other scenarii where this bug can occur:
* `mpc_print_config` can be built on compute nodes but is run during starting
process and is run on the login node. An architecture mismatch can lead to such error.
