#!/bin/sh

die()
{
	printf "Error: %s\n" "$@" 1>&2
	exit 2
}

warn()
{
	printf "Warning: %s\n" "$@" 1>&2
}

info()
{
	printf "%s\n" "$@" 1>&2
}

check_rc()
{			
	if test "x$1" != "x0" ; then
		exit "$1"
	fi
}

#used to override option w/ compiler specific when option has been provided
# $1: var name
# $2: compiler-specific option
override_var_if_isset()
{
	content="`eval echo "$"${1}`"
	if [ ! -z "${content}" ]
	then
		content="${2}"
	else
		content=""
	fi

	eval "${1}=${content}"
}

CACHED_RES_FOR_PRIV_COMP=""

compiler_is_privatizing()
{
	if test -n "$CACHED_RES_FOR_PRIV_COMP"; then
		RES="${CACHED_RES_FOR_PRIV_COMP}"
	fi

	${1} "--ap-help" > /dev/null 2>&1

	if test "x$?" != "x0"; then
		RES="no"
	else
		RES="yes"
	fi

	CACHED_RES_FOR_PRIV_COMP="${RES}"
}

assert_compiler_is_privatizing()
{
	compiler_is_privatizing "${1}"

	shift

	if test "x${RES}" = "xno"; then
		die "$@"
	fi
}

warn_if_compiler_not_privatizing()
{
	compiler_is_privatizing "${1}"

	shift

	if test "x${RES}" = "xno"; then
		warn "$@"
	fi
}

mpc_disable_header_privarization()
{
	compiler_is_privatizing "${1}"

	if test "x${RES}" = "xyes"; then
		append_to "CFLAGS" "-fno-priv-file=mpc_mpi_comm_lib.h:mpcmicrothread.h:ompt.h"
	fi
}

print_if_stdout()
{
	if test -t 1; then
		printf "%s" "$@" 1>&2
	fi
}

append_to()
{
	val=$(eval "echo \"\$${1}\"")

	if test -n "$val"; then
		val="${val} "
	fi

	eval "${1}=\"${val}${2}\""
}

SHOW_COMPILE="no"
SHOW_ARGS="no"
SHOW_LINK="no"
SHOW_COMMAND="no"

handle_show_args()
{
	if test "x$SHOW_ARGS" = "xyes" ; then 
		if test "x$SHOW_COMPILE" = "xyes"; then 
			echo "$CFLAGS"
		fi 
		if test "x$SHOW_LINK" = "xyes"; then 
			echo "$LDFLAGS"
		fi 
		if test "x$SHOW_COMMAND" = "xyes"; then 
			echo "$COMPILER"
		fi 
	
		exit 0
	fi
}


# Do we have to link eventually?
MPC_DOING_LINK=yes
# Are we using the pre-processor ? default to NO until seeing -E
MPC_DOING_PREPROC=no
# All args concerning the compiling process (w/out mpc_cc-related options)
COMPILATION_ARGS=""
# Output file argument
OUTPUT_FILE_ARGS=""
# Default included MPC header
MPC_HEADER_INCLUDE="-include mpc.h"
# This variable is updated to 'echo' when '-show' is active
MPC_COMMAND_WRAPPER="eval"


PRIV_FLAG_ALREADY_SET=""
# By default priv is enabled in mpc_cc if the compiler supports it
DO_PRIV="yes"

compiler_is_privatizing "$1"

if test "x${RES}" = "xno"; then
	DO_PRIV="no"
fi

mpc_enable_priv()
{
	if test -n "$PRIV_FLAG_ALREADY_SET"; then
		return
	fi

	compiler_is_privatizing "$1"

	if test "x${RES}" = "xyes"; then
		append_to "CFLAGS" "-fmpc-privatize"
	fi

	PRIV_FLAG_ALREADY_SET="yes"
}

IS_FORTRAN="no"
set_fortran()
{
	IS_FORTRAN="yes"
}


show_help()
{
	if "${COMPILER}" "--help" > /dev/null 2>&1; then
		"${COMPILER}" "--help"
	else
		echo "INFO: target compiler \'${COMPILER}' does not seem to handle the --help flag"
	fi

	#And now we add the help for MPC
cat << EOF

################################################################
Options below are specific to $(basename "$0") for use with MPC.
################################################################

Changing compiler:

-cc=[COMP]	      : change the default compiler to COMP

Getting informations from the wrapper:

--showme:link       : show link command to be used
--showme:compile    : show compilation command
--showme:command    : show full command
--use:command       : show bare compiler command
-show               : print full resulting command (no execution)

MPC main rewrite:

-fmpc-include       : include MPC header for main rewriting
-fno-mpc-include    : do not include MPC header for main rewrite
EOF

if compiler_is_privatizing "${COMPILER}"; then
cat << EOF

Privatization:

-fmpc-privatize     : enable global variable privarization (passed by default)
-fno-mpc-privatize  : disable global variable privatization

EOF


fi


	exit 0

}




#
# Parse Command Line arguments and store those for the compiler
#
# Args:
#  - Command line parameters ($@)
#
# Output Args (in env):
#  - OUTPUT_FILE_ARGS: the "-o foo.o" string
#  - MPC_DOING_LINK: yes if the compiler is linking
#  - COMPILER: could be altered by this function
#  - SHOW_ARGS: if mpi_x flags are to be displayed
#  - SHOW_LINK: show link flags before running the command
#  - SHOW_COMPILE: show compilation flags before running the command
#  - SHOW_COMMAND: display the compiler to be used
#  - 
#
parse_cli_args()
{
	OUTPUT_FILE_ARGS=""
	TMP_HANDLE_O_FLAG=no

	for arg in "$@" ; do
		# Set TMP_IS_FOR_COMPILER to no if this arg should be ignored by the C compiler
		TMP_IS_FOR_COMPILER=yes

		if [ $TMP_HANDLE_O_FLAG = yes ] ; then
			TMP_HANDLE_O_FLAG=no
			OUTPUT_FILE_ARGS="-o $arg"
			continue
		fi

		case $arg in
			# ----------------------------------------------------------------
			# Compiler options that affect whether we are linking or not
		-E)
			MPC_DOING_PREPROC=yes
			MPC_DOING_LINK=no
			;;
		-c|-S|-M|-MM)
			# The compiler links by default
			MPC_DOING_LINK=no
			;;

		-cc=*)
			COMPILER=$(echo "A$arg" | sed -e 's/^A-cc=//g')
			COMPILER=$(command -v "${COMPILER}" || echo "${COMPILER}")
			TMP_IS_FOR_COMPILER=no
			;;
		-o)
			TMP_HANDLE_O_FLAG=yes
			TMP_IS_FOR_COMPILER=no
			;;
		--showme:link)
			SHOW_LINK="yes"
			SHOW_ARGS="yes"
			TMP_IS_FOR_COMPILER=no
			;;
		--showme:compile)
			SHOW_COMPILE="yes"
			SHOW_ARGS="yes"
			TMP_IS_FOR_COMPILER=no
			;;
		--showme:command)
			SHOW_COMMAND="yes"
			SHOW_ARGS="yes"
			TMP_IS_FOR_COMPILER=no
			;;
		--use:command)
			USE_COMMAND="1"
			TMP_IS_FOR_COMPILER=no
			;;
		-fmpc-privatize|-fmpcprivatize|-f-mpc-privatize)
			warn_if_compiler_not_privatizing "$COMPILER" "cannot pass $arg to a non-privarizing compiler"
			DO_PRIV="yes"
			TMP_IS_FOR_COMPILER=no
			;;
		-fno-mpc-privatize|-fno-mpcprivatize|-fnompc-privatize|-fnompcprivatize)
			warn_if_compiler_not_privatizing "$COMPILER" "Cannot pass $arg to a non-privarizing compiler"
			DO_PRIV="no"

			TMP_IS_FOR_COMPILER=no
			;;
		-fmpc-include)
			# nothing to do as mpc_main is included by default
			# see mpc_compiler_common.sh
			TMP_IS_FOR_COMPILER=no
			;;
		-fno-mpc-include|-fnompc-include)
			MPC_HEADER_INCLUDE=""
			TMP_IS_FOR_COMPILER=no
			;;
		-compilers|--compilers)
			deprectated_option
			;;
		-cuda)
			append_to "LDFLAGS" "-lmpc_cuda_addon"
			append_to "LDFLAGS" "-Wl,--unresolved-symbols=ignore-in-shared-libs"
			TMP_IS_FOR_COMPILER=no
			;;
		-show)
			TMP_IS_FOR_COMPILER=no
			MPC_COMMAND_WRAPPER="echo"
			;;

			# Verbose mode
		-v)
			# Pass this argument to the pre-compiler/compiler as well.
			echo "mpc_cc for $MPC_VERSION"
			;;

		# Help
		--help)
			show_help
			;;
		esac

		# Update compiler arguments
		if [ $TMP_IS_FOR_COMPILER = yes ] ; then
			# Thanks to Bernd Mohr for the following that handles quotes and spaces
			modarg=`echo "x$arg" | sed -e 's/^x//' -e 's/"/\\\"/g' -e s,\',%@%\',g -e 's/%@%/\\\/g' -e 's/ /\\\ /g' -e 's#(#\\\(#g' -e 's#)#\\\)#g'`
			COMPILATION_ARGS="$COMPILATION_ARGS $modarg"
		fi
	done


	if test "x${DO_PRIV}" = "xyes"; then
		mpc_enable_priv "$COMPILER"
	fi

	mpc_disable_header_privarization "$COMPILER"

	if test "x${IS_FORTRAN}" = "xno"; then
		if test "$MPC_DOING_PREPROC" = "yes"; then 
			MPC_HEADER_INCLUDE=""
		fi

		COMPILATION_ARGS="${COMPILATION_ARGS} ${MPC_HEADER_INCLUDE}"
	fi

	handle_show_args
}

#
# Run the final compiler command
#
# Input Args (from env mostly parse_cli_args):
#  - MPC_COMMAND_WRAPPER: wrapping command 
#  - COMPILER: the command to compile
#  - CFLAGS: compilation flags
#  - LDFLAGS: linker flags
#  - COMPILATION_ARGS: all params for the compiler
#  - OUTPUT_FILE_ARGS: output file (-o foo.o)
run_compiler()
{
	#Only pass link flags when linking
	FINAL_LDFLAGS=""

	if [ "$MPC_DOING_LINK" = yes ] ; then

		if test "$USE_COMMAND" = "1"; then
			LDFLAGS=""
			CFLAGS=""
		fi 

		# Append LDFLAGS to COMPILATION arguments
		FINAL_LDFLAGS="$LDFLAGS" 
	fi

	#shellcheck disable=SC2086
	${MPC_COMMAND_WRAPPER} ${COMPILER} ${CFLAGS} ${COMPILATION_ARGS} ${FINAL_LDFLAGS} ${OUTPUT_FILE_ARGS}
	rc=$?

	exit $rc
}

#
#Fortran Main Renaming Helpers
#


##########################################
# Check if object in $1 contains symbol $2
has_symbol ()
{
	HAS_SYMBOL="no"

	if test -f "${1}"; then
		# nm -P -> Posix formating (start w/ symbol name)
		nb=$(nm -P "${1}" | grep -cE "^${2} ")
		
		if test "x${nb}" = "x1"; then
			HAS_SYMBOL="yes"
		fi
	fi
}

##########################################
# Replace symbol $2 by $3 for the object file $1
rename_sym ()
{
	objcopy --redefine-sym ${2}=${3} "${1}"
}

##########################################
# Replace any main* symbol in $1 object file.
# this includes:
# - the MAIN call
# - any outlined OpenMP rountines, which could be located in MAIN function
rename_main_symbols ()
{
	# rename the MAIN
	rename_sym "${1}" "MAIN__" "mpc_user_main_"
	objcopy --globalize-symbol="mpc_user_main_" "${1}"

	#rename OpenMP outlined routines
	index=0
	has_symbol "${1}" "MAIN__._omp_fn.${index}"

	while test "x${HAS_SYMBOL}" = "x1"
	do
		rename_sym "${1}" "MAIN__._omp_fn.${index}" "mpc_user_main_._omp_fn.${index}"
		index=`expr ${index} + 1`

		has_symbol "${1}" "MAIN__._omp_fn.${index}"

	done

	# Remove the main
	objcopy --strip-symbol main "${1}"
}

##########################################
# Checks if the $1 file ends with $2 (return 0 means "OK it's good")
has_ext ()
{
	case "$1" in
		*$2) return 0;;
		*) return 1;;
	esac
}
