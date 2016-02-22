How-To: Write a fancy documentation for MPC
===========================================

In this document, we want to present the right manner of writing documentation
for MPC and its components. This will ensure an necessary uniformity, ensuing a
better understanding and readability for out-soured user interested by using MPC
and its modules.

MPC will be completely documented through Doxygen comments (under mpcframework/
folder). In addition to in-source documentation, complete and well-writtent, 
each module should provide a Markdown file, named with the module name it
describes. For example, the documentation file for MPC_Common module should be
named "MPC_Common.md". 

DOCUMENTATION FILE STRUCTURE
----------------------------

Below, we show a template of documentation file as it should be used. It is
split into three main sections (detailed after the example):

	MPC : Module <MODULE_NAME>
	==========================

	CONTENTS:
	---------
		* src/     : contains modules source files
			- mpc/ : mpc-specific source files
			- mpi/ : mpi-generic source files
		* include/ : contains include files used as interface w/ other modules
		* extra/   : extra contents for this module

	SUMMARY:
	---------
	Goal of the module is to provide global configuration interface for the
	whole framework.

	COMPONENTS:
	-----------

	### Component #1
	### Component #2
	...


Obviously, MODULE_NAME should be set with actual module name.

### CONTENTS
Detailed as a bullet list, this section allow to briefly describes what it can
be found in current module folder. You can create as much depths as needed in
your description. However, for readability reasons, you should limit depth level
to 2.

### SUMMARY
A short description of the goal of this module. You should not write
more than 50-100 words about it. This section will be useful for real
users to understand what this module stand for.

### COMPONENTS
This section should detail all main components in the current module.  The best
way is to create one subsection per component to explain. For example, in module
MPC_Message_Passing, a component per subfolder should be documented :
inter_thread_comm, low_level_comm, drivers... It is not forbidden (and even
encouraged) to build your own plan, creating as many sub-subsections as needed.
The root section should start with three '#', corresponding to the third level
of header in Markdown. For your own titles, you can prefix them with at least 4
'#' (max-depth: 6)
