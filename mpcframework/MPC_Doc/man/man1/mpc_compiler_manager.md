% mpc_compiler_manager
% Julien Adam <adamj@paratools.com>

# NAME

mpc_compiler_manager - Manage compilers to use to build applications with MPC

# SYNOPSIS

**mpc\_compiler\_manager** [ **(list|stat|list\_default|add|del|set\_default)**
[c|cxx|fortran] *compiler_name* [*compiler_vendor*] ]

# DESCRIPTION

MPC can rely on a compiler during the installation (depicted as compiler A) and
use another compiler to build applications (compiler B). There is not relation
between compilers A and B. This original feature allows users to compile their
application without having to change the MPI flavor in the process. The C ABI
standard is enough to ensure binary compatibility of programs written in C as
long as the same architecture is used (i.e. an integer is 4-byte long for the
compiler which built MPC as for the compiler which is building applications).
FOr most MPI implementations, issue arises when dealing with Fortran programs.
The MPI fortran module compiled with A (when the MPI flavor has been built) has
to be compatible with application binary, compiled with B. To keep it easier to
maintain, most implementations requires to rebuild everything with the right
compiler to do so. To deal with this issue, MPC will recompiles Fortran module
'on-the-fly' when Fortran compilers differ. This is made possible thanks to
Fortran bindings embedded into MPC installation, but still consuming a very
small amount of disk space.

To deal with these compilers in a dynamic manner, the **mpc\_compiler\_manager**
tool makes the compiler addition/selection/deletion easy to the end user. The
**mpc_compiler_status** is the previous name of the command and is still
maintained for compatibility reasons. However, we encourage new user to use the
**mpc_compiler_manager** instead.

# OPTIONS

**list**
: Displays all compilers currently handlded by MPC for building applications,
indifferently from their language. For each found compiler, The privatization
support is also displayed (whether able to privatize a code or not).

**list_default**
: Same effect as **list** option, but will do so for the default compiler for
each language. It will give in a single view, what is the current set of
compilers used to build applications. This is the same output the `installmpc`
script is giving the installation is complete.

**stat**
: Gives some statistics about known compilers, how many are supported for each
language as well as how many of them are supporting the privatization model.

**add** (c|cxx|fortran) *compiler_path* [*compiler_vendor*]
: Add a new compiler to use to build applications with MPC. It will make it
available only for the current user, not for the global MPC installation. This
is particularly interesting if one single MPC installation is shared among
users, who do not have write access to it. To override this parameter, please
use the `\-\-global` option. `\-\-local` is assumed by default, as long as the
current user is able to write (write-access & quotas) in his HOME directory.
To add a new compiler, one must specify which language it targets. A compiler
name is expected (looked for the PATH if not an absolute PATH) to locate the
binary. Eventually, a 'family' or 'vendor' can be specified to identify an
already known vendor (currently: GNU, INTEL, PGI). In some cases, some
optimization occurs based on the compiler vendor (ex: the OpenMP flag can
differ). `GNU` is assumed by default. During the adding process, the compiler is
tested against privatization support. If not, any privatization-specific options
will be discarded when used. Awarning may also be emitted at compile-time. Note
that a compiler cannot be added twice (IDs are retrieved through a hash
function).


**del** (c|cxx|fortran) *compiler_path* [*compiler_vendor*]
: Is the exact opposite of the `add` rule. It will remove the compiler for the
designated language, if any. The compiler path should be absolute and can be
looked into the PATH variable if there is no match.

**set_default** (c|cxx|fortran) *compiler_path*
: Set the given compiler as the default one to use when invoking a compilation
wrapper. After picking up the language to update, the compiler path must be
absolute (or availabable from PATH). Even if it is still possible to choose a
compiler from the `-cc=` option explicitly, this may be more convenient for
repetitive use.

# HOW IT WORKS

## Structure layout

MPC compiler definition is based on configuration files, one per supported
language in MPC. These files are generated, used and edited by MPC internals, it
is not recommended editing them directly (except if you know exactly what you
want to do...). These files are created in the install path provided during MPC
installation (`\-\-prefix` option). These files are hidden and are named according
to the language they represent : `.'lang'_compilers.cfg`. MPC supports, for
now, three languages : C, C++ and Fortran. Then, you can found the following
files:
	
	${MPC_RPREFIX}/.c_compilers.cfg
	${MPC_RPREFIX}/.cxx_compilers.cfg
	${MPC_RPREFIX}/.cf77_compilers.cfg

`MPC_RPREFIX` representing your MPC installation path. Each file contains
compiler description, one per line. The syntax of a compiler definition is as
follows:

	<FAMILY>:PRIVATIZING?>:<ABSOLUTE_PATH>

Each of these fields are described below:

* `FAMILY`: represents the compiler collection (GNU, INTEL, PGI,...). This field
  will allow us to determine which set of options to use when the compiler is
  called.
* `PRIVATIZING`: This field is set to 'Y' if the compiler supports privatization
  options, 'N' otherwise. We explain later how this value is defined.
* `ABSOLUTE_PATH`: Compiler path (absolute format)

The compiler order in each file is important. Indeed, the first of each file is
considered as the default compiler for the given language. Even if you can edit
the file manually, we recommend using the command-line to do so.

## What happen when installing MPC ?

Compiler Configuration files are generated during the whole MPC installation
process (when you run ./installmpc). A preliminary step consists of detect how
many compilers are available at this point and from which collection they are.
As the order is important, detection order is important. Then, the order, during
MPC building is made like follows:

1. If the user requests internal GCC, add it
2. Else if the user provides an external GCC path (`\-\-with-mpc-gcc` option), add
   it.
3. Detects all the INTEL compiler paths existing in `$PATH` (3 languages)
4. Detects all the GNU compiler paths present in `$PATH` (just for GCC, we then
   suppose that `gcc`, `g++` and `gfortran` are in the same directory).

Items 1 and 2 are exclusive, only one of them can be true at a time. We think
about a way to systematically put INTEL compilers as default but the thing is
that it is not careful to use INTEL compiler as default if MPC process is built
through GNU for example. To resolve this, as you may already know, `installmpc`
provides an option to select the compiler to be used: `\-\-compiler=`. This way,
you can define which compiler to use for the whole MPC building process. In this
case, `GCC_PREFIX` is disabled (even if the user provides an external GCC
path) and INTEL compilers will become the first for each language.

In a general way, it is not really recommended changing of ABI standard (and
thus the compiler collection) between library building and user source code
compilation.  Fortunately, GNU and INTEL have few differences in their standard
for C and C++, and everything seems to go well. But it is not the case in
Fortran. Indeed, some Fortran modules are compiler-specific generated and their
content differs from GNU to INTEL. This way, it is complex to build MPC (and the
modules) with GNU, then compile a code with `ifort`. For now, behavior is
undefined. We plan to make `mpc_f77` aware of this constraint and able to
generate multiple times the Fortran modules during the MPC compilation. This
way, if compilers was available during MPC installation, it will be safe to
change Fortran compilers.

After the build, it is still possible to add, remove, or set as default
compilers thanks to a dedicated manager present in the environment once
`mpcvars.sh` is loaded.

## How it is used by compilation wrappers ? (mpc\_\*)

Thus, by default, mpc compiler scripts will take the first compiler available in
the configuration file for their own language and use it as is. The interest of
compiler collection indication helps the wrapper to determine the argument to
propagate. The user can only use `-fmpc-privatize` whatever the support and the
wrapper will check and set compiler-specific options. It can even warn you if
you attempt to privatize a code while the default compiler does not handle any
privatization support.

Beyond that, you can directly provide from the command line a path to a
compiler. It will be used only for the current run, thanks to the `-cc=` option.
Generally, an absolute path is provided, otherwise, the first available path in
`$PATH` environment will be selected. MPC will then check if this compiler path
is registered inside configuration files. Two cases are possible:

* The path exists : MPC extracts specific information and adapts its behavior
  depending on it (privatization supports, warnings...)
* The path does not exit : MPC warns you for an unknown compiler and simply
  forward all the command line to the given compiler. There will be no checks
  and it’s your own responsibility to ensure the compiler goes well.

Other compiler front-ends are now deprecated. For compatibility reasons, we will
maintain a link to old wrapper names (`mpc_icc, mpc_gcc,...`) but there are not
recommended for use.

# NOTES

This intended to developers who want to add a new compiler support inside this
structure. Everything has been done to facilitate this work. There are only
three updates to make:

1. In `mpcframework/MPC_Tools/mpc_configure_compiler`, you first have to update
   the manager to handle a new compiler. There is no proper way to do it for
   now, just add your own `if` for autodetect privatization support.
2. In the same file, edit (around line 300, try `grep "case \${CC_FAMILY}"` in
   it), add a new branching in the `case`, defining how to handle
   compiler-specific arguments. The function `override_var_if_isset` set the
   variable named by the first argument and set its content with the second
   argument only if the variable is not empty. This trick allows us to redefine
   `PRIV_FLAG` and/or `OMP_FLAG` only if they have been activated through option
   parsing before.
3. in `tools/Common.sh`, in function `setCompilerList()`, you have to add your
   own way to detect the compiler. This function fills initial version of
   compiler configuration file, so be careful with the order you add lines in
   this function.
