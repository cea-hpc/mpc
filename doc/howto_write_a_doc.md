How-To: Write a fancy documentation for MPC
===========================================

In this document, we want to present the right manner of writing documentation
for MPC and its components. This will ensure an necessary uniformity, ensuing a
better understanding and readability for out-soured user interested by using MPC
and its modules.

MPC will be completely documented through Doxygen comments (under mpcframework/
folder). In addition to in-source documentation, complete and well-written,
each module should provide a Markdown file, named with the module name it
describes. For example, the documentation file for MPC_Common module should be
named "MPC_Common.md".

MARKDOWN DOCUMENTATION FILE STRUCTURE
-------------------------------------

Below, we show a template of documentation file as it should be used. It is
split into three main sections (detailed after the example):

	MPC : Module <MODULE_NAME>
	==========================

	SUMMARY:
	---------
	Goal of the module is to provide global configuration interface for the
	whole framework.

	CONTENTS:
	---------
		* src/     : contains modules source files
			- mpc/ : mpc-specific source files
			- mpi/ : mpi-generic source files
		* include/ : contains include files used as interface w/ other modules
		* extra/   : extra contents for this module

	COMPONENTS:
	-----------

	### Component #1
	### Component #2
	...


As seen above, there should be three sections. Obviously, MODULE_NAME should be
set with actual module name.

### SUMMARY ###
A short description of the goal of this module. You should not write
more than 50-100 words about it. This section will be useful for real
users to understand what this module stand for.

### CONTENTS ###
Detailed as a bullet list, this section allow to briefly describes what it can
be found in current module folder. You can create as much depths as needed in
your description. However, for readability reasons, you should limit depth level
to 2.

### COMPONENTS ###
This section should detail all main components in the current module.  The best
way is to create one subsection per component to explain. For example, in module
MPC_Message_Passing, a component per sub-folder should be documented :
inter_thread_comm, low_level_comm, drivers... It is not forbidden (and even
encouraged) to build your own plan, creating as many sub-subsections as needed.
The root section should start with three '#', corresponding to the third level
of header in Markdown. For your own titles, you can prefix them with at least 4
'#' (max-depth: 6)


BASIC MARKDOWN SYNTAX & CONVENTIONS
-----------------------------------

We want to remind here basic principles of Markdown syntax and some conventions
which should be used in MPC Documentation.

### Headers ###
Headers are built with a various number of successive '#' w/o any space between
them. Here the list of headers handled by most of parsers and Doxygen:

	# HEADER 1
	## HEADER 2
	### HEADER 3
	#### HEADER 4
	##### HEADER 5
	###### HEADER 6

Header 1 can be replaced by underlining the text with '=' (at least 3) and
header 2 can be replaced by underlining the text with '-' (at least 3).
Generally, header 1 and 2 should be used by major sections inside main file and
should not be used by your written documentation. However you can use as many
header sub-levels as you want, without any restrictions.

Tip: In order to make it well-balanced, you can surround your header with
'#' at the beginning and the end of the title.

### Code ###
Verbatim section are really easy to deploy. You just have to insert a tab (or 4
spaces) + one blank line before and after your code. Here an example:

	int main(void)
	{
		return 0;
	}

You can also use fenced code blocks (ie surrounding lines of '`'=backquotes,
suffixed by code language). Doxygen requires '~' (tilde) character and language
surrounded by braces '{}'. Here an example :

	```c
	int main(void)
	{
		return 0;
	}
    ```c

will produces:

```c
int main(void)
{
	return 0;
}
```

You can also use span code blocks, inlining code section with single back-quotes
around it list `this`.

### Links ###
With Markdown, you can use 3 way to add a link:

	1. [http://link]
	2. [I'm a link](http://link "link title") (the title is optional)
	3. [I'm a link][LINK]
       [LINK]: http://link (generally at the bottom of document)
	4. [I'm a link][1]
	   [1]: http://link (create an endnote)

### Conclusion ###
Markdown provides a lot of features not presented here. You can add what you
want in your documentation, the main goal being to stay uniform. Thus, if a
standard seems useful to you, you can add it to this documentation.

BASIC DOXYGEN SYNTAX & CONVENTIONS
----------------------------------

Doxygen provides a way to write comments, helping to understanding code around
it and can be easily parsed in order to produce a completely formatted output
(like a website or pdf document). In this section, we will try to present basic
usage of Doxygen and some rules that should be followed by documentation writers
in order to keep uniformity over the whole project.

### Create a comment ###
In C language a multi-line Doxygen comment is generally prepended to the content to
document and start with /** or /\*! and end with \*/. See below a short example:

```c
/**
 * This is a standard multi-lines Doxygen comment
 \*/
 int func(void);
```

If you want to use single-line comment, it can be done through C++-style
comments. Doxyen comment starts with /// or //!

```cpp
/// this is a C++-style comment
int func(void);
```

If you want to post-pend your comment instead, you can use the '<' character
just after Doxygen-specific sequence in the comment :

```cpp
int a;
/*!< This will be the doc for the variable above 'a' */
int b; //!< This will the doc for the variable to the left 'b'
```

### Fill a comment ###
For each action, a special command exists in Doxygen. These command are callable
directly within a comment and are prefixed with a '@' or a '\'. For example, if you write
the documentation for a function, you can explicitly detail an argument meaning.
For that, you can use `@param` or `\param` and the argument name to specifically
provide a doc for this one. If you really want to be rigorous, you can add
'[in]', '[out]' or '[inout]' to specify for what the argument stands for. Here
the list of the most used special command in MPC documentation:

* `\brief` : The next phrase will be used as a brief description of the comment.
  This should be used for each comment. The details of the comment is generally
  split from the brief with an empty line.
* `\param[in|out|inout] <argname> <doc>` : add a specific information about the
  arg.
* `\return` : define the type of return. Generally it is convenient to make a list
  (w/ hyphens, `\li` or HTML-style syntax) to list all possible return values.
* `\see <ref>`: If you want to point to a specific documentation elsewhere, you can use
  this command to create a link to the remote ref.

Generally, Doxygen is able to detect which kind of object the comment refers. If
you want explicitly write the object the comment is attached, you can use
special command associated to the type of object :
* `\def <name>` : comment for a #define
* `\enum <name>` : comment for an enum
* `\struct <name>` : comment for a struct
* ...
This can be useful when the comment is not directly followed by the object. For
example, you can add all the documentation on top of file (not recommended if
you want to follow good practices...).

To highlighting some part of your comment, Doxygen provide 4 ways to attract
user's eyes:
* `\warning`
* `\todo`
* `deprecated`
* `\fixme`

Each function prototype and/or definition should be documented. Prototype
documentation explains materials needed by the user to use it, while function
definition documentation details how the function works.

Finally, each file should start w/ a Doxygen comment and `\file` special command
to briefly explain what the file contains (then used by Doxygen as file summary)

### Special usage ###
* `\code ... \endcode` : insert code in Doxygen comment
* `\verbatim ... \endverbatim` : insert raw text
* `\dot...\enddot` : Insert dot graph block generated by Doxygen

### Conclusion ###
Doxygen provides a lot a other features not presented here and you are invited
to add them in this documentation if useful.
To generate a PDF version of this file, you can use Pandoc, in the following
way:

```sh
$ pandoc ./howto_write_a_doc.md -o howto_write_a_doc.pdf --latex-engine=pdflatex
```
