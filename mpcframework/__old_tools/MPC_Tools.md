Module MPC_Tools
================

SUMMARY:
--------

This module gathers structure & configuration around the MPC runtime.
Especially, you will found here configure & generators for the launcher, the
configuration layer , etc...

CONTENTS:
---------
* **config/**                    : default configuration files
* **cross\_compilation\_scripts/** : MIC script to enable cross-compilation
* **fortran\_gen/**               : Interface to build Fortran to C MPI interface
* **mpcrun\_opt/**                : mpcrun laucher scripts
* **sctk\_configs/**              : (probably) deprecated
* **mpirun/**                    : 'mpirun' binary code (standalone)
* **mpit/**                      : python-based MPIT interface generator

COMPONENTS:
-----------

### COMPILATION

Each \*\_configure\_\* script is in charge of configuring subparts of the MPC
configure script. Each file is assigned to a chunk of the configuration. 

The first important script is (probably) `mpc_configure_compiler`. This script
aims to build the compilation scripts like `mpc_cc`, `mpc_cxx`, etc. This script
is based on a template `printf_compiler`, called multiple with different
parameters, to build the major scripts. Then, it provides templates to build
deprecated symlinks (retro-compatibility).

#### COMPILER MANAGER

An another useful script here is `mpc_configure_compiler_manager`. The manager
is in charge of compiler selection once MPC is built. As a remind, MPC does not
need to be completely rebuild for each compiler. This is time and space saving,
but need a special handling. First, build compilers are automatically added to
the list of available compilers, because it is the one MPC is compiler against.
For C/C++ language support, we don't need to recompile the 'mpcframework'
library because of ABI compliance. For Fortran language, parts of the library
needs to be recompiled and especially the MPI modules `*.mod`, by each compiler.

The above script will build a command named `mpc_compiler_manager`, in charge of
managing them. Its usage is pretty simple: 
```{sh}
Usage: mpc_compiler_manager OPERATION LANGUAGE PATH FAMILY

LANGUAGE belongs to {c,cxx,fortran} (default: c)
PATH is the compiler path (default '1')
FAMILY is the compiler type: INTEL, GNU... (default: GNU)
OPERATION is one of the following:
   list: shows, for each known compiler, the priv. support
   stat: shows a summary for each language (number of compilers + priv. support)
   list_default: list, for each language, the default compiler used

   add: adds a new compiler to the manager
   del: removes a known compiler from the manager
   set_default: set the given compiler as default (adds it first if not known)

   check: Verify compiler integrity against stored hash value.
   help: Print this help.

Extra options, which can be used in scripts to parse compilers:
An special value for PATH can be set: '1' means 'the first compiler in the list'
   get: returns the compiler path if exist in configuration (lookup).
   get_detail: like get, but returns all the data (lookup).
```

You can use this command for adding, modifying or deleting compilers. In case of
Fortran compilers, a preliminary step for recompiling MPI modules is
automatically triggered when the comman is invoked. But most of the time, you
can directly control the compiler you want to use through the `-cc=` option to
MPC compilation wrappers. When passing this argument, the manager is invoked to
find the associated compiler (instead of loading the default). If the
installation does not know it, it will add it immediatly. In this case, the
compilation overhead can be slightly increased (noticeably if adding a Fortran
compiler).

The point of sharing an MPC installation between users is resolved by putting
this whole management thing **in the user HOME space**. This was the only way to
share the same installation while separating the compilation configuration
process. The drawback of this is that Fortran modules are directly stored inside
the HOME. In some rare cases, this can slow the application booting step if the
HOME filesystem is less efficient (than scratch partition, for example).

#### INSTALL CLEANER

More often used by developer team, users can also use a tool aiming to manage
multiple MPC installations. The `mpc_configure_install_cleaner` will build the
command `mpc_cleaner`. It helps you to list / remove MPC installation referenced
in your HOME directory. Each time you install MPC, a link is made in your HOME
to reference it (under `~/.mpcompil`). It creates an SHA1SUM-based directory,
containing the file to the MPC install path. You can then choose to list, remove
the 'dangling' HOME reference or even remove complete installation.
Because this command can seriously be destructive, it requires to provide the
`--force` (no short option) to ask for removing anything. The following shows up
the command help:
```{sh}
Usage: mpc_cleaner --force [--list|-l] [--dry-run|-d] [--rm-link] [--rm-install] [--color|-c]
```

#### GENERATORS
The generators are designed to be called to generate MPC parts by the dev team.
Users should not be required to run them. Among the generators, you can find:
* mpc_gen_all_makefiles: rebuild all module Makefiles
* mpc_gen_makefile: rebuild one specific Makefile
* mpc_gen_runtime_config: Rebuild the configuration (MPC_Config) from the
  config-meta.xml files. This needs to be called each time a new field is
  appended to a file.

### RUNTIME

All scripts in the mpcrun/ directory are scripts callable directly from mpcrun.
You can list them with `mpcrun --launch_list`. Each line is mapped to one of
these files (prefixed with 'mpcrun\_').
