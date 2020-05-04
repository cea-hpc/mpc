############################# MPC License ##############################
# Wed Nov 19 15:19:19 CET 2008                                         #
# Copyright or (C) or Copr. Commissariat a l'Energie Atomique          #
#                                                                      #
# IDDN.FR.001.230040.000.S.P.2007.000.10000                            #
# This file is part of the MPC Runtime.                                #
#                                                                      #
# This software is governed by the CeCILL-C license under French law   #
# and abiding by the rules of distribution of free software.  You can  #
# use, modify and/ or redistribute the software under the terms of     #
# the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     #
# following URL http://www.cecill.info.                                #
#                                                                      #
# The fact that you are presently reading this means that you have     #
# had knowledge of the CeCILL-C license and that you accept its        #
# terms.                                                               #
#                                                                      #
# Authors:                                                             #
#   - CARRIBAULT Patrick patrick.carribault@cea.fr                     #
#   - PERACHE Marc marc.perache@cea.fr                                 #
#   - JAEGER Julien julien.jaeger@cea.fr                               #
#                                                                      #
########################################################################


# Global AC setup, no update required here
AC_INIT([Multi-Processor Computing Framework],
	[m4_esyscmd_s([./utils/get_version])],
	[france@paratools.com], [mpcframework], [https://mpc.hpcframework.com])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIRS([config/m4])
AC_CONFIG_SRCDIR([src/MPC_Arch/src/timer.c])
AC_LANG(C)

# Pre-required maintainer tool to rebuild.
# Enforcing this ensure mimnimum changes to
# generated files under version control
AC_PREREQ(2.69)
LT_PREREQ([2.4.6])
AM_INIT_AUTOMAKE([1.15 subdir-objects foreign no-define])
AM_SILENT_RULES([yes])

# DO NOT REBUILD Makefile.in & CO
# IT MEANS YOU NEED TO REBUILD MANUALLY
# EACH TIME YOU EDIT THE BUILD SYSTEM !
# USE ./autogen.sh to do so
AM_MAINTAINER_MODE([disable])

#
# Reload first configure environment if present
# this is needed as we want all the variables
# set by the root configure script (or first configure)
# to be present as we rerun this configure
#
MPC_CONFIG_RELOADED="no"

if test -f ${PWD}/bin/mpc_build_env.sh; then
	MPC_CONFIG_RELOADED="yes"
	. ${PWD}/bin/mpc_build_env.sh
fi

# General Configuration
CONFIGURE_SCRIPT_PATH=$(dirname "`readlink -f "$0"`")

LT_INIT([disable-static])
LT_LANG([C])
AC_PROG_CXX([apc++ icpc g++ clang++])
AC_PROG_CC([apcc icc gcc clang])
AC_PROG_FC([apfortran ifort gfortran])

MPC_VERSION_MAJOR="$($CONFIGURE_SCRIPT_PATH/utils/get_version --major)"
MPC_VERSION_MINOR="$($CONFIGURE_SCRIPT_PATH/utils/get_version --minor)"
MPC_VERSION_PATCH="$($CONFIGURE_SCRIPT_PATH/utils/get_version --patch)"
MPC_VERSION_TAINT="$($CONFIGURE_SCRIPT_PATH/utils/get_version --taint)"
MPC_VERSION_STRING="$($CONFIGURE_SCRIPT_PATH/utils/get_version --all)"


AC_DEFINE_UNQUOTED([MPC_VERSION_MAJOR], [$MPC_VERSION_MAJOR], [MPC Major Version])
AC_DEFINE_UNQUOTED([MPC_VERSION_MINOR], [$MPC_VERSION_MINOR], [MPC Minor Version])
AC_DEFINE_UNQUOTED([MPC_VERSION_PATCH], [$MPC_VERSION_PATCH], [MPC Patch Version])
AC_DEFINE_UNQUOTED([MPC_VERSION_PRE], ["$MPC_VERSION_TAINT"], [MPC Tainted Version])
AC_DEFINE_UNQUOTED([MPC_VERSION_STRING], ["$MPC_VERSION_STRING"], [MPC Tainted Version])

AC_SUBST([MPC_VERSION_MAJOR])
AC_SUBST([MPC_VERSION_MINOR])
AC_SUBST([MPC_VERSION_PATCH])
AC_SUBST([MPC_VERSION_PRE])
AC_SUBST([MPC_VERSION_STRING])

AC_DEFINE_UNQUOTED([MPC_PREFIX_PATH], ["$prefix"], [Prefix Path for MPC])


MPC_SUPPORTED_ARCHITECTURES_LIST="x86_64 ia64 i686 arm aarch64 powerpc64"

# Resolve Host CPU

AC_CANONICAL_HOST
MPC_TARGET_ARCHITECTURE="$host_cpu"

# Check if architecture is in support list

AC_MSG_CHECKING([Support for target architecture])

is_in_list()
{
    echo "$2" | grep -iq "$1"
    return $?
}

if is_in_list "$MPC_TARGET_ARCHITECTURE" "$MPC_SUPPORTED_ARCHITECTURES_LIST"; then
        AC_MSG_RESULT([$MPC_TARGET_ARCHITECTURE])
else
        AC_MSG_RESULT(Not supported)
        AC_MSG_ERROR(The current architecture is not supported yet. Please contact support team.)
fi

AC_SUBST([MPC_TARGET_ARCHITECTURE])



# Programs Checks
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_CC
AM_PROG_AS

# Optionnal XML generators

AC_CHECK_PROG([xsltproc],[xsltproc],[yes],[no])
AC_CHECK_PROG([xmllint],[xmllint],[yes],[no])

AM_CONDITIONAL([CAN_GENERATE_CONFIG], [test "x$xsltproc" = xyes -o "x$xmllint" = xyes])

# Optionnal Pandoc Generator

AC_CHECK_PROG([pandoc],[pandoc],[yes],[no])

AM_CONDITIONAL([CAN_USE_PANDOC], [test "x$pandoc" = xyes])

# Checks for libraries.

# Library detector helper


prefix_candidate_path="/usr /usr/local /opt"
prefix_candidate_sub_include_path=". include include/slurm include/slurm-wml"

#
# $1 header name
# $2 provided prefix
#
# Sets check_or_infer_prefix_include or returns ""
#
__check_or_infer_prefix_include()
{
        check_or_infer_prefix_include="no"
        looked_up_header=$1
        provided_prefix=$2
        var_prefix=$3

	AC_CHECK_HEADER([$looked_up_header],[hfound=yes],[hfound=no])

	if test "x$hfound" = "xyes"; then
		check_or_infer_prefix_include=yes
		AC_MSG_RESULT([System-wide])
		return
	fi

	AC_MSG_NOTICE([Checking for $1 header manually in different places])
        for pref in $provided_prefix $prefix_candidate_path
        do
                for suffix in $prefix_candidate_sub_include_path
                do
                        #AC_MSG_CHECKING([for '$looked_up_header' in '$pref/$suffix'])
                        if test -f "$pref/$suffix/$looked_up_header"
                        then
                                #AC_MSG_RESULT([yes])
                                # Set include path for matched header
                                CPPFLAGS+=" -I$pref/$suffix/"

                                AX_RESET_HEADERS_CACHE([$looked_up_header])
                                AC_CHECK_HEADER([$looked_up_header],[],[AC_MSG_ERROR([Failed to autodetect '$looked_up_header' in '$pref/$suffix'])])
                                check_or_infer_prefix_include=yes
                                AC_MSG_NOTICE(['$1' found in $pref/$suffix])
                                return
                        fi
                done
        done
}

prefix_candidate_sub_lib_path=". lib lib64"

#
# libname
#
___unset_lib_cache()
{
        cache_vars=$(set | cut -d "=" -f 1 | grep ac_cv_lib | sed "s/'//g")
        for i in $cache_vars
        do
                AS_UNSET([$i])
        done
}


#
# $1 lib name
# $2 lib function name
# $3 provided prefix
#
# Sets check_or_infer_prefix_include or returns ""
#
__check_or_infer_prefix_lib()
{
        check_or_infer_prefix_lib="no"
        looked_up_lib=$1
        looked_up_function=$2
        provided_prefix=$3
        var_prefix=$4

	AC_CHECK_LIB($looked_up_lib, $looked_up_function, [lib_found=1], [lib_found=0])

	if test "x$lib_found" = "x1"
	then
		LDFLAGS=" -l$1 $LDFLAGS"
		check_or_infer_prefix_lib=yes
		return
	fi

	AC_MSG_NOTICE([Checking for '$looked_up_lib' library in different places])
        for pref in $provided_prefix $prefix_candidate_path
        do
                for suffix in $prefix_candidate_sub_lib_path
                do
                        lib_found=0


                        OLD_LDFLAGS=$LDFLAGS

                        LDFLAGS=" -L$pref/$suffix/ -Wl,-rpath=$pref/$suffix/ -l$1 $LDFLAGS"

                        ___unset_lib_cache
                        AC_CHECK_LIB($looked_up_lib, $looked_up_function, [lib_found=1], [lib_found=0])

                        if test "x$lib_found" = "x1"
                        then
                        	AC_MSG_NOTICE(['$looked_up_lib' found in $pref/$suffix])
                                check_or_infer_prefix_lib=yes
                                return
                        fi

                        LDFLAGS=$OLD_LDFLAGS

                done
        done
}


# Get the value from a define
# $1 Header to inspect
# $2 Define to extract
# $3 can fail
#
extract_integer_define_value()
{
	can_fail="$3"

	if test -z "$can_fail"; then
		can_fail="yes"
	fi

	AC_MSG_CHECKING([$2 existence])

	MACRO_VALUE=""
	AC_RUN_IFELSE(
	[AC_LANG_PROGRAM([#include <$1>
			#include <stdio.h>],
			[
				long long int val = $2;
				FILE *out = fopen("./conftest.out", "w");

				if(!out)
					return 1;

				fprintf(out, "%lld\n", val);

				fclose(out);
			])],
	[MACRO_VALUE="`cat conftest.out`"],
	[MACRO_VALUE=""])

	if test -z "$MACRO_VALUE"; then
		if test "x$can_fail" = "xyes"; then
			AC_MSG_FAILURE([Failed to lookup for $2 in $1])
		else
			AC_MSG_RESULT([not found])
		fi

	else
		AC_MSG_RESULT([$MACRO_VALUE ($1)])
	fi
}


#
# $1 header_name (ex. hwloc.h)
# $2 libname     (ex. hwloc)
# $3 looked up function (ex. hwloc_topology_init)
# $4 LIBCODE (ex XML, HWLOC, ...)
# $5 optionnal library version (need pkg-config)
# $6 optionnal lib_prefix
#
AC_DEFUN([MPC_DETECT_DEPENDENCY],
         [
                AS_VAR_PUSHDEF([dep_found], [mpc_dep_found])
                AC_ARG_WITH([$2],
                [AS_HELP_STRING([--with-$2],
                                [Provide prefix for $2])],
                [whole_prefix=$withval],[whole_prefix="/usr/"])

                inc_prefix=""
                AC_ARG_WITH([$2-include],
                [AS_HELP_STRING([--with-$2-inc],
                                [Provide include path for $2])],
                [inc_prefix=$withval],[])
                lib_prefix=""
                AC_ARG_WITH([$2-libs],
                [AS_HELP_STRING([--with-$2-libs],
                                [Provide library path for $2])],
                [lib_prefix=$withval],[lib_prefix="$6"])

                check_or_infer_prefix_result="no"
                header_name=$1
                libname=$2
                function_name=$3
                var_prefix=$4
                version="$5"

                if test -z "$lib_prefix"
                then
                        lib_prefix=$whole_prefix
                fi

                if test -z "$inc_prefix"
                then
                        inc_prefix=$whole_prefix
                fi

                AC_SUBST([$4_CFLAGS])
                AC_SUBST([$4_LIBS])

                #echo "PREF : $whole_prefix LIB: $lib_prefix INC: $inc_prefix"

                # First try with PKG config
                PKG_CHECK_MODULES([$4], [$libname $version], [pkg_found=1], [pkg_found=0])

                if test "x$pkg_found" = "x1"
                then
                        #Insert flags
                        CPPFLAGS="${$4_CFLAGS} $CPPFLAGS"
                        LDFLAGS="${$4_LIBS} $LDFLAGS"

                        AC_SUBST([$4_CFLAGS])
                        AC_SUBST([$4_LIBS])

                        #AC_MSG_NOTICE([$libname found using pkg-config])
                        #AC_CHECK_LIB($libname,$function_name,[], [AC_MSG_ERROR([Could not locate '$function_name' in '$libname'])])
                        check_or_infer_prefix_result="yes"
                else
                        __check_or_infer_prefix_include "$header_name" "$inc_prefix" "$var_prefix"
                        __check_or_infer_prefix_lib "$libname" "$function_name" "$lib_prefix" "$var_prefix"

                        if test "x$check_or_infer_prefix_include" = "xyes" -a "x$check_or_infer_prefix_lib" = "xyes"
                        then
                                check_or_infer_prefix_result="yes"
                        else
                                check_or_infer_prefix_result="no"
                        fi
                fi

                AS_VAR_SET([dep_found], [$check_or_infer_prefix_result])
         ])


AC_DEFUN([MPC_REQUIRED_DEPENDENCY],
         [
                if test "x$2" = "xno"
                then
                        AC_MSG_WARN([consider using --with-$1=PREFIX])
                        AC_MSG_WARN([consider using --with-$1-libs=LIBS])
                        AC_MSG_WARN([consider using --with-$1-include=INCS])
                        AC_MSG_ERROR([Failed to locate required dependency $1])
                fi
         ])

MPC_DETECT_DEPENDENCY([libxml/tree.h],[libxml-2.0], [xmlStrcmp], [XML2])
MPC_REQUIRED_DEPENDENCY([libxml-2.0], [$mpc_dep_found])

MPC_DETECT_DEPENDENCY([hwloc.h],[hwloc], [hwloc_topology_init], [HWLOC], [< 2.0.0])
MPC_REQUIRED_DEPENDENCY([hwloc], [$mpc_dep_found])

MPC_DETECT_DEPENDENCY([opa.h],[openpa], [OPA_load_int], [OPA])
MPC_REQUIRED_DEPENDENCY([openpa], [$mpc_dep_found])


#
# Detect MPC Allocator
#

mpc_allocator="Standard"

MPC_DETECT_DEPENDENCY([mpcalloc.h], [mpcalloc], [sctk_malloc], [ALLOC])
if test "x$mpc_dep_found" = "xyes"; then
        AC_DEFINE([MPC_Allocator], [1], [mpcalloc support is enabled])
        mpc_allocator="mpcalloc"
fi

#
# Detect libunwind
#
AC_MSG_CHECKING([support for libunwind library])
mpc_libunwind="no"

MPC_DETECT_DEPENDENCY([libunwind.h], [unwind], [unw_init_local], [UNW])

#AC_MSG_CHECKING([support for libunwind library])
if test "x$mpc_dep_found" = "xyes"; then
        AC_DEFINE([MPC_HAVE_LIBUNWIND], [1], [libunwind support is enabled])
        mpc_libunwind="yes"
fi

#
# Network list
#

MPC_NETWORK_LIST="tcp shm"



#
# Detect libibverbs presence
#

infiniband_enabled="no"

MPC_DETECT_DEPENDENCY([infiniband/verbs.h], [ibverbs], [ibv_query_qp], [IBV])
if test "x$mpc_dep_found" = "xyes"; then
        MPC_NETWORK_LIST="$MPC_NETWORK_LIST verbs"
        AC_DEFINE([MPC_USE_INFINIBAND], [1], [Infiniband support is enabled])
        infiniband_enabled="yes"
fi

AM_CONDITIONAL([INFINIBAND_DRIVER_ENABLED], [test "x$infiniband_enabled" = "xyes"])

#
# Detect Portals4
#

portals4_enabled="no"

MPC_DETECT_DEPENDENCY([portals4.h], [portals], [PtlGet], [PORTALS])
if test "x$mpc_dep_found" = "xyes"; then
        MPC_NETWORK_LIST="$MPC_NETWORK_LIST portals4 portals4_am"
        AC_DEFINE([MPC_USE_PORTALS], [1], [Portals support is enabled])
        portals4_enabled="yes"
fi

AM_CONDITIONAL([PORTALS4_DRIVER_ENABLED], [test "x$portals4_enabled" = "xyes"])

AC_SUBST([MPC_NETWORK_LIST])

#
# Configure MPC Launch
#

mpc_hydra_enabled=yes

AC_ARG_WITH([hydra],
[AS_HELP_STRING([--with-hydra],
                [Compile MPC with hydra support (requires an external hydra launcher)])],
[mpc_hydra_enabled=$withval],
[mpc_hydra_enabled=yes])

AC_ARG_WITH([slurm],
[AS_HELP_STRING([--with-slurm],
                [Rely on SLURM-based PMI])],
[mpc_hydra_enabled=no; mpc_slurm_enabled=$withval],
[mpc_slurm_enabled=no])

if test "x$mpc_hydra_enabled" = "xyes" -a  "x$mpc_slurm_enabled" = "xyes"
then
        AC_MSG_ERROR([You must pick up one between Hydra and Slurm (--with-hydra or --with-slurm)])
fi

if test "x$mpc_hydra_enabled" = "xyes" -o  "x$mpc_slurm_enabled" = "xyes"
then
        #All good
        dummy=1337
else
        AC_MSG_ERROR([You must activate either Hydra and Slurm (--with-hydra or --with-slurm)])
fi

MPC_MPIEXEC_HYDRA_PATH="mpiexec.hydra"

if test "x$mpc_slurm_enabled" = "xyes"; then

        MPC_LAUNCHER="SLURM"
        MPC_DETECT_DEPENDENCY([pmi.h],[pmi], [PMI_Finalize], [PMI])
        MPC_REQUIRED_DEPENDENCY([pmi], [$mpc_dep_found])

        AC_DEFINE([MPC_USE_SLURM], [1], [Slurm PMI will be used to launch parallel programs])
else
        AC_CHECK_PROG([HAVE_MPIEXEC], [mpiexec.hydra], [yes], [no])

        if test "x$HAVE_MPIEXEC" = "xno"
        then
                AC_MSG_ERROR([Could not locate Hydra wrapper 'mpiexec.hydra' in PATH])
        fi

        MPC_MPIEXEC_HYDRA_PATH=$(which mpiexec.hydra)

        # Here we look for hydra only if hydra has not been disabled
        MPC_LAUNCHER="HYDRA"

        AC_CHECK_FUNCS([socket setsockopt gethostbyname],[can_access_hydra_pmi_socket=no], [can_access_hydra_pmi_socket=yes])

        if test "x$can_access_hydra_pmi_socket" = "xno" ; then
                AC_DEFINE(USE_PMI_PORT,1,[HYDRA supports sockets])
        else
                AC_MSG_ERROR([Hydra requires socket support])
        fi

        AC_DEFINE([MPC_USE_HYDRA], [1], [Hydra will be used to launch parallel programs])
fi

AM_CONDITIONAL([HYDRA_CLIENT_LIB_ENABED], [test "x$MPC_LAUNCHER" = "xHYDRA"])

AC_SUBST([MPC_MPIEXEC_HYDRA_PATH])
AC_SUBST([MPC_LAUNCHER])

#
# Cuda Detection
#

AC_ARG_WITH([cuda],
[AS_HELP_STRING([--with-cuda],
                [Enable CUDA context switching])],
[mpc_cuda_ctx_switch=$withval],
[mpc_cuda_ctx_switch=no])

cuda_enabled="no"

if test "x$mpc_cuda_ctx_switch" = "xyes"; then

	MPC_DETECT_DEPENDENCY([cuda.h], [cuda], [cuMemFree], [CUDA])
	MPC_REQUIRED_DEPENDENCY([cuda], [$mpc_dep_found])

	if test "x$mpc_dep_found" = "xyes"; then
		MPC_DETECT_DEPENDENCY([cuda_runtime.h], [cudart], [cudaDeviceSetLimit], [CUDART])
		MPC_REQUIRED_DEPENDENCY([cudart], [$mpc_dep_found])

		AC_DEFINE([MPC_USE_CUDA], [1], [Cuda support is enabled])
		cuda_enabled="yes"
	fi
fi

AM_CONDITIONAL([MPC_CUDA_ENABLED], [test "x$cuda_enabled" = "xyes"])

#
# DMTCP detection
#

AC_ARG_WITH([dmtcp],
[AS_HELP_STRING([--with-dmtcp],
                [Compile MPC with checkpoint restart support])],
[MPC_HAVE_DMTCP=$withval],
[MPC_HAVE_DMTCP=no])



if test "x$MPC_HAVE_DMTCP" = "xyes"; then


AC_CHECK_PROG([dmtcp_launch],[dmtcp_launch],[yes],[no])

	AC_MSG_CHECKING([support for DMTCP])

	if test "x${dmtcp_launch}" = "xyes"; then
		AC_MSG_RESULT([yes])
		MPC_HAVE_DMTCP="yes"
		AC_DEFINE([MPC_USE_DMTCP], [1], [MPC transparent checkpointing is enabled])
	else
		AC_MSG_RESULT([no])
		AC_MSG_ERROR([Cannot enable DMTCP (no 'dmtcp_launch' command found)])
	fi
fi

AC_SUBST([MPC_HAVE_DMTCP])

# Checks for header files.

AC_HEADER_STDC

AC_CHECK_HEADERS([aio.h])
AC_CHECK_HEADERS([asm/unistd.h])
AC_CHECK_HEADERS([assert.h])
AC_CHECK_HEADERS([ctype.h])
AC_CHECK_HEADERS([errno.h])
AC_CHECK_HEADERS([execinfo.h])
AC_CHECK_HEADERS([elf.h])
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([getopt.h])
AC_CHECK_HEADERS([hwloc.h])
AC_CHECK_HEADERS([immintrin.h])
AC_CHECK_HEADERS([libxml/parser.h])
AC_CHECK_HEADERS([libxml/tree.h])
AC_CHECK_HEADERS([libxml/xmlschemas.h])
AC_CHECK_HEADERS([limits.h])
AC_CHECK_HEADERS([linux/personality.h])
AC_CHECK_HEADERS([netdb.h])
AC_CHECK_HEADERS([netinet/in.h])
AC_CHECK_HEADERS([netinet/tcp.h])
AC_CHECK_HEADERS([regex.h])
AC_CHECK_HEADERS([sched.h])
AC_CHECK_HEADERS([setjmp.h])
AC_CHECK_HEADERS([signal.h])
AC_CHECK_HEADERS([stddef.h])
AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([stdint.h])
AC_CHECK_HEADERS([stdbool.h])
AC_CHECK_HEADERS([string.h])
AC_CHECK_HEADERS([sys/mman.h])
AC_CHECK_HEADERS([sys/param.h])
AC_CHECK_HEADERS([sys/pstat.h])
AC_CHECK_HEADERS([sys/socket.h])
AC_CHECK_HEADERS([sys/stat.h])
AC_CHECK_HEADERS([sys/syscall.h])
AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_HEADERS([sys/types.h])
AC_CHECK_HEADERS([sys/ucontext.h])
AC_CHECK_HEADERS([sys/un.h])
AC_CHECK_HEADERS([sys/utsname.h])
AC_CHECK_HEADERS([sys/wait.h])
AC_CHECK_HEADERS([time.h])
AC_CHECK_HEADERS([ucontext.h])
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([wchar.h])

# Checks for optionnal headers

#TODO CHECK IB
#TODO CHECK WHY libunwind.h
#TODO CHECK PMI
#TODO CHECK STDBOOL
#TODO CHECK VALGRIND

AC_CHECK_HEADERS([valgrind/memcheck.h])
AC_CHECK_HEADERS([valgrind/valgrind.h])
AC_CHECK_HEADER_STDBOOL
AC_CHECK_HEADERS([libunwind.h])
AC_CHECK_HEADERS([infiniband/verbs.h])


AC_CHECK_HEADERS([semaphore.h],[HAVE_SEMAPHORE_H=1],[HAVE_SEMAPHORE_H=0])
AC_SUBST([HAVE_SEMAPHORE_H])

AC_CHECK_HEADERS([pthread.h], [HAVE_PTHREAD_H=1], [HAVE_PTHREAD_H=0])
AC_SUBST([HAVE_PTHREAD_H])

# Checks for typedefs, structures, and compiler characteristics.

AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_OFF_T
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for Macros

AC_MSG_CHECKING([presence of '__func__' variable])

# Alternative : AX_C_VAR_FUNC
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]],
                                  [[return __func__;]])],
                res_macro=yes,res_macro=no)

if test "$res_macro" = "yes" ; then 
   MPC_ARCH_DETECTED_FUNCTION="__func__"
   AC_MSG_RESULT(yes)
   AC_SUBST(MPC_ARCH_DETECTED_FUNCTION)
else
   AC_MSG_RESULT(no)
   AC_MSG_ERROR([Unable to detect function name])
fi

AC_MSG_CHECKING([presence of __STRING macro])
# Alternative : insert #define __STRING(u) #u if not present (not portable macro)
AC_PREPROC_IFELSE([AC_LANG_SOURCE([[#include <stdio.h>
				  #ifndef __STRING
				  #error "No __STRING macro defined !"
				  #endif]])],
                res_macro=yes,res_macro=no)

if test "$res_macro" = "yes" ; then 
  MPC_ARCH_DETECTED_STRING="__STRING"
   AC_MSG_RESULT(yes)
   AC_SUBST(MPC_ARCH_DETECTED_STRING)
else
   AC_MSG_RESULT(no)
   AC_MSG_ERROR([Missing '__STRING' macro])
fi

# Checking for ENVIRON variable


AC_MSG_CHECKING([presence of 'environ' global variable])

AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <stdlib.h>
                                 extern char ** environ;]],
                                [[return environ;]])],
                                res_macro=yes,res_macro=no)

if test "$res_macro" = "yes" ; then 
   AC_MSG_RESULT(yes)
   AC_DEFINE([HAVE_ENVIRON_VAR],[1],[The environ variable is present])
else
   AC_MSG_RESULT(no)
fi

# Check for pragma message support

AC_MSG_CHECKING([pragma message support])

OLD_CFLAGS="$CFLAGS"
#Set WERROR to catch the ignored pragmas
CFLAGS="-Werror -Wall"

AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>]],
                                [[#pragma message "Hello There"]])],
                                res_macro=yes,res_macro=no)

if test "$res_macro" = "yes" ; then
   AC_MSG_RESULT(yes)
   AC_DEFINE([HAVE_PRAGMA_MESSAGE],[1],[Message pragmas are supported by the compiler])
else
   AC_MSG_RESULT(no)
fi

#Restore old CFLAGS after pragma test
CFLAGS="$OLD_CFLAGS"


# Checking for builtin

AX_GCC_BUILTIN([__builtin_expect])

AC_C_CONST
AC_C_RESTRICT
AC_C_VOLATILE
AC_C_INLINE

AX_TLS([AC_DEFINE([TLS_SUPPORT], [1], [TLS extension supported by compiler])],
       [AC_DEFINE([TLS_SUPPORT], [0], [TLS extension NOT supported by compiler])])


# Checks for type sizes

AC_CHECK_SIZEOF(char)
AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(float)

AC_CHECK_SIZEOF(long)
AC_SUBST([SIZEOF_LONG], [$ac_cv_sizeof_long])

AC_CHECK_SIZEOF(double)
AC_CHECK_SIZEOF(size_t)
AC_CHECK_SIZEOF(void*)
AC_CHECK_SIZEOF(long long)

# Checks for standard library functions.

AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memcpy memmove memset statfs])
AC_CHECK_FUNCS([strcasecmp strchr strdup strerror strstr strtol snprintf vsnprintf])
AC_CHECK_FUNCS([gethostbyname gethostname])
AC_CHECK_FUNCS([gettimeofday])
AC_CHECK_FUNCS([mmap munmap])
AC_CHECK_FUNCS([setenv getenv])
AC_CHECK_FUNCS([uname])
AC_CHECK_FUNCS([regcomp])
AC_CHECK_FUNCS([socket connect accept])
AC_CHECK_FUNCS([dup2 fdatasync])
AC_CHECK_FUNCS([makecontext setcontext getcontext swapcontext])

# Check for libmath
AC_CHECK_LIB([m],[log])

# Check for pThread
AC_CHECK_HEADER([pthread.h])
AC_CHECK_LIB(pthread,pthread_create,LIBS="$LIBS -lpthread")
AC_CHECK_SIZEOF(pthread_t,,[#include <pthread.h>])
AC_CHECK_SIZEOF(sigset_t,,[#include <signal.h>])

extract_integer_define_value "signal.h" "NSIG" "no"

if test -z "$MACRO_VALUE"; then
	extract_integer_define_value "signal.h" "_NSIG" "no"
	if test -z "$MACRO_VALUE"; then
		MACRO_VALUE="32"
	fi
fi

AC_DEFINE_UNQUOTED([MPC_CONFIG_NSIG], [$MACRO_VALUE], [Value of NSIG from signal.h])
AC_SUBST([MPC_CONFIG_NSIG], [$MACRO_VALUE])

# All that is in pthread.h

#PTHREAD_MUTEX_NORMAL

extract_integer_define_value "pthread.h" "PTHREAD_MUTEX_NORMAL"
AC_DEFINE_UNQUOTED([MPC_CONFIG_PTHREAD_MUTEX_NORMAL], [$MACRO_VALUE], [Value for PTHREAD_MUTEX_NORMAL from pthread.h])
AC_SUBST([MPC_CONFIG_PTHREAD_MUTEX_NORMAL], [$MACRO_VALUE])

#PTHREAD_MUTEX_RECURSIVE

extract_integer_define_value "pthread.h" "PTHREAD_MUTEX_RECURSIVE"
AC_DEFINE_UNQUOTED([MPC_CONFIG_PTHREAD_MUTEX_RECURSIVE], [$MACRO_VALUE], [Value for PTHREAD_MUTEX_RECURSIVE from pthread.h])
AC_SUBST([MPC_CONFIG_PTHREAD_MUTEX_RECURSIVE], [$MACRO_VALUE])

#PTHREAD_MUTEX_ERRORCHECK

extract_integer_define_value "pthread.h" "PTHREAD_MUTEX_ERRORCHECK"
AC_DEFINE_UNQUOTED([MPC_CONFIG_PTHREAD_MUTEX_ERRORCHECK], [$MACRO_VALUE], [Value for PTHREAD_MUTEX_ERRORCHECK from pthread.h])
AC_SUBST([MPC_CONFIG_PTHREAD_MUTEX_ERRORCHECK], [$MACRO_VALUE])

#PTHREAD_MUTEX_DEFAULT

extract_integer_define_value "pthread.h" "PTHREAD_MUTEX_DEFAULT"
AC_DEFINE_UNQUOTED([MPC_CONFIG_PTHREAD_MUTEX_DEFAULT], [$MACRO_VALUE], [Value for PTHREAD_MUTEX_DEFAULT from pthread.h])
AC_SUBST([MPC_CONFIG_PTHREAD_MUTEX_DEFAULT], [$MACRO_VALUE])

#PTHREAD_PRIO_NONE

extract_integer_define_value "pthread.h" "PTHREAD_PRIO_NONE"
AC_DEFINE_UNQUOTED([MPC_CONFIG_PTHREAD_PRIO_NONE], [$MACRO_VALUE], [Value for PTHREAD_PRIO_NONE from pthread.h])
AC_SUBST([MPC_CONFIG_PTHREAD_PRIO_NONE], [$MACRO_VALUE])

#PTHREAD_PRIO_INHERIT

extract_integer_define_value "pthread.h" "PTHREAD_PRIO_INHERIT"
AC_DEFINE_UNQUOTED([MPC_CONFIG_PTHREAD_PRIO_INHERIT], [$MACRO_VALUE], [Value for PTHREAD_PRIO_INHERIT from pthread.h])
AC_SUBST([MPC_CONFIG_PTHREAD_PRIO_INHERIT], [$MACRO_VALUE])

#PTHREAD_PRIO_NONE

extract_integer_define_value "pthread.h" "PTHREAD_PRIO_PROTECT"
AC_DEFINE_UNQUOTED([MPC_CONFIG_PTHREAD_PRIO_PROTECT], [$MACRO_VALUE], [Value for PTHREAD_PRIO_PROTECT from pthread.h])
AC_SUBST([MPC_CONFIG_PTHREAD_PRIO_PROTECT], [$MACRO_VALUE])

#PTHREAD_CREATE_JOINABLE

extract_integer_define_value "pthread.h" "PTHREAD_CREATE_JOINABLE"
AC_DEFINE_UNQUOTED([MPC_CONFIG_PTHREAD_CREATE_JOINABLE], [$MACRO_VALUE], [Value for PTHREAD_CREATE_JOINABLE from pthread.h])
AC_SUBST([MPC_CONFIG_PTHREAD_CREATE_JOINABLE], [$MACRO_VALUE])

#PTHREAD_CREATE_DETACHED

extract_integer_define_value "pthread.h" "PTHREAD_CREATE_DETACHED"
AC_DEFINE_UNQUOTED([MPC_CONFIG_PTHREAD_CREATE_DETACHED], [$MACRO_VALUE], [Value for PTHREAD_CREATE_DETACHED from pthread.h])
AC_SUBST([MPC_CONFIG_PTHREAD_CREATE_DETACHED], [$MACRO_VALUE])

#PTHREAD_INHERIT_SCHED

extract_integer_define_value "pthread.h" "PTHREAD_INHERIT_SCHED"
AC_DEFINE_UNQUOTED([MPC_CONFIG_PTHREAD_INHERIT_SCHED], [$MACRO_VALUE], [Value for PTHREAD_INHERIT_SCHED from pthread.h])
AC_SUBST([MPC_CONFIG_PTHREAD_INHERIT_SCHED], [$MACRO_VALUE])

#PTHREAD_EXPLICIT_SCHED

extract_integer_define_value "pthread.h" "PTHREAD_EXPLICIT_SCHED"
AC_DEFINE_UNQUOTED([MPC_CONFIG_PTHREAD_EXPLICIT_SCHED], [$MACRO_VALUE], [Value for PTHREAD_EXPLICIT_SCHED from pthread.h])
AC_SUBST([MPC_CONFIG_PTHREAD_EXPLICIT_SCHED], [$MACRO_VALUE])

#PTHREAD_SCOPE_SYSTEM

extract_integer_define_value "pthread.h" "PTHREAD_SCOPE_SYSTEM"
AC_DEFINE_UNQUOTED([MPC_CONFIG_PTHREAD_SCOPE_SYSTEM], [$MACRO_VALUE], [Value for PTHREAD_SCOPE_SYSTEM from pthread.h])
AC_SUBST([MPC_CONFIG_PTHREAD_SCOPE_SYSTEM], [$MACRO_VALUE])

#PTHREAD_SCOPE_PROCESS

extract_integer_define_value "pthread.h" "PTHREAD_SCOPE_PROCESS"
AC_DEFINE_UNQUOTED([MPC_CONFIG_PTHREAD_SCOPE_PROCESS], [$MACRO_VALUE], [Value for PTHREAD_SCOPE_PROCESS from pthread.h])
AC_SUBST([MPC_CONFIG_PTHREAD_SCOPE_PROCESS], [$MACRO_VALUE])

#PTHREAD_PROCESS_PRIVATE

extract_integer_define_value "pthread.h" "PTHREAD_PROCESS_PRIVATE"
AC_DEFINE_UNQUOTED([MPC_CONFIG_PTHREAD_PROCESS_PRIVATE], [$MACRO_VALUE], [Value for PTHREAD_PROCESS_PRIVATE from pthread.h])
AC_SUBST([MPC_CONFIG_PTHREAD_PROCESS_PRIVATE], [$MACRO_VALUE])

#PTHREAD_PROCESS_SHARED

extract_integer_define_value "pthread.h" "PTHREAD_PROCESS_SHARED"
AC_DEFINE_UNQUOTED([MPC_CONFIG_PTHREAD_PROCESS_SHARED], [$MACRO_VALUE], [Value for PTHREAD_PROCESS_SHARED from pthread.h])
AC_SUBST([MPC_CONFIG_PTHREAD_PROCESS_SHARED], [$MACRO_VALUE])

#PTHREAD_CANCEL_ENABLE

extract_integer_define_value "pthread.h" "PTHREAD_CANCEL_ENABLE"
AC_DEFINE_UNQUOTED([MPC_CONFIG_PTHREAD_CANCEL_ENABLE], [$MACRO_VALUE], [Value for PTHREAD_CANCEL_ENABLE from pthread.h])
AC_SUBST([MPC_CONFIG_PTHREAD_CANCEL_ENABLE], [$MACRO_VALUE])

#PTHREAD_CANCEL_DISABLE

extract_integer_define_value "pthread.h" "PTHREAD_CANCEL_DISABLE"
AC_DEFINE_UNQUOTED([MPC_CONFIG_PTHREAD_CANCEL_DISABLE], [$MACRO_VALUE], [Value for PTHREAD_CANCEL_DISABLE from pthread.h])
AC_SUBST([MPC_CONFIG_PTHREAD_CANCEL_DISABLE], [$MACRO_VALUE])

#PTHREAD_CANCEL_DEFERRED

extract_integer_define_value "pthread.h" "PTHREAD_CANCEL_DEFERRED"
AC_DEFINE_UNQUOTED([MPC_CONFIG_PTHREAD_CANCEL_DEFERRED], [$MACRO_VALUE], [Value for PTHREAD_CANCEL_DEFERRED from pthread.h])
AC_SUBST([MPC_CONFIG_PTHREAD_CANCEL_DEFERRED], [$MACRO_VALUE])

#PTHREAD_BARRIER_SERIAL_THREAD

extract_integer_define_value "pthread.h" "PTHREAD_BARRIER_SERIAL_THREAD"
AC_DEFINE_UNQUOTED([MPC_CONFIG_PTHREAD_BARRIER_SERIAL_THREAD], [$MACRO_VALUE], [Value for PTHREAD_BARRIER_SERIAL_THREAD from pthread.h])
AC_SUBST([MPC_CONFIG_PTHREAD_BARRIER_SERIAL_THREAD], [$MACRO_VALUE])

#PTHREAD_CANCEL_ASYNCHRONOUS

extract_integer_define_value "pthread.h" "PTHREAD_CANCEL_ASYNCHRONOUS"
AC_DEFINE_UNQUOTED([MPC_CONFIG_PTHREAD_CANCEL_ASYNCHRONOUS], [$MACRO_VALUE], [Value for PTHREAD_CANCEL_ASYNCHRONOUS from pthread.h])
AC_SUBST([MPC_CONFIG_PTHREAD_CANCEL_ASYNCHRONOUS], [$MACRO_VALUE])

#PTHREAD_CANCELED

extract_integer_define_value "pthread.h" "PTHREAD_CANCELED"
AC_DEFINE_UNQUOTED([MPC_CONFIG_PTHREAD_CANCELED], [$MACRO_VALUE], [Value for PTHREAD_CANCELED from pthread.h])
AC_SUBST([MPC_CONFIG_PTHREAD_CANCELED], [$MACRO_VALUE])

# For sched.h

extract_integer_define_value "sched.h" "SCHED_OTHER"
AC_DEFINE_UNQUOTED([MPC_CONFIG_SCHED_OTHER], [$MACRO_VALUE], [Value of SCHED_OTHER from sched.h])
AC_SUBST([MPC_CONFIG_SCHED_OTHER], [$MACRO_VALUE])

extract_integer_define_value "sched.h" "SCHED_RR"
AC_DEFINE_UNQUOTED([MPC_CONFIG_SCHED_RR], [$MACRO_VALUE], [Value of SCHED_RR from sched.h])
AC_SUBST([MPC_CONFIG_SCHED_RR], [$MACRO_VALUE])

extract_integer_define_value "sched.h" "SCHED_FIFO"
AC_DEFINE_UNQUOTED([MPC_CONFIG_SCHED_FIFO], [$MACRO_VALUE], [Value of SCHED_FIFO from sched.h])
AC_SUBST([MPC_CONFIG_SCHED_FIFO], [$MACRO_VALUE])

AC_CHECK_FUNCS(pthread_yield)
AC_CHECK_FUNCS(pthread_atfork )
AC_CHECK_FUNCS(pthread_cancel )
AC_CHECK_FUNCS(pthread_create )
AC_CHECK_FUNCS(pthread_detach )
AC_CHECK_FUNCS(pthread_equal )
AC_CHECK_FUNCS(pthread_exit )
AC_CHECK_FUNCS(pthread_getconcurrency )
AC_CHECK_FUNCS(pthread_getcpuclockid )
AC_CHECK_FUNCS(pthread_getschedparam )
AC_CHECK_FUNCS(pthread_getspecific )
AC_CHECK_FUNCS(pthread_join )
AC_CHECK_FUNCS(pthread_key_create )
AC_CHECK_FUNCS(pthread_key_delete )
AC_CHECK_FUNCS(pthread_once )

AC_CHECK_SIZEOF(pthread_key_t)
AC_SUBST([SIZEOF_PTHREAD_KEY_T],[$ac_cv_sizeof_pthread_key_t])


AC_MSG_CHECKING([Checking if PTHREAD_STACK_MIN is defined])

#
# PTHREAD STACK MIN
#

have_pthread_stack_min=no
AC_RUN_IFELSE([AC_LANG_PROGRAM([
                                #include <pthread.h>
                                #include <limits.h>
                                ],[
                                #ifndef PTHREAD_STACK_MIN
                                exit(1);
                                #endif
                                ])], [have_pthread_stack_min=yes])

AC_MSG_RESULT([$have_pthread_stack_min])

if test "x$have_pthread_keys_max" = "x1"; then
        extract_integer_define_value "pthread.h" "PTHREAD_STACK_MIN" "no"

        if test -z "$MACRO_VALUE"; then
                extract_integer_define_value "limits.h" "PTHREAD_STACK_MIN" "no"
                if test -z "$MACRO_VALUE"; then
                        MACRO_VALUE="8192"
                fi
        fi
else
        MACRO_VALUE="8192"
fi


AC_SUBST([MPC_CONFIG_PTHREAD_STACK_MIN], [$MACRO_VALUE])

#
# Pthread keys MAX
#

AC_MSG_CHECKING([support for PTHREAD_KEYS_MAX keyword])

have_pthread_keys_max=no
AC_RUN_IFELSE([AC_LANG_PROGRAM([
                                #include <pthread.h>
                                ],[
                                #ifndef PTHREAD_KEYS_MAX
                                exit(1);
                                #endif
                                ])], [have_pthread_keys_max=yes])

AC_MSG_RESULT([$have_pthread_keys_max])


if test "x$have_pthread_keys_max" = "x1"; then
        AC_DEFINE([MPC_THREAD_KEYS_MAX], [PTHREAD_KEYS_MAX], [PTHREAD_KEYS_MAX from header (requires pthread.h)])
        extract_integer_define_value "pthread.h" "MPC_THREAD_KEYS_MAX"
        AC_SUBST([MPC_THREAD_KEYS_MAX], [$MACRO_VALUE])
else
        AC_MSG_CHECKING([PTHREAD_KEYS_MAX value manually (not defined)])
        for key_count in `seq 0 128 65536`
        do

        current_key_count_supported=0

        have_pthread_keys_max=no
        AC_RUN_IFELSE([AC_LANG_PROGRAM([
                                        #include <pthread.h>
                                        ],[
                                        int ptkm = 0;

                                        do
                                        {
                                                int i;
                                                pthread_key_t k;

                                                /*Pthread key max determination */
                                                /* ensure to fail one step ahead reason for + 128 */
                                                for ( i = 0; i < $key_count + 128; i++ )
                                                {
                                                        if ( pthread_key_create ( &k, NULL ) != 0 )
                                                        {
                                                                return 1;
                                                        }
                                                }

                                                ptkm = i;
                                        }
                                        while ( 0 );
                                        ])], [current_key_count_supported=1])

        if test "x$current_key_count_supported" != "x1"; then
                break
        fi

        done

        AC_DEFINE_UNQUOTED([MPC_THREAD_KEYS_MAX], [$key_count], [Experimentally defined PTHREAD_KEYS_MAX])
        AC_SUBST([MPC_THREAD_KEYS_MAX], [$key_count])

        AC_MSG_RESULT([$key_count])

fi


AC_CHECK_TYPE(pthread_attr_t,is_thread=yes,is_thread=no,[#include <pthread.h>])

if test "$is_thread" = "yes" ; then
        AC_CHECK_SIZEOF(pthread_attr_t,,[#include <pthread.h>])
        AC_SUBST([SIZEOF_PTHREAD_ATTR_T],[$ac_cv_sizeof_pthread_attr_t])

        AC_CHECK_FUNCS(pthread_attr_destroy )
        AC_CHECK_FUNCS(pthread_attr_getdetachstate )
        AC_CHECK_FUNCS(pthread_attr_getguardsize )
        AC_CHECK_FUNCS(pthread_attr_getinheritsched )
        AC_CHECK_FUNCS(pthread_attr_getschedparam )
        AC_CHECK_FUNCS(pthread_attr_getschedpolicy )
        AC_CHECK_FUNCS(pthread_attr_getscope )
        AC_CHECK_FUNCS(pthread_attr_getstack )
        AC_CHECK_FUNCS(pthread_attr_getstackaddr )
        AC_CHECK_FUNCS(pthread_attr_getstacksize )
        AC_CHECK_FUNCS(pthread_attr_init )
        AC_CHECK_FUNCS(pthread_attr_setdetachstate )
        AC_CHECK_FUNCS(pthread_attr_setguardsize )
        AC_CHECK_FUNCS(pthread_attr_setinheritsched )
        AC_CHECK_FUNCS(pthread_attr_setschedparam )
        AC_CHECK_FUNCS(pthread_attr_setschedpolicy )
        AC_CHECK_FUNCS(pthread_attr_setscope )
        AC_CHECK_FUNCS(pthread_attr_setstack )
        AC_CHECK_FUNCS(pthread_attr_setstackaddr )
        AC_CHECK_FUNCS(pthread_attr_setstacksize )
        AC_CHECK_FUNCS(pthread_attr_setaffinity_np )
fi

AC_CHECK_TYPE(pthread_barrier_t,is_barrier=yes,is_barrier=no,[#include <pthread.h>])

if test "$is_barrier" = "yes" ; then
        AC_CHECK_SIZEOF(pthread_barrier_t,,[#include <pthread.h>])
        AC_SUBST([SIZEOF_PTHREAD_BARRIER_T],[$ac_cv_sizeof_pthread_barrier_t])

        AC_CHECK_SIZEOF(pthread_barrierattr_t,,[#include <pthread.h>])
        AC_SUBST([SIZEOF_PTHREAD_BARRIERATTR_T],[$ac_cv_sizeof_pthread_barrierattr_t])

        AC_CHECK_FUNCS(pthread_barrier_destroy )
        AC_CHECK_FUNCS(pthread_barrier_init )
        AC_CHECK_FUNCS(pthread_barrier_wait )
        AC_CHECK_FUNCS(pthread_barrierattr_destroy )
        AC_CHECK_FUNCS(pthread_barrierattr_getpshared )
        AC_CHECK_FUNCS(pthread_barrierattr_init )
        AC_CHECK_FUNCS(pthread_barrierattr_setpshared )
fi

AC_CHECK_SIZEOF(pthread_once_t,,[#include <pthread.h>])
AC_SUBST([SIZEOF_PTHREAD_ONCE_T],[$ac_cv_sizeof_pthread_once_t])

AC_CHECK_TYPE(pthread_cond_t,is_cond=yes,is_cond=no,[#include <pthread.h>])
if test "$is_cond" = "yes" ; then
        AC_CHECK_SIZEOF(pthread_cond_t,,[#include <pthread.h>])
        AC_SUBST([SIZEOF_PTHREAD_COND_T],[$ac_cv_sizeof_pthread_cond_t])

        AC_CHECK_SIZEOF(pthread_condattr_t,,[#include <pthread.h>])
        AC_SUBST([SIZEOF_PTHREAD_CONDATTR_T],[$ac_cv_sizeof_pthread_condattr_t])

        AC_CHECK_FUNCS(pthread_cond_broadcast )
        AC_CHECK_FUNCS(pthread_cond_destroy )
        AC_CHECK_FUNCS(pthread_cond_init )
        AC_CHECK_FUNCS(pthread_cond_signal )
        AC_CHECK_FUNCS(pthread_cond_timedwait )
        AC_CHECK_FUNCS(pthread_cond_wait )
        AC_CHECK_FUNCS(pthread_condattr_destroy )
        AC_CHECK_FUNCS(pthread_condattr_getclock )
        AC_CHECK_FUNCS(pthread_condattr_getpshared )
        AC_CHECK_FUNCS(pthread_condattr_init )
        AC_CHECK_FUNCS(pthread_condattr_setclock )
        AC_CHECK_FUNCS(pthread_condattr_setpshared )
fi

AC_CHECK_TYPE(pthread_mutex_t,is_mutex=yes,is_mutex=no,[#include <pthread.h>])

if test "$is_mutex" = "yes" ; then 
        AC_CHECK_SIZEOF(pthread_mutex_t,,[#include <pthread.h>])
        AC_SUBST([SIZEOF_PTHREAD_MUTEX_T],[$ac_cv_sizeof_pthread_mutex_t])

        AC_CHECK_SIZEOF(pthread_mutexattr_t,,[#include <pthread.h>])
        AC_SUBST([SIZEOF_PTHREAD_MUTEXATTR_T],[$ac_cv_sizeof_pthread_mutexattr_t])

        AC_CHECK_FUNCS(pthread_mutex_destroy )
        AC_CHECK_FUNCS(pthread_mutex_getprioceiling )
        AC_CHECK_FUNCS(pthread_mutex_init )
        AC_CHECK_FUNCS(pthread_mutex_lock )
        AC_CHECK_FUNCS(pthread_mutex_setprioceiling )
        AC_CHECK_FUNCS(pthread_mutex_timedlock )
        AC_CHECK_FUNCS(pthread_mutex_trylock )
        AC_CHECK_FUNCS(pthread_mutex_unlock )
        AC_CHECK_FUNCS(pthread_mutexattr_destroy )
        AC_CHECK_FUNCS(pthread_mutexattr_getprioceiling )
        AC_CHECK_FUNCS(pthread_mutexattr_getprotocol )
        AC_CHECK_FUNCS(pthread_mutexattr_getpshared )
        AC_CHECK_FUNCS(pthread_mutexattr_gettype )
        AC_CHECK_FUNCS(pthread_mutexattr_init )
        AC_CHECK_FUNCS(pthread_mutexattr_setprioceiling )
        AC_CHECK_FUNCS(pthread_mutexattr_setprotocol )
        AC_CHECK_FUNCS(pthread_mutexattr_setpshared )
        AC_CHECK_FUNCS(pthread_mutexattr_settype )
fi

AC_CHECK_TYPE(pthread_rwlock_t,is_rwlock=yes,is_rwlock=no,[#include <pthread.h>])

if test "$is_rwlock" = "yes" ; then 
        AC_CHECK_SIZEOF(pthread_rwlock_t,,[#include <pthread.h>])
        AC_SUBST([SIZEOF_PTHREAD_RWLOCK_T],[$ac_cv_sizeof_pthread_rwlock_t])

        AC_CHECK_SIZEOF(pthread_rwlockattr_t,,[#include <pthread.h>])
        AC_SUBST([SIZEOF_PTHREAD_RWLOCKATTR_T],[$ac_cv_sizeof_pthread_rwlockattr_t])

        AC_CHECK_FUNCS(pthread_rwlock_destroy )
        AC_CHECK_FUNCS(pthread_rwlock_init )
        AC_CHECK_FUNCS(pthread_rwlock_rdlock )
        AC_CHECK_FUNCS(pthread_rwlock_timedrdlock )
        AC_CHECK_FUNCS(pthread_rwlock_timedwrlock )
        AC_CHECK_FUNCS(pthread_rwlock_tryrdlock )
        AC_CHECK_FUNCS(pthread_rwlock_trywrlock )
        AC_CHECK_FUNCS(pthread_rwlock_unlock )
        AC_CHECK_FUNCS(pthread_rwlock_wrlock )
        AC_CHECK_FUNCS(pthread_rwlockattr_destroy )
        AC_CHECK_FUNCS(pthread_rwlockattr_getpshared )
        AC_CHECK_FUNCS(pthread_rwlockattr_init )
        AC_CHECK_FUNCS(pthread_rwlockattr_setpshared )
fi

AC_CHECK_FUNCS(pthread_self )
AC_CHECK_FUNCS(pthread_setcancelstate )
AC_CHECK_FUNCS(pthread_setcanceltype )
AC_CHECK_FUNCS(pthread_setconcurrency )
AC_CHECK_FUNCS(pthread_setschedparam )
AC_CHECK_FUNCS(pthread_setschedprio )
AC_CHECK_FUNCS(pthread_setspecific )

AC_CHECK_TYPE(pthread_spinlock_t,is_spinlock=yes,is_spinlock=no,[#include <pthread.h>])

if test "$is_spinlock" = "yes" ; then
        AC_CHECK_SIZEOF(pthread_spinlock_t)
        AC_SUBST([SIZEOF_PTHREAD_SPINLOCK_T],[$ac_cv_sizeof_pthread_spinlock_t])

        AC_CHECK_FUNCS(pthread_spin_destroy )
        AC_CHECK_FUNCS(pthread_spin_init )
        AC_CHECK_FUNCS(pthread_spin_lock )
        AC_CHECK_FUNCS(pthread_spin_trylock )
        AC_CHECK_FUNCS(pthread_spin_unlock )
fi

AC_CHECK_FUNCS(pthread_testcancel )

AC_CHECK_HEADER([semaphore.h])

AC_CHECK_TYPE(sem_t,is_semaphore=yes,is_semaphore=no,[#include <semaphore.h>])

if test "$is_semaphore" = "yes" ; then 
        AC_CHECK_SIZEOF(sem_t,,[#include <semaphore.h>])
        AC_SUBST([SIZEOF_SEM_T],[$ac_cv_sizeof_sem_t])
        AC_CHECK_FUNCS(sem_init )
        AC_CHECK_FUNCS(sem_wait )
        AC_CHECK_FUNCS(sem_trywait )
        AC_CHECK_FUNCS(sem_post )
        AC_CHECK_FUNCS(sem_getvalue )
        AC_CHECK_FUNCS(sem_destroy )
        AC_CHECK_FUNCS(sem_open )
        AC_CHECK_FUNCS(sem_close )
        AC_CHECK_FUNCS(sem_unlink )
        AC_CHECK_FUNCS(sem_timedwait )

	extract_integer_define_value "semaphore.h" "SEM_VALUE_MAX" "no"

	if test -z "$MACRO_VALUE"; then
		MACRO_VALUE="2147483647"
	fi

	AC_DEFINE_UNQUOTED([MPC_CONFIG_SEM_VALUE_MAX], [$MACRO_VALUE], [Value of SEM_VALUE_MAX from semaphore.h])
        AC_SUBST([MPC_CONFIG_SEM_VALUE_MAX],[$MACRO_VALUE])

	extract_integer_define_value "semaphore.h" "SEM_FAILED"
	AC_DEFINE_UNQUOTED([MPC_CONFIG_SEM_FAILED], [$MACRO_VALUE], [Value of SEM_FAILED from semaphore.h])
        AC_SUBST([MPC_CONFIG_SEM_FAILED],[$MACRO_VALUE])

fi

AC_CHECK_FUNCS(pthread_sigmask )
AC_CHECK_FUNCS(pthread_kill )
AC_CHECK_FUNCS(sched_get_priority_max )
AC_CHECK_FUNCS(sched_get_priority_min )

#
# Configure Privatization Support
#

AC_ARG_ENABLE([process-mode],
[AS_HELP_STRING([--enable-process-mode],
                [Compile MPC in process mode])],
[mpc_in_process_mode=$enableval],
[mpc_in_process_mode=no])
if test "x${mpc_in_process_mode}" = "xyes"; then
        AC_DEFINE([MPC_IN_PROCESS_MODE], [1], [MPC compiled in process mode])
        MPC_IN_PROCESS_MODE="yes"
fi

AC_SUBST([MPC_IN_PROCESS_MODE])

mpc_privatization_support="no"

if test "x$mpc_in_process_mode" = "xno"; then

        MPC_DETECT_DEPENDENCY([extls.h],[extls], [extls_init], [EXTLS])
        if test "x$mpc_dep_found" = "xyes"; then
                AC_DEFINE([MPC_USE_EXTLS], [1], [Extls support is enabled])
                mpc_privatization_support="yes"
	fi
fi

#
# Configure MPC_Arch
#

MPC_CONTEXT_SWITCH_SUPPORTED_ARCHITECTURE="x86_64"

AC_MSG_CHECKING([which context switches to use])

MPC_ARCH_CONTEXT_SWITCH="generic"

if is_in_list "$MPC_TARGET_ARCHITECTURE" "$MPC_CONTEXT_SWITCH_SUPPORTED_ARCHITECTURE"; then
        MPC_ARCH_CONTEXT_SWITCH="$MPC_TARGET_ARCHITECTURE"
        AC_MSG_RESULT($MPC_TARGET_ARCHITECTURE)
else
        AC_MSG_RESULT(standard)
fi

# Note that by construction the following conditionnals are multually exclusive
AM_CONDITIONAL([MPC_ARCH_X86_64_CONTEXT_SWITCH], [test "x${MPC_ARCH_CONTEXT_SWITCH}" = "xx86_64"])

MPC_MICROTASK_SUPPORTED_ARCHITECTURE="x86_64"

AC_MSG_CHECKING([which Configuring optimized microtasks])

MPC_ARCH_MICROTASK_ARCH="generic"

if is_in_list "$MPC_TARGET_ARCHITECTURE" "$MPC_MICROTASK_SUPPORTED_ARCHITECTURE"; then
        MPC_ARCH_MICROTASK_ARCH="$MPC_TARGET_ARCHITECTURE"
        AC_MSG_RESULT($MPC_TARGET_ARCHITECTURE)
else
        AC_MSG_RESULT(Using standard microtasks)
fi

# Note that by construction the following conditionnals are multually exclusive
AM_CONDITIONAL([MPC_ARCH_X86_64_MICROTASK], [test "x${MPC_ARCH_MICROTASK_ARCH}" = "xx86_64"])
# Note that by construction the following conditionnals are multually exclusive
AM_CONDITIONAL([MPC_ARCH_GENERIC_MICROTASK], [test "x${MPC_ARCH_MICROTASK_ARCH}" = "xgeneric"])


AC_MSG_CHECKING([KMP optimized asm functions])

if test "x$MPC_ARCH_MICROTASK_ARCH" = "xx86_64"; then
        AC_DEFINE([KMP_ARCH_X86_64],[1], [KMP intrisincs x64 enabled])
        AC_MSG_RESULT([x86_64])
fi

if test "x$MPC_ARCH_MICROTASK_ARCH" = "xx86"; then
        AC_DEFINE([KMP_ARCH_X86],[1], [KMP intrisincs enabled])
        AC_MSG_RESULT([x86_64])
fi

#
# Guess Compiler Family
#

mpc_compiler_family="notset"

mpc_compiler_is_gcc="no"

AC_RUN_IFELSE([AC_LANG_PROGRAM([#include <stdlib.h>],[
#ifndef __GNUC__
exit(1);
#endif
])], [mpc_compiler_is_gcc="yes"])

AC_MSG_CHECKING([if compiler is gcc])

if test "x${mpc_compiler_is_gcc}" = "xyes"; then
        AC_MSG_RESULT([yes])
else
        AC_MSG_RESULT([no])
fi

mpc_compiler_is_icc="no"

AC_RUN_IFELSE([AC_LANG_PROGRAM([#include <stdlib.h>],[
#ifndef __INTEL_COMPILER
exit(1);
#endif
])], [mpc_compiler_is_icc="yes"])

AC_MSG_CHECKING([if compiler is icc])

if test "x${mpc_compiler_is_icc}" = "xyes"; then
        AC_MSG_RESULT([yes])
else
        AC_MSG_RESULT([no])
fi

mpc_compiler_is_clang="no"

AC_RUN_IFELSE([AC_LANG_PROGRAM([#include <stdlib.h>],[
#ifndef __clang__
exit(1);
#endif
])], [mpc_compiler_is_clang="yes"])

AC_MSG_CHECKING([if compiler is clang])

if test "x${mpc_compiler_is_clang}" = "xyes"; then
        AC_MSG_RESULT([yes])
else
        AC_MSG_RESULT([no])
fi

# Now set the compiler family

AC_MSG_CHECKING([compiler family to be])

if test "x$mpc_compiler_is_icc" = "xyes"; then
        AC_MSG_RESULT([icc])
        mpc_compiler_family="icc"
else
        if test "x$mpc_compiler_is_clang" = "xyes"; then
                AC_MSG_RESULT([clang])
                mpc_compiler_family="clang"
        else
                        if test "x$mpc_compiler_is_gcc" = "xyes"; then
                                AC_MSG_RESULT([gcc])
                                mpc_compiler_family="gcc"
                        fi
        fi

fi


#
# Mpc Optionnal Flags
#

AC_ARG_ENABLE([shell-colors],
[AS_HELP_STRING([--enable-shell-colors],
                [Activate Color Output in MPC])],
[mpc_shell_color=$enableval],
[mpc_shell_color=yes])

if test "x${mpc_shell_color}" = "xyes"; then
        AC_DEFINE([MPC_ENABLE_SHELL_COLORS], [1], [MPC color output was enabled])
        MPC_ENABLE_SHELL_COLORS=1
fi

AC_SUBST([MPC_ENABLE_SHELL_COLORS])


AC_ARG_ENABLE([cma],
[AS_HELP_STRING([--enable-cma],
                [Activate Cross Memory Attach for SHM communications])],
[mpc_enable_cma=$enableval],
[mpc_enable_cma=no])

if test "x${mpc_enable_cma}" = "xyes"; then

        #Check the content of the YAMA file if present
        if test -f /proc/sys/kernel/yama/ptrace_scope; then
                AC_MSG_CHECKING([YAMA ptrace rights])
                YAMA_VAL=$(cat /proc/sys/kernel/yama/ptrace_scope)

                if test "x$YAMA_VAL" = "x0"; then
                        AC_MSG_RESULT([OK])
                else
                        AC_MSG_RESULT([FAIL])
                        AC_MSG_NOTICE([/proc/sys/kernel/yama/ptrace_scope shoud contain 0 for CMA support in MPC])
                        mpc_enable_cma="no"
                fi
        fi

        AC_MSG_CHECKING([enabling CMA])
        if test "x$mpc_enable_cma" = "xyes"; then
                AC_MSG_RESULT([yes])
                AC_DEFINE([MPC_USE_CMA], [1], [MPC CMA for SHM drive is enabled])
        else
                AC_MSG_RESULT([no])
        fi

fi

AC_ARG_ENABLE([opt],
[AS_HELP_STRING([--enable-opt],
                [Build MPC with optimization flags])],
[mpc_enable_opt=$enableval],
[mpc_enable_opt=yes])

MPC_PORTABLE_OPT_FLAGS=" -O3"

case $mpc_compiler_family in
        "gcc")
                MPC_OPT_FLAGS=" -O3  -funroll-loops"
        ;;
        "icc")
                MPC_OPT_FLAGS=" -O3  -opt-prefetch -unroll-aggressive"
        ;;
        "clang")
                MPC_OPT_FLAGS=" -O3"
        ;;
        *)
                MPC_OPT_FLAGS=" $MPC_PORTABLE_OPT_FLAGS"
        ;;
esac

if test "x${mpc_enable_opt}" = "xyes"; then
        CPPFLAGS+=" $MPC_OPT_FLAGS"
fi


AC_ARG_ENABLE([opt-native],
[AS_HELP_STRING([--enable-opt-native],
                [Build MPC for current architecture])],
[mpc_enable_native_opt=$enableval],
[mpc_enable_native_opt=no])

MPC_PORTABLE_NATIVE_FLAGS=" "

case $mpc_compiler_family in
        "gcc")
                MPC_NATIVE_FLAGS=" -march=native"
        ;;
        "icc")
                MPC_NATIVE_FLAGS=" -xhost"
        ;;
        "clang")
                MPC_NATIVE_FLAGS=" -march=native"
        ;;
        *)
                MPC_NATIVE_FLAGS=" $MPC_PORTABLE_NATIVE_FLAGS"
        ;;
esac

if test "x${mpc_enable_native_opt}" = "xyes"; then
        CPPFLAGS+=" $MPC_NATIVE_FLAGS"
fi

AC_ARG_ENABLE([debug-messages],
[AS_HELP_STRING([--enable-debug-messages],
                [Output debug messages when running MPC])],
[mpc_debug_messages=$enableval],
[mpc_debug_messages=no])

if test "x${mpc_debug_messages}" = "xyes"; then
        AC_DEFINE([MPC_ENABLE_DEBUG_MESSAGES], [1], [MPC debug messages output was enabled])
fi

AC_ARG_ENABLE([debug],
[AS_HELP_STRING([--enable-debug],
                [Compile MPC in debug mode])],
[mpc_debug=$enableval],
[mpc_debug=no])

MPC_PORTABLE_DEBUG_FLAGS=" -g"

case $mpc_compiler_family in
        "gcc")
                MPC_DEBUG_FLAGS=" -g -Wall -Wextra"
        ;;
        "icc")
                MPC_DEBUG_FLAGS=" -g -Wall"
        ;;
        "clang")
                MPC_DEBUG_FLAGS=" -g -Wall -Wextra"
        ;;
        *)
                MPC_DEBUG_FLAGS=" $MPC_PORTABLE_DEBUG_FLAGS"
        ;;
esac

# Todo we cannot pass it to APCC portably

if test "x${mpc_debug}" = "xyes"; then
        CFLAGS+=" $MPC_DEBUG_FLAGS"
fi

AC_ARG_ENABLE([profiler],
[AS_HELP_STRING([--enable-profiler],
                [Compile MPC with Internal Profiling Support])],
[MPC_HAVE_PROFILER=$enableval],
[MPC_HAVE_PROFILER=no])

if test "x${MPC_HAVE_PROFILER}" = "xyes"; then
        AC_DEFINE([MPC_Profiler], [1], [MPC_Profiler module was enabled])
fi

AM_CONDITIONAL([MPC_PROFILER_ENABLED], [test "x${MPC_HAVE_PROFILER}" = "xyes"])

AC_SUBST([MPC_HAVE_PROFILER])

#
# Modular MPC configuration
#

AC_ARG_ENABLE([threads],
[AS_HELP_STRING([--enable-threads],
                [Compile MPC with pthread and user-level thread support])],
[MPC_THREAD_ENABLED=$enableval],
[MPC_THREAD_ENABLED=yes])

if test "x${MPC_THREAD_ENABLED}" = "xyes"; then
        AC_DEFINE([MPC_Threads], [1], [MPC_THREAD module was enabled])
fi

AM_CONDITIONAL([MPC_THREAD_ENABLED], [test "x${MPC_THREAD_ENABLED}" = "xyes"])
AC_SUBST([MPC_THREAD_ENABLED])


AC_ARG_ENABLE([message-passing],
[AS_HELP_STRING([--enable-message-passing],
                [Compile MPC with Message Passing Support])],
[MPC_MESSAGE_PASSING_ENABLED=$enableval],
[MPC_MESSAGE_PASSING_ENABLED=yes])

if test "x${MPC_MESSAGE_PASSING_ENABLED}" = "xyes"; then
        AC_DEFINE([MPC_Message_Passing], [1], [MPC_MESSAGE_PASSING module was enabled])
fi

AM_CONDITIONAL([MPC_MESSAGE_PASSING_ENABLED], [test "x${MPC_MESSAGE_PASSING_ENABLED}" = "xyes"])
AC_SUBST([MPC_MESSAGE_PASSING_ENABLED])


AC_ARG_ENABLE([mpi],
[AS_HELP_STRING([--enable-mpi],
                [Compile MPC's MPI interface])],
[MPC_MPI_ENABLED=$enableval],
[MPC_MPI_ENABLED=yes])

if test "x${MPC_MPI_ENABLED}" = "xyes"; then
        AC_DEFINE([MPC_MPI], [1], [MPC_MPI module was enabled])
fi

AM_CONDITIONAL([MPC_MPI_ENABLED], [test "x${MPC_MPI_ENABLED}" = "xyes"])
AC_SUBST([MPC_MPI_ENABLED])


AC_ARG_ENABLE([openmp],
[AS_HELP_STRING([--enable-openmp],
                [Compile MPC's OpenMP interface])],
[MPC_OPENMP_ENABLED=$enableval],
[MPC_OPENMP_ENABLED=yes])

if test "x${MPC_OPENMP_ENABLED}" = "xyes"; then
        AC_DEFINE([MPC_OpenMP], [1], [MPC_OpenMP module was enabled])
fi

AM_CONDITIONAL([MPC_OPENMP_ENABLED], [test "x${MPC_OPENMP_ENABLED}" = "xyes"])
AC_SUBST([MPC_OPENMP_ENABLED])


#
# MPI IO Support (ROMIO)
#

# TODO move it its own M4 file

AC_ARG_ENABLE([mpiio],
[AS_HELP_STRING([--enable-mpiio],
                [Compile MPI IO support in MPC (romio)])],
[MPC_MPIIO_ENABLED=$enableval],
[MPC_MPIIO_ENABLED=yes])

if test "x${MPC_MPIIO_ENABLED}" = "xyes"; then
        AC_DEFINE([MPC_MPIIO], [1], [MPC_MPIIO module was enabled])
fi

AM_CONDITIONAL([MPC_MPIIO_ENABLED], [test "x${MPC_MPIIO_ENABLED}" = "xyes"])
AC_SUBST([MPC_MPIIO_ENABLED])

AM_CONDITIONAL([BUILD_ROMIO_EMBEDDED], [true])
AM_CONDITIONAL([BUILD_F77_BINDINGS], [true])
AM_CONDITIONAL([BUILD_MPIO_REQUEST], [false])
AM_CONDITIONAL([BUILD_MPIO_ERRHAN], [true])
AM_CONDITIONAL([MPIO_GLUE_DEFAULT], [false])
AM_CONDITIONAL([MPIO_GLUE_OPENMPI], [false])
AM_CONDITIONAL([MPIO_GLUE_MPICH], [true])

DEFINE_HAVE_MPI_GREQUEST="#define HAVE_MPI_GREQUEST"
AC_SUBST([DEFINE_HAVE_MPI_GREQUEST])

DEFINE_MPI_OFFSET="#define HAVE_MPI_OFFSET"
AC_SUBST([DEFINE_MPI_OFFSET])
NEEDS_MPI_FINT=""
AC_SUBST([NEEDS_MPI_FINT])
HAVE_MPI_INFO="#define HAVE_MPI_INFO"
AC_SUBST([HAVE_MPI_INFO])
HAVE_MPI_DARRAY_SUBARRAY="#define HAVE_MPI_DARRAY_SUBARRAY"
AC_SUBST([HAVE_MPI_DARRAY_SUBARRAY])

DEFINE_MPI_OFFSET="/* MPI OFFSET is in mpi.h */"
AC_SUBST([DEFINE_MPI_OFFSET])

#
# Check for statfs (many) and specifically f_fstypename field (BSD)
#
AC_CHECK_HEADERS(sys/vfs.h sys/param.h sys/mount.h sys/statvfs.h)
AC_CHECK_FUNCS([statfs])
AC_MSG_CHECKING([whether struct statfs properly defined])
AC_TRY_COMPILE([
#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif
#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
    ],[
    struct statfs f;
    ],
    pac_cv_have_statfs=yes,pac_cv_have_statfs=no
)
AC_MSG_RESULT($pac_cv_have_statfs)

if test "$pac_cv_have_statfs" = yes ; then
    AC_DEFINE(HAVE_STRUCT_STATFS,1,[Define if struct statfs can be compiled])
fi
AC_MSG_CHECKING([for f_type member of statfs structure])
AC_TRY_COMPILE([
#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif
#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
    ],[
    struct statfs f;
    memset(&f, 0, sizeof(f));
    f.f_type = 0;
    ],
    pac_cv_have_statfs_f_type=yes,
    pac_cv_have_statfs_f_type=no
)
AC_MSG_RESULT($pac_cv_have_statfs_f_type)
if test $pac_cv_have_statfs_f_type = yes ; then
    AC_DEFINE(ROMIO_HAVE_STRUCT_STATFS_WITH_F_TYPE, 1,[Define if statfs has f_type])
fi


AC_MSG_CHECKING([for f_fstypename member of statfs structure])
AC_TRY_COMPILE([
#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif
#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
    ],[
    struct statfs f;
    memset(&f, 0, sizeof(f));
    strncmp("nfs", f.f_fstypename, 3);
    ],
    pac_cv_have_statfs_f_fstypename=yes,
    pac_cv_have_statfs_f_fstypename=no
)
AC_MSG_RESULT($pac_cv_have_statfs_f_fstypename)
if test $pac_cv_have_statfs_f_fstypename = yes ; then
    AC_DEFINE(ROMIO_HAVE_STRUCT_STATFS_WITH_F_FSTYPENAME, 1,[Define if statfs has f_fstypename])
fi

#
# ROMIO File systems
#

ROMIO_FS_LIST=""

AM_CONDITIONAL([BUILD_AD_NFS], [false])

AM_CONDITIONAL([BUILD_AD_LUSTRE], [false])
AM_CONDITIONAL([LUSTRE_LOCKAHEAD], [false])

AM_CONDITIONAL([BUILD_AD_PANFS], [false])
AM_CONDITIONAL([BUILD_PANFS_OPEN6], [false])

AM_CONDITIONAL([BUILD_AD_DAOS], [false])
AM_CONDITIONAL([BUILD_AD_GPFS], [false])
AM_CONDITIONAL([BUILD_AD_BG], [false])
AM_CONDITIONAL([BUILD_AD_PE], [false])

AM_CONDITIONAL([BUILD_AD_PVFS2], [false])
AM_CONDITIONAL([BUILD_AD_TESTFS], [false])
AM_CONDITIONAL([BUILD_AD_XFS], [false])
AM_CONDITIONAL([BUILD_AD_IME], [false])

AM_CONDITIONAL([BUILD_AD_UFS], [true])

file_system_nfs="yes"
file_system_lustre="yes"
file_system_ime="yes"
file_system_ufs="yes"

file_system_xfs=""
file_system_gpfs=""
file_system_args=""

if test -n "$file_system_nfs" ; then
    AC_DEFINE(ROMIO_NFS,1,[Define for ROMIO with NFS])
    AC_MSG_WARN([File locks may not work with NFS.  See the Installation and
users manual for instructions on testing and if necessary fixing this])
    AM_CONDITIONAL([BUILD_AD_NFS], [true])
    ROMIO_FS_LIST="${ROMIO_FS_LIST} nfs"
fi


if test -n "$file_system_gpfs"; then
    AC_DEFINE(ROMIO_GPFS,1,[Define for ROMIO with GPFS])
    ROMIO_FS_LIST="${ROMIO_FS_LIST} gpfs"
fi

if test -n "$file_system_ufs"; then
    AC_DEFINE(ROMIO_UFS,1,[Define for ROMIO with UFS])
    ROMIO_FS_LIST="${ROMIO_FS_LIST} unix"
fi

AM_CONDITIONAL([BUILD_AD_BG],[false])
AM_CONDITIONAL([BUILD_AD_PE],[false])

if test "$file_system_args" = "BGQ" -a -n "$file_system_gpfs"; then
    AC_DEFINE(BGQPLATFORM,1,BGQ platform)
    AM_CONDITIONAL([BUILD_AD_BG],[true])
    dnl what if anything can make Blue Gene support aio?
    disable_aio=yes
fi

if test "$file_system_args" = "PE" -a -n "$file_system_gpfs"; then
    AC_DEFINE(PEPLATFORM,1,PE platform)
    AM_CONDITIONAL([BUILD_AD_PE],[true])
fi

#
# Verify presence of lustre/lustre_user.h
#
lustre_lockahead="no"
if test -n "$file_system_lustre"; then
    AC_CHECK_HEADERS([linux/lustre/lustre_user.h lustre/lustre_user.h],
        break)
    if test x"$ac_cv_header_linux_lustre_lustre_user_h" = "xyes" -o x"$ac_cv_header_lustre_lustre_user_h" = "xyes" ; then
        AC_DEFINE(ROMIO_LUSTRE, 1, [Define for ROMIO with LUSTRE])
	AM_CONDITIONAL([BUILD_AD_LUSTRE], [true])
	ROMIO_FS_LIST="${ROMIO_FS_LIST} lustre"
    else
        AC_MSG_NOTICE([LUSTRE support requested but cannot find lustre/lustre_user.h header file])
    fi
    AC_COMPILE_IFELSE([
        AC_LANG_SOURCE([
#include <lustre/lustre_user.h>
#include <sys/ioctl.h>
        int main(int argc, char **argv) {
            int fd=100;
            struct ladvise_hdr *ladvise_hdr;
            ioctl(fd, LL_IOC_LADVISE, &ladvise_hdr);
            return 0; }
        ])],
        lustre_lockahead="yes"
	AC_DEFINE(HAVE_LUSTRE_LOCKAHEAD, 1, [Define if LUSTRE_LOCKAHEAD is enabled.])
	AM_CONDITIONAL([LUSTRE_LOCKAHEAD], [true]))
fi

# Add conditional compilation of Lustre lockahead sources
AM_CONDITIONAL([LUSTRE_LOCKAHEAD],[test "$lustre_lockahead" = "yes"])

if test -n "$file_system_xfs"; then
    AC_DEFINE(ROMIO_XFS,1,[Define for ROMIO with XFS])
    AM_CONDITIONAL([BUILD_AD_XFS], [true])
    ROMIO_FS_LIST="${ROMIO_FS_LIST} xfs"
    # Check for memalign value
    AC_CACHE_CHECK([for memory alignment needed for direct I/O],
    pac_cv_memalignval,
    [
    rm -f confmemalignval
    rm -f /tmp/romio_tmp.bin
    AC_TRY_RUN([
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
int main(int argc, char **argv) { 
	struct dioattr st;
	int fd = open("/tmp/romio_tmp.bin", O_RDWR | O_CREAT, 0644);
	FILE *f=fopen("confmemalignval","w");
	if (fd == -1) exit(1);
	if (!f) exit(1);
	fcntl(fd, F_DIOINFO, &st);
	fprintf( f, "%u\n", st.d_mem);
	exit(0);
	}
	],
	pac_cv_memalignval=`cat confmemalignval`,
	pac_cv_memalignval="unknown",pac_cv_memalignval="unknown"
    )
    rm -f confmemalignval
    rm -f /tmp/romio_tmp.bin
    ])
    if test -n "$pac_cv_memalignval" -a "$pac_cv_memalignval" != 0 -a \
	"$pac_cv_memalignval" != "unknown" ; then
        CFLAGS="$CFLAGS -DXFS_MEMALIGN=$pac_cv_memalignval"
    else
        AC_MSG_RESULT(assuming 128 for memory alignment)
        CFLAGS="$CFLAGS -DXFS_MEMALIGN=128"
    fi
fi

if test -n "$file_system_ime"; then
        AC_CHECK_HEADERS(ime_native.h,
                AC_DEFINE(ROMIO_IME,1,[Define for ROMIO with IME])
		[AM_CONDITIONAL([BUILD_AD_IME], [true])
		ROMIO_FS_LIST="${ROMIO_FS_LIST} ime"],
                AC_MSG_NOTICE([IME support requested but cannot find ime_native.h header file])
        )
fi

#
# Verify presence of pvfs2.h
#
if test -n "$file_system_pvfs2"; then
    CPPFLAGS="$CPPFLAGS $CFLAGS"
    AC_CHECK_HEADERS(pvfs2.h,
	AC_DEFINE(ROMIO_PVFS2,1,[Define for ROMIO with PVFS2])
	[AM_CONDITIONAL([BUILD_AD_PVFS2], [true])
	AC_DEFINE(HAVE_PVFS2_SUPER_MAGIC, 1, [Define if PVFS2_SUPER_MAGIC defined.])
	ROMIO_FS_LIST="${ROMIO_FS_LIST} pvfs2"],
        AC_MSG_ERROR([PVFS2 support requested but cannot find pvfs2.h header file])                                        
    )
fi

# layout change after pvfs-2.6.3:
if test -n "$file_system_pvfs2"; then
    AC_COMPILE_IFELSE([
        AC_LANG_SOURCE([
#include <stdlib.h>
#include "pvfs2.h"
          int main(int argc, char **argv) { 
	      PVFS_object_ref ref;
	      PVFS_sys_attr attr;
	      PVFS_sys_create(NULL, ref, attr, NULL, NULL, NULL, NULL); 
	  return 0; }
       ])],
       , AC_DEFINE(HAVE_PVFS2_CREATE_WITHOUT_LAYOUT, 1, 
       		[Define if PVFS_sys_create does not have layout parameter])
       )
fi

AS_IF([test -n "$file_system_gpfs"],
    [SYSDEP_INC=-I${prefix}/include], [SYSDEP_INC=])

AS_IF([test -n "$file_system_gpfs"],
	AC_CHECK_HEADERS([gpfs.h gpfs_fcntl.h])
	AS_IF([test "$ac_cv_header_gpfs_h" = "yes" -o "$ac_cv_header_gpfs_fcntl_h" = "yes"], [
	  AC_SEARCH_LIBS([gpfs_fcntl], [gpfs], [],
	    [AC_MSG_ERROR([Library containing gpfs_fcntl symbols not found])])
	])
	)

if test "x${MPC_MPIIO_ENABLED}" = "xyes"; then
MPC_MPIIO_CONFIGURATION="\
### MPI IO Configuration ###\n\
#\n\
# File Systems.........$ROMIO_FS_LIST\n\
# Lustre Lookahead..... $lustre_lockahead\n\
#\n"
else
MPC_MPIIO_CONFIGURATION=""
fi


#
# MPI IO (romio) END
#

#
# Compute the all MPC libs variable
#

ALL_MPC_LIBS="${ac_pwd}/src/MPC_Config/src/libmpcconfig.la"

if test "x${MPC_THREAD_ENABLED}" = "xyes"; then
	ALL_MPC_LIBS="${ALL_MPC_LIBS} ${ac_pwd}/src/MPC_Launch/src/libmpclaunch.la"
	ALL_MPC_LIBS="${ALL_MPC_LIBS} ${ac_pwd}/src/MPC_Threads/src/libmpcthread.la"
fi

if test "x${MPC_OPENMP_ENABLED}" = "xyes"; then
	ALL_MPC_LIBS="${ALL_MPC_LIBS} ${ac_pwd}/src/MPC_OpenMP/src/libmpcomp.la"
fi

if test "x${MPC_MPI_ENABLED}" = "xyes"; then
	ALL_MPC_LIBS="${ALL_MPC_LIBS} ${ac_pwd}/src/MPC_Message_Passing/src/libmpclowcomm.la"
fi

if test "x${MPC_MPI_ENABLED}" = "xyes"; then
	ALL_MPC_LIBS="${ALL_MPC_LIBS} ${ac_pwd}/src/MPC_MPI/src/libmpcmpi.la"
fi

if test "x${MPC_MPIIO_ENABLED}" = "xyes"; then
	ALL_MPC_LIBS="${ALL_MPC_LIBS} ${ac_pwd}/src/MPC_IO/src/libmpcio.la"
fi


AC_SUBST([ALL_MPC_LIBS])

#
# Configure MPC's compilers
#

MPC_LANGUAGE_LIST=""


MPC_C_COMPILER_LIST=""
MPC_CPP_COMPILER_LIST=""
MPC_FORTRAN_COMPILER_LIST=""

value_is_in_list()
{
        is_in_list="no"

        echo $1 | grep $2 2>&1 > /dev/null

        if test "x$?" = "x0"; then
                is_in_list="yes"
        fi
}


append_to_language_list_if_needed()
{
        value_is_in_list "$MPC_LANGUAGE_LIST" $1

        if test "x$is_in_list" = "xyes"; then
                return
        else
                MPC_LANGUAGE_LIST="$MPC_LANGUAGE_LIST $1 "
        fi

}


compiler_to_lang()
{
        mpc_comp_type=""
        case $1 in
                apcc|gcc|icc|pgcc|clang)
                        mpc_comp_type="c"
                        MPC_C_COMPILER_LIST+=" $1"
                ;;
                ap++|g++|icpc|pgc++|pgcpp|clang++)
                        mpc_comp_type="cxx"
                        MPC_CPP_COMPILER_LIST+=" $1"
                ;;
                apfortran|gfortran|ifort|pgfortran|pgf77|flang)
                        mpc_comp_type="fortran"
                        MPC_FORTRAN_COMPILER_LIST+=" $1"
                ;;
                *)
                        AC_MSG_ERROR([No such compiler type ${1}])
                ;;
        esac

        append_to_language_list_if_needed $mpc_comp_type
}

locate_bin()
{
        locate_bin_path=""
        which $1 2>&1 > /dev/null

        if test "x$?" = "x0"; then
                locate_bin_path=$(which $1)
        fi

}

# Insert all known compiler wrappers for compilers

for prog in gcc g++ gfortran icc icpc ifort pgcc pgc++ pgcpp pgfortran pgf77 clang clang++ flang
do
        AC_MSG_CHECKING([for ${prog}])

        locate_bin $prog

        if test -n "$locate_bin_path"; then
                compiler_to_lang $prog
                MPC_COMPILER_LIST="$MPC_COMPILER_LIST $prog"
                AC_MSG_RESULT([yes])
        else
                AC_MSG_RESULT([no])
        fi

done

at_least_one_privatizing_compiler=""

# Is CC apcc ?
AC_MSG_CHECKING([if CC is apcc])

$CC --ap-help > /dev/null 2>&1

if test "x$?" = "x0"; then
	at_least_one_privatizing_compiler="yes"
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
fi

for prog in apcc ap++ apfortran
do
	AC_MSG_CHECKING([for ${prog}])
	locate_bin $prog

	if test -n "$locate_bin_path"; then
		compiler_to_lang $prog
		MPC_COMPILER_LIST+=" $prog"
		AC_MSG_RESULT([yes])
		at_least_one_privatizing_compiler+="yes"
	else
		AC_MSG_RESULT([no])
	fi
done

if test "x${mpc_privatization_support}" = "xyes"; then
        if test -z "$at_least_one_privatizing_compiler"; then
		AC_MSG_WARN([Consider passing --enable-process-mode to deactivate priv])
                AC_MSG_ERROR([Privatization was enabled but no privatizing compiler was found])
	fi
fi

# Check that we at least have a C compiler

AC_MSG_CHECKING([if we have at least a C compiler for MPC])

if test -n "$MPC_C_COMPILER_LIST"; then
        AC_MSG_RESULT([yes])
else
        AC_MSG_RESULT([no])
        AC_MSG_ERROR([MPC requires at least a c compiler to be in your PATH])
fi

AC_SUBST([MPC_LANGUAGE_LIST])
AC_SUBST([MPC_C_COMPILER_LIST])
AC_SUBST([MPC_CPP_COMPILER_LIST])
AC_SUBST([MPC_FORTRAN_COMPILER_LIST])

MPC_COMPILER_LIST="$MPC_C_COMPILER_LIST $MPC_CPP_COMPILER_LIST $MPC_FORTRAN_COMPILER_LIST"

AC_SUBST([MPC_COMPILER_LIST])

#
# Set autopriv as default compiler if privatization is enabled and autpriv present
#

mpc_have_autopriv="no"

AC_CHECK_PROG([apcc],[apcc],[yes],[no])
AC_CHECK_PROG([apcpp],[ap++],[yes],[no])
AC_CHECK_PROG([apfortran],[apfortran],[yes],[no])

AC_MSG_CHECKING([for autopriv])

if test "x${apcc}" = "xyes" -a "x${apcpp}" = "xyes" -a "x${apfortran}" = "xyes";
then
	# We have autopriv in path
	AC_MSG_RESULT([yes])
	mpc_have_autopriv=yes
else
	AC_MSG_RESULT([no])
fi

AM_CONDITIONAL([MPC_HAVE_AUTOPRIV], [test "x${mpc_have_autopriv}" = "xyes"])


#
# Extraction of compilation state
#

clean_env_path_var()
{
	var_data=$1

	new_data_list=`echo $var_data | sed "s/\\:/\\n/g" | sort | uniq | xargs echo`

	ret=""
	for entry in $new_data_list
	do
		ret="${ret}:${entry}"
	done

	echo $ret
}



CONFIGURE_PATH="`clean_env_path_var $PATH`"
AC_SUBST([CONFIGURE_PATH])
CONFIGURE_LD_LIBRARY_PATH="`clean_env_path_var $LD_LIBRARY_PATH`"
AC_SUBST([CONFIGURE_LD_LIBRARY_PATH])
CONFIGURE_PKG_CONFIG_PATH="`clean_env_path_var $PKG_CONFIG_PATH`"
AC_SUBST([CONFIGURE_PKG_CONFIG_PATH])
CONFIGURE_CPATH="`clean_env_path_var $CPATH`"
AC_SUBST([CONFIGURE_CPATH])
CONFIGURE_LIBRARY_PATH="`clean_env_path_var $LIBRARY_PATH`"
AC_SUBST([CONFIGURE_LIBRARY_PATH])

# Generate Compilation timestamp

date +%s 2>&1 > /dev/null

AC_MSG_CHECKING([Save configure run timestamp ])

MPC_GENERATION_DATE=""

if test "x$?" = "x0"; then
        # Date was sufficient
        MPC_GENERATION_DATE=$(date +%s)
        AC_MSG_RESULT([$MPC_GENERATION_DATE])
else
        # Try in perl who knows
        perl -e 'print time, "\n"' 2>&1 > /dev/null
        if test "x$?" = "x0"; then
                MPC_GENERATION_DATE=$(perl -e 'print time, "\n"')
                AC_MSG_RESULT([$MPC_GENERATION_DATE])
        else
                MPC_GENERATION_DATE=0
                AC_MSG_RESULT([fail assuming '0'])
        fi
fi

AC_SUBST([MPC_GENERATION_DATE])

#
# Generate the Reconfigure Script
#
cat << EOF > ./reconfigure
#!/bin/sh
if test -n "\`find ${ac_abs_confdir}/configure -newer \$0\`"; then
	printf "Warning: Source configure is newer than reconfigure file (./autogen.sh || git check-out)\n"
	printf "Warning: Rebuilding over a different version of MPC can lead to unexpected errors.\n"
fi
if test -f "${PWD}/src/bin/mpc_build_env.sh"; then
        . ${PWD}/src/bin/mpc_build_env.sh
fi

${ac_abs_confdir}/configure $ac_configure_args "\$@"

EOF

chmod +x ./reconfigure


# Generate configuration summary

MPC_CONFIG_RESULT="\
########## MPC Configuration Script ##########\n\
# Installation into.... $prefix\n\
# Configure run on..... `date -d @$MPC_GENERATION_DATE`\n\
# Version built........ $MPC_VERSION_STRING\n\
# C Compiler........... $CC\n\
# C++ Compiler......... $CXX\n\
# Fortran Compiler..... $FC\n\
#\n\
### BUILD CONFIGURATION ###\n\
#\n\
# Architecture......... $MPC_TARGET_ARCHITECTURE\n\
# Optimizations........ $mpc_enable_opt ($MPC_OPT_FLAGS)\n\
# Native build......... $mpc_enable_native_opt ($MPC_NATIVE_FLAGS)\n\
# Microtask arch....... $MPC_ARCH_MICROTASK_ARCH\n\
# Context switch....... $MPC_ARCH_CONTEXT_SWITCH\n\
#\n\
### OPTIONAL BUILT MODULE ###\n\
#\n\
# MPC_Thread........... $MPC_THREAD_ENABLED\n\
# MPC_Message_passing.. $MPC_MESSAGE_PASSING_ENABLED\n\
# MPC_MPI.............. $MPC_MPI_ENABLED\n\
# MPC_MPIIO............ $MPC_MPIIO_ENABLED\n\
#\n\
### COMPILERS ###\n\
#\n\
# Privatization........ $mpc_privatization_support\n\
# Process mode......... $mpc_in_process_mode\n\
# Languages............ $MPC_LANGUAGE_LIST\n\
# Compilers............ $MPC_COMPILER_LIST\n\
#\n\
### NETWORKS ###\n\
#\n\
# Supported list....... $MPC_NETWORK_LIST\n\
# CMA over SHM......... $mpc_enable_cma\n\
#\n$MPC_MPIIO_CONFIGURATION\
### ACCELERATORS ###\n\
#\n\
# Cuda support......... $cuda_enabled\n\
#\n\
### OTHER OPTIONS SET ###\n\
#\n\
# Launcher............. $MPC_LAUNCHER\n\
# Allocator............ $mpc_allocator\n\
# libunwind............ $mpc_libunwind\n\
# DMTPC c/r............ $MPC_HAVE_DMTCP\n\
#\n\
### DEBUGGING ###\n\
#\n\
# Embedded profiler.... $MPC_HAVE_PROFILER\n\
# Debug symbols........ $mpc_debug ($MPC_DEBUG_FLAGS)\n\
# Debug messages....... $mpc_debug_messages\n\
# Colored output....... $mpc_shell_color\n\
#\n\
#############################################\n"

AC_SUBST([MPC_CONFIG_RESULT])

AC_SUBST([PKG_DATADIR], ["${prefix}/share/${PACKAGE_TARNAME}/"])
AC_SUBST([PKG_INCDIR], ["${prefix}/include/${PACKAGE_TARNAME}/"])

#
# Final Output
#

AC_CONFIG_HEADERS([mpc_config.h])

AC_OUTPUT([Makefile
	   src/MPC_Arch/include/mpc_arch.h
	   src/Makefile
	   src/lib/Makefile
           src/MPC_Doc/Makefile
           src/MPC_Arch/Makefile
	   src/MPC_Arch/src/Makefile
           src/MPC_Common/Makefile
	   src/MPC_Common/src/Makefile
           src/MPC_Config/Makefile
	   src/MPC_Config/src/Makefile
           src/MPC_Launch/Makefile
	   src/MPC_Launch/src/Makefile
           src/MPC_Message_Passing/Makefile
	   src/MPC_Message_Passing/src/Makefile
           src/MPC_MPI/Makefile
	   src/MPC_MPI/src/Makefile
           src/MPC_OpenMP/src/Makefile
	   src/MPC_OpenMP/Makefile
           src/MPC_Topology/Makefile
	   src/MPC_Topology/src/Makefile
           src/MPC_Threads/Makefile
	   src/MPC_Threads/src/Makefile
	   src/MPC_IO/Makefile
	   src/MPC_IO/src/Makefile
	   src/MPC_IO/include/mpio.h
	   src/include/mpi.h
	   src/MPC_IO/src/romio/Makefile
           src/include/mpc_threads_config.h
           src/bin/Makefile
	   src/bin/mpc_build_env.sh
           src/bin/mpc_compiler_manager.sh
           src/bin/mpc_cc.sh
           src/bin/mpc_cflags
           src/bin/mpc_ldflags
           src/bin/mpc_cxx.sh
	   src/bin/mpc_nvcc.sh
           src/bin/mpc_f77.sh
           src/bin/mpcrun.sh
	   src/bin/mpcvars.sh
	   src/bin/mpc_status.sh])



printf "\n$MPC_CONFIG_RESULT"

if test "x${MPC_CONFIG_RELOADED}" = "xyes"; then

echo "************ IMPORTANT NOTICE ****************"
echo "MPCs configure realized that you already ran"
echo "a configure in this directory and restored the"
echo "previous build environment. If it is not what"
echo "you wanted please manually reconfigure MPC"
echo "***********************************************"

fi
