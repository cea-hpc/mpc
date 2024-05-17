MPC Documentation structure
============================

General-purpose
-----------------

### Main Documentation

The whole documentation is split into both Doxygen in-code documentation and
global Markdown files, located within each directory it is referring to.
The list of markdown files is as follows :
* `README.md`: in root directory, will just present what MPC means and contains
  without going deep into details
* `GETTING-STARTED.md`: A quick guide to help users to step in to MPC and deploy
  a basic environment to build & run MPI/OpenMP applications in various
  combinations of thread-based/process-based modes.
* `INSTALL.md`: will go deeper to detail all the possibilites of installing the
  framework
* `MAINTAINERS`: contains the whole list of constributors alongside with the
  people to contact in case of bugs/issues/feature-requests.

### Other documentation resources

Now in the `mpcframework` directory, a more specialized documentation can be
found to help new users/developers to interact with MPC modules and how they are
(and should) interact with each others. Each module contains a Markdown file
named after the module name (for instance `MPC_Doc.md` for the MPCi\_Doc module)
and describes directories and main concepts provided at this location. You are
free to open and edit them as necessary to make them as accurate as possible.

Here a list of all Markdown files (to be upated):
- `MPC_MPI.md`
- `MPC_Lowcomm.md`
- `MPC_Tools.md`
- `MPC_OpenMP.md`
- `MPC_Config.md`
- `MPC_Fault_Tolerance.md`
- `MPC_Doc.md`
- `MPC_Allocator.md`
- `MPC_Accelerators.md`
- `MPC_Profiler.md`
- `MPC_Common.md`
- `MPC_Microthreads.md`
- `MPC_Threads.md`
- `MPC_VIrtual_Machines.md`
- `MPC_Debugger.md`

Beyond the markdown, you can find the in-place documentation in most
command-line tools (either through man-pages or `--help` option):
- `mpc_cc`
- `mpc_cxx`
- `mpc_f77`
- `mpc_f90`
- `mpc_f08`
- `mpc_compiler_manager`
- `mpc_status`
- `mpc_print_config`

the `man/` directory contains Markdown-based man-pages, used by `pandoc` at
compile time to build an up-to-date documentation while keeping the task of
documenting easy for developers. The `man_static/` directory contains every
man-pages in 'man-page format', used in the case the tool is not available when
building the documentation. Any man-page in `man/` should have a static version,
already generated into the `main_static/` directory.

How-to build the documentation
------------------------------

One can generate the whole Doxygen-steered documentation from the ./gen_doc
script, to be called from the /doc/ directory, located in the root directory
(*NOT in `mpcframework`)*. The `Doxyfile.in can be altered to fit the needs. By
default, a static-page-based website will be generated and browsable by reaching
`/doc/build/html/index.hmtl` main page.

All the man-pages are built during the `make` phase of MPC. All MPI_* man-pages,
based on the MPI standard documentation, are coming from Open-MPI, an another
MPI implementation developed under the 3-clause BSD license and can bound found
at [https://www.open-mpi.org](https://www.open-mpi.org/).

To make documentation updates easier, **OTHER** man-pages are written in
Markdown (located in this directory) and then generated into man-pages through
the `pandoc` tool. Due to the possible unavailability of such tool, an already
built version of each man-page is also stored to be copied in case `pandoc` is
missing. The date of generation is displayed into the man-pages for more
information. To avoid any confusion, please, be sure to read the up-to-date
version of the documentation.

How-to update the documentation
-------------------------------

* **DO NOT EDIT THE MAN-PAGE FILES DIRECTLY**. If doing so, any modification
  will be lost when static man-pages will be regnerated, later on. Furthermore,
  as long as the user is having the `pandoc` tool in the `$PATH`, these updated
  man-pages won't be copied to be the ones to load. So, please keep you away
  from such a pain and edit only the Markdown associated files under `/`. The
  syntax is clearly easier and more intuitive.

* When the documentation is built, the Makefile iterates over every man-pages
  under `man_static/` directory. It looks for a 'dynamic' version under `man/`
  directory to be run with pandoc. When adding a new man-page, be sure to add
  both files, one in Markdown under `man/manX` and `man_static/manX`, manually
  generated.

* Developers are encouraged to keep their code documented through Doxygen for
  both functions **AND** global/static variables. It will help any further
  maintainer to understand how and why any piece of codes are required to make
  everything work.

* When adding a new option to any command-line program, please keep the help
  updated as well as the man-page. Keep in mind that no one can be aware of such
  functionalitites if it is not publicly exposed and explained.

* The script `install_manpage`, taking the man-page to install as first
  argument, is called by the Makefile during the MPC installation process, to
  deploy the documentation.

* the script `build_manpage` will generate static man-pages from the dynamic
  one, if any. It is executing something similar to : `pandoc -s man/manX/<file>.md -t
  man > man_static/manX/<file>.Xin`

* The man-page name is appended with 'in' suffix, because a pre-processing is
  done when installing the documentation. This is because we did not want to
  make changes on original Open-MPI man-pages to make later updates easier. The
  following macros will be replaced:
    - PACKAGE_DATE: Date of build
    - PACKAGE_NAME: Name of the project (Multi-Processor Computing)
    - PACKAGE_VERSION: The actual version
