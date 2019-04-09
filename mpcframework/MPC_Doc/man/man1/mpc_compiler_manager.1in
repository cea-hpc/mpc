HOWTO : Compiler Dynamic Switch
===============================

This documentation file is designed to help users and developers in the compiler
switch usage inside MPC framework. In order to make this document easier to
read, we split it into five sections :

1. **MPC compiler : Structure** : We will detail how supported compilers are
   handled in MPC construction process.
2. **MPC compiler : Building** : We will see why compiler choice in MPC
   installation process matters.
3. **MPC compiler : Management** : Once built, users can permanently affect
   which compiler can be used by MPC. In this section, we explain how to use the
   'compiler manager' to configure MPC compiler wrappers.
4. **MPC compiler : Usage** : After the configuration, we will present small
   examples of `mpc_*` usage and how you can still change the compiler.
5. **MPC compiler : Support** : Dedicated to developers, this section will
   explain the work to do in order to make MPC support a new compiler (or
   collection) in the easiest way.

MPC compiler : Structure
------------------------

MPC compiler definition is based on configuration files, one per supported
language in MPC. These files are generated, used and edited by MPC internals, it
is not recommended editing them directly (except if you know exactly what you
want to do...). These files are created in the install path provided during MPC
installation (`--prefix` option). These files are hidden and are named according
to the language they represent : `.'lang'_compilers.cfg`. MPC supports, for
now, three languages : C, C++ and Fortran. Then, you can found the following
files : 
	
	${MPC_RPREFIX}/.c_compilers.cfg
	${MPC_RPREFIX}/.c++_compilers.cfg
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
the file manually, we recommend seeing our section, '[MPC compiler :
Management][]'

MPC compiler : Building
-----------------------

Compiler Configuration files are generated during the whole MPC installation
process (when you run ./installmpc). A preliminary step consists of detect how
many compilers are available at this point and from which collection they are.
As the order is important, detection order is important. Then, the order, during
MPC building is made like follows:

1. If the user requests internal GCC, add it
2. Else if the user provides an external GCC path (`--with-mpc-gcc` option), add
   it.
3. Detects all the INTEL compiler paths existing in `$PATH` (3 languages)
4. Detects all the GNU compiler paths present in `$PATH` (just for GCC, we then
   suppose that `gcc`, `g++` and `gfortran` are in the same directory).

Items 1 and 2 are exclusive, only one of them can be true at a time. We think
about a way to systematically put INTEL compilers as default but the thing is
that it is not careful to use INTEL compiler as default if MPC process is built
through GNU for example. To resolve this, as you may already know, `installmpc`
provides an option to select the compiler to be used: `--compiler=`. This way,
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

MPC compiler : Management
-------------------------
In order to allow the user to easily interact with compiler configuration files,
MPC provides an executable dedicated to file management: `mpc_compiler_manager`.
This replaces the old `mpc_compiler_status` command (but still present for
retro-compatibility reasons). This manager handles four modes (in addition to a
`--help` mode) for interaction with compiler environments inside MPC.

### Displaying ###
For displaying current compiler support inside MPC, the manager proposes two
commands :

	mpc_compiler_manager (list|stat) [(c|c++|f77)]

The first argument set the type of displaying. While `list` simply list the
available compilers and test each one to verify their privatization support,
`stat` provides some information per language like the number of detected
compilers and how many are currently supporting privatization. The second
argument is optional and allows you to filter for which language you want to
display the information.

### Adding ###
The first modifier mode we present is the 'adding' one. To add a new
compiler, you have to provide exactly four arguments:

	mpc_compiler_manager add (c|c++|f77) compiler_path compiler_collection

The first argument request the addition mode. The second argument describes the
language to add a compiler. The third one set the path to the compiler you want
to add. It has to be an absolute path, this argument represents the 'index'. In
case of duplication, some unexpected behaviors can occur. In any case, the path
is tested to ensure a real existence before insertion. The fourth argument is
the compiler collection. It defines which set of options will be used by
the compiler wrapper (as you may know, GNU and INTEL does not have the same
privatization option string and we have to make the difference). Generally, the
family is written in uppercase and is one of {GNU, INTEL} but nothing restricts
you that.

The manager will test by itself the privatization support. That's why it is
important the collection compiler you provide fits with an existing one.
Moreover, the compiler will be added only if provided compiler path does not
already exist in a previous one.

### Removing ###
The next modifier mode we present after 'adding' is 'removing'. For
this, you have to provide the same four arguments, as we present them in
[Adding][] section:

	mpc_compiler_manager del (c|c++|f77) compiler_path compiler_collection

To make your life easier, we support `del, delete, remove` and `rm` as deletion
mode keywords. If the compiler path does not exist, nothing happens. As
previously, the key used in regexes is the compiler absolute path.


### Change default compiler ###
The last modifier mode is the most useful in case of MPC environment already
deployed with multiple compilers. It is designed to set a given compiler as the
default one for a language. This simply means putting the compiler on top of the
file.
This mode has exactly three arguments:

	mpc_compiler_manager set_default (c|c++|f77) compiler_path

As usual, the first one depicts the mode (here: set a compiler as default) and
the second one describes the language where the alteration takes place. The last
argument provides the compiler absolute path.


MPC compiler : Usage
--------------------
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

MPC compiler : Support
----------------------
This last section is intended to developers who want to add a new compiler
support inside this structure. Everything has been done to facilitate this
work. There are only three updates to make:

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
