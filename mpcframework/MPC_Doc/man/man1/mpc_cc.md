% MPC Compilation wrappers
% Julien Adam <adamj@paratools.com>

# NAME

mpc\_cc, mpc\_cxx, mpc\_f77, mpc\_f90, mpc\_f08 - MPI/OpenMP compilation wrappers

# SYNOPSIS

**mpc\_cc** \[options\] **file...**

**mpc\_cxx** \[options\] **file...**

**mpc\_ff77** \[options\] **file...**

**mpc\_f90** \[options\] **file...**

**mpc\_f08** \[options\] **file...**

# DESCRIPTION

The purpose of this wrapper is to provide to the user a convenient manner to
build MPI/OpenMP applications with a minimum effort. All the flags required for
the application to be linked to the libraries are transparently provided by the
wrapper. The simplest invocation for a C code would be :

```sh
$ mpc_cc my_mpi_code.c
```

# OPTIONS

Here will be presented options allowed to be passed as argument to the wrapper
command. All native options targeting the underlying compiler can be provided
transparently, any unkown options from MPC's point of view is discretly
forwarded to the final compiler. There is only one collision in option name, for
the **\-\-help**, which is caught by the wrapper to print its own help. To avoid
long output, the help from the underlying compiler is not shown. To display the
native compiler help, please use **\-\-compiler-help** instead. Options are
available for any language-specific wrapper, only for few exceptions, explicitly
mentioned.

**-h**, **\-\-help**

: Print the help / usage for this command.

**\-\-compiler-help**

: Forward the actual help from the underlying compiler to the user

**-v**, **\-\-version**

: Display the MPC version is wrapper is coming from.

**\-\-compilers**

: Display all compilers handled by the current installation. As MPC can rely on
a different compiler than the one used to build the framework, an installation
can handle multiple underlying compiler to build an application. the
**\-\-compilers** option will list all registered compilers for the C language,
in the order there are loaded by the wrapper. The first one listed is picked as
the default. To change/add/remove compilers from this list, one can use the
**mpc\_compiler\_manager** tool.

**\-\-show**

: Echoes every invocation to the underlying compiler instead of actually running
them (equivalent to a dry-run). Note that for MPC, a single wrapper invocation
does *NOT* map to a single compiler invocation. Because some other tasks have to
be performed beforehand, this command will display every invocation leading to
build the final binary. To have a partial view and/ord retrieve only subsets of
the compilafion flags, it could be better to look for the **\-\-showme** option.

**\-\-showme:***token*

: Depending on *token*, displays the arguments supplied to the underlying
compiler. The following values for *token* are accepted:

    - *compile* : displays compilation flags for a translation unit to be built.
    - *link*: displays linking flags for the programm to be assembled.
    - *command*: displays the absolute path to the underlying compiler. This
      last value is equivalent to what **mpc\_compiler\_manager** is able to do.

**\-\-use:***token*

: Depending on *token*, invoke the supplied compiler with only a subset of
arguments. The following values for *token* are accepted:
    - *command*: call the underlying compiler without any implicit arguments.
      User can provide its own argument list.

**-cc=***compiler*
: Force the compilation wrapper to use the designated compiler instead of the
default one. This may be convenient to punctually override the compiler
management, avoiding the pain to change the default compiler back and forth.
Even if the option mention 'cc', it can be used for any language, like C++ and
Fortran.

**-fmpc-privatize**, **-fnompc-privatize**

: The privatization is steered by the use of the flag **-fmpc-privatize**. It
will make the application compatible with a thread-based MPI usage, by
privatizing every global/static variables, changing their scope to have
different copies for each MPI 'process' (becoming threads in a thread-based
context). This flag will enable compilation optimisations, but does not limit
the application to be run in thread-base mode only. the **\-\-fnompc-privatize**
only as an 'explicit' interest. If specified multiples times, only the last
occurence prevails.

**-fmpc-tlsopt**, **-fnompc-tlsopt**
: Because global/variables are translated into multi-level TLS variables
(ExTLS), the overhead is higher. To reduce this overhead, the same methodoly
used for pthread TLS variables is applied to 'ExTLS' variables. Multiple
approaches exist to access a TLS variable, depending the DSO it is coming from,
the number of access in an instruction subset, etc. These approaches can be
split into 2 main categories: by using a function call OR by using a process
register (the 32-bit GS register in that case). As the use of such register
can be an issue in some situations, the **-fnompc-tlsopt** option indicates that
only TLS optimizations through function calls are allowed. **-fmpc-tlsopt** is
implied by default and only used to be explicit.

**-fmpc-plugin**, **-fnompc-plugin** [ __*C only*__ ]
: Global and TLS variables have some similarities but are also quite different.
One ont them is the way they can be initialized before the program starts. FOr
example, such code below is allowed for global/static variable but clearly
forbidden when initializing a TLS variables (ExTLS or not) : Because the value
of *b* cannot be determined statically (as *a* is a TLS too and the value became
dynamic), the C standard fordids the dynamic initialiation of variables before
the call to the main() function -- while it is permitted in C++. As this code
can be valid with global variables, the thread-based approach and the
privatization of global variables cannot be limited by that. MPC provies a
plugin to create such a dynamic initialization when necessary. the
**-fnompc-plugin** explicitly disable such plugin, which might be useful in some
situations. the **-fmpc-plugin** is implied by default for C code only and is
provided for clarity.
```c
int a = 2;
int &b = &a;
```

**-fmpc-include**, **-fnompc-include** [ __*C/C++ only*__ ]
: Force the inclusion of *mpc_main.h* to the binary. This header contains some
macro redefinition to help replacing symbols of the targeted application. The
approach of running MPI main() codes require the catch such function and running
it in threads. The purpose of such header is to rename the `main()` symbol in
`mpc_user_main__()` to avoid the program to be sticked to it (as the main() is
one of the only symbols which cannot be preloaded, for safety reasons).
This is only a C/C++ option, the approach is different in Fortran, based on a
rewrite of the symbol afterwards, through the `objcopy \-\-redefine-sym` tool.

**-fopenmp**, **-fnoopenmp**
: Enable of disable the support of OpenMP pragma directives. As the OpenMP
semantics needs to be enlarged for MPC purposes (threadprivate variables need to
be promoted for instance), is option is caught by the wrapper before being
forwared to the underlying compiler. Furthermore, depending on the vendor (GCC,
Intel) the proper option is set (-fopenmp vs -openmp) without requiring any
modification from the command-line.

**\-\-target=***target*
: Specifies the target the compilation is for. The list of valid target
available for the current installation is available by using the
**\-\-target\_list** option.

**\-\-target-list**
: List currently active MPC target, for cross-compilation only.
