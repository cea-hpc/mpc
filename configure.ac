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

#####################################################################
####                    TABLE OF CONTENTS                        ####
#####################################################################
# This configure.ac is organized as follows:
#(you may use tags below to reach the proper section directly)
#
# - {sec:setup}: GENERAL INITIALIZATION
# - {sec:globals}: GLOBAL INFORMATION
# - {sec:version}: BUILD VERSION IDENTIFICATION
# - {sec:checks}: CHECKS BEFORE BUILD, including
#     - {sec:arch}: ARCHITECTURE
#     - {sec:programs} : EXTERNAL PROGRAMS
#     - {sec:headers}: EXTERNAL HEADERS
#     - {sec:libraries}: EXTERNAL LIBRARIES
#     - {sec:datatypes}: DATA TYPES
#     - {sec:funcs}: STANDARD FUNCTIONS
# - {sec:deps}: LOOKING FOR DEPENDENCIES, including...
# - {sec:pthread}: PThread API checks
# - {sec:modular}: MPC MODULAR CONFIGURATION
# - {sec:compiler}: COMPILERS IDENTIFICATION
# - {sec:finalize}: FINALIZATION & SUMMARY
#
#####################################################################

####################################
###### GENERAL INITIALIZATION ###### {sec:setup}
####################################

#Autoconf initialization w/ project name, version, bug report email, package name & website
AC_INIT([Multi-Processor Computing Framework],
        [m4_esyscmd_s([./utils/get_version])],
        [france@paratools.com], [mpcframework], [https://mpc.hpcframework.com])


# The first check is that the configure was generated with
# autogen.sh and not manually for example with autoconf
# it is crucial to ensure that configuration patches (located in ./config/conf_patches/)
# are applied before running the configure script for a patch
# description see the end of the autogen.sh file
GENERATED_WITH_AUTOGEN=$(tail -n 1 "$0" | grep -c "MPC_AUTOGEN_WAS_USED")

if test "x${GENERATED_WITH_AUTOGEN}" = "x0"; then
        AC_MSG_ERROR([The configure script was not generated using autogen.sh])
fi


#Some basic setups: where build configuration/macro files are stored
#and check for source directory properly found
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIRS([config/m4])
AC_CONFIG_SRCDIR([src/MPC_Arch/src/timer.c])

# Language is C-based (init compilers properly)
AC_LANG(C)

# Pre-required maintainer tool to rebuild.
# Enforcing this ensure mimnimum changes to
# generated files under version control
AC_PREREQ(2.69)

# two first arguments are omitted: PACKAGE & VERSION (infered from AC_INIT)
# subdir-objects: any source file will map with a corresponding object file under same subtree from builddir
# foreign: not conformant with GNU standard strictness
# no-define: do not subst PACKAGE & VERSION (inferred from AC_INIT)
# std-options: Check any installed script provides a '--help' and '--version'
# no-dist: do not distribute the project through "make dist", MPC more complex than what provided
AM_INIT_AUTOMAKE([1.15 subdir-objects foreign no-define std-options])

# Avoid Makefile to print commands before running it
# Overriden by configure: --disable-silent-rules
# overriden by make: make V=1
AM_SILENT_RULES([yes])

# Editing the build system (new directory, source file, changes in *.am/*.ac)
# leads to regenerate the configuration. To do so, run "./autogen.sh" script.
# by using `--enable-maintainer-mode` as configure option, a change in build files
# will automatically trigger a new configuration (not working properly for now)
AM_MAINTAINER_MODE([disable])

#libtool initialization
LT_PREREQ([2.4.6])
LT_INIT([disable-static])
LT_LANG([C])

# TODO:
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

################################
###### GLOBAL INFORMATION ###### {sec:globals}
################################

if test "x${prefix}" = "xNONE"; then
        AC_MSG_WARN([No prefix was passed will use /usr/local/])
        prefix="/usr/local/"
fi

# installation prefix, defined for the whole build
AC_DEFINE_UNQUOTED([MPC_PREFIX_PATH], ["$prefix"], [Prefix Path for MPC])
#static list of supported architecture (to be expanded)
MPC_SUPPORTED_ARCHITECTURES_LIST="x86_64 ia64 i686 arm aarch64 powerpc64"
# General Configuration path
CONFIGURE_SCRIPT_PATH=$(dirname "`readlink -f "$0"`")

# Proceed to prefix checks

# Note that we used to prevent reinstall but it led to several overheads
# for user we then propose to leave the warning but let the code go without
# complaining. In particular this allows maintainer-mode to work cleanly
# and it also allows direct reconfiguring (with the ./reconfigure scrip
# generated in the buildir). This is then a tradeoff, changing MPC's
# configuration is sill a possible souce of error.

if test -f "${prefix}/bin/mpc_status"; then
        AC_MSG_NOTICE([!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!])
        AC_MSG_NOTICE([There seems to be a previous install of MPC in ${prefix}.])
        AC_MSG_NOTICE([Reinstalling over it can lead to spurious errors if configuration changed.])
        AC_MSG_NOTICE([It is recommended to remove MPC beforehand to avoid conflicts.])
        AC_MSG_NOTICE([PROCEED at your OWN RISKS !!])
        AC_MSG_NOTICE([!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!])
        AC_MSG_WARN([You have 5 seconds to abort.])
        sleep 5
fi

####################################
###### VERSION IDENTIFICATION ###### {sec:version}
####################################

#Create environment vars

#
# Compute version
#
MPC_VERSION_MAJOR="$($CONFIGURE_SCRIPT_PATH/utils/get_version --major)"
MPC_VERSION_MINOR="$($CONFIGURE_SCRIPT_PATH/utils/get_version --minor)"
MPC_VERSION_PATCH="$($CONFIGURE_SCRIPT_PATH/utils/get_version --patch)"
MPC_VERSION_TAINT="$($CONFIGURE_SCRIPT_PATH/utils/get_version --taint)"
MPC_VERSION_STRING="$($CONFIGURE_SCRIPT_PATH/utils/get_version --all)"

# define macros in mpc_config.h for proper use from sources
AC_DEFINE_UNQUOTED([MPC_VERSION_MAJOR], [$MPC_VERSION_MAJOR], [MPC Major Version])
AC_DEFINE_UNQUOTED([MPC_VERSION_MINOR], [$MPC_VERSION_MINOR], [MPC Minor Version])
AC_DEFINE_UNQUOTED([MPC_VERSION_PATCH], [$MPC_VERSION_PATCH], [MPC Patch Version])
AC_DEFINE_UNQUOTED([MPC_VERSION_PRE], ["$MPC_VERSION_TAINT"], [MPC Tainted Version])
AC_DEFINE_UNQUOTED([MPC_VERSION_STRING], ["$MPC_VERSION_STRING"], [MPC Tainted Version])

#Substitute any @MPC_VERSION_*@ in *.in files
AC_SUBST([MPC_VERSION_MAJOR])
AC_SUBST([MPC_VERSION_MINOR])
AC_SUBST([MPC_VERSION_PATCH])
AC_SUBST([MPC_VERSION_PRE])
AC_SUBST([MPC_VERSION_STRING])

#################################
###### CHECKS BEFORE BUILD ###### {sec:checks}
#################################

#### ARCHITECTURE #### {sec:arch}
######################
AC_CANONICAL_HOST
MPC_TARGET_ARCHITECTURE="$host_cpu"

# Check if architecture is in support list
AC_MSG_CHECKING([Support for target architecture])
if test -z "${MPC_SUPPORTED_ARCHITECTURES_LIST##*$MPC_TARGET_ARCHITECTURE*}" ; then
        AC_MSG_RESULT([$MPC_TARGET_ARCHITECTURE])
        AC_SUBST([MPC_TARGET_ARCHITECTURE])
else
        AC_MSG_RESULT([No])
        AC_MSG_ERROR([Current architecture is not supported yet. Please contact support team])
fi

#### EXTERNAL PROGRAMS #### {sec:programs}
###########################
AC_PROG_CC([gcc apcc icc pgcc clang])
AC_PROG_CXX([g++ ap++ icpc pg++ pgcpp clan++])
AC_PROG_FC([gfortran apfortran ifort pgfortran pgf77 flang])

AC_PROG_INSTALL
AM_PROG_AS
AC_PROG_LN_S
AC_PROG_MKDIR_P

AM_PATH_PYTHON([],[HAVE_PYTHON="yes"],[HAVE_PYTHON="no"])

if test "x${HAVE_PYTHON}" = "xyes"; then
        AX_PYTHON_MODULE([json])
        AX_PYTHON_MODULE([argparse])
fi

AM_CONDITIONAL([HAVE_PYTHON], [test "x$HAVE_PYTHON" = "xyes" -a "x${HAVE_PYMOD_JSON}" = "xyes" -a "x${HAVE_PYMOD_ARGPARSE}" = "xyes"])

# Optional Pandoc Generator (dynamically regen manpages
AC_PATH_PROG([PANDOC],[pandoc],[:])
AM_CONDITIONAL([CAN_USE_PANDOC], [test "x$PANDOC" != "x:"])

#### EXTERNAL HEADERS #### {sec:headers}
##########################
AC_HEADER_STDC
AC_CHECK_HEADER_STDBOOL
AC_CHECK_HEADERS([ aio.h \
        asm/unistd.h \
        assert.h \
        ctype.h \
        errno.h \
        execinfo.h \
        elf.h \
        fcntl.h \
        getopt.h \
        hwloc.h \
        immintrin.h \
        limits.h \
        linux/personality.h \
        netdb.h \
        netinet/in.h \
        netinet/tcp.h \
        pthread.h \
        regex.h \
        sched.h \
        semaphore.h \
        setjmp.h \
        signal.h \
        stddef.h \
        stdlib.h \
        stdint.h \
        stdbool.h \
        string.h \
        sys/mman.h \
        sys/param.h \
        sys/pstat.h \
        sys/socket.h \
        sys/stat.h \
        sys/syscall.h \
        sys/time.h \
        sys/types.h \
        sys/ucontext.h \
        sys/un.h \
        sys/utsname.h \
        sys/wait.h \
        time.h \
        ucontext.h \
        unistd.h \
        wchar.h \
        valgrind/memcheck.h \
        valgrind/valgrind.h \
])

#### EXTERNAL LIBRARIES #### {sec:libraries}
############################

AC_CHECK_LIB([m],[log])
AC_CHECK_LIB(pthread, pthread_create)
AC_CHECK_SIZEOF(pthread_t,,[#include <pthread.h>])


#### DATATYPES #### {sec:datatypes}
###################

# Configure subtitutions for mpc_common_types.h
AC_CHECK_HEADER([stdbool.h],[HAVE_STDBOOL_H=1],[HAVE_STDBOOL_H=0])
AC_SUBST([HAVE_STDBOOL_H], [$HAVE_STDBOOL_H])

AC_CHECK_HEADER([stdint.h],[HAVE_STDINT_H=1],[HAVE_STDINT_H=0])
AC_SUBST([HAVE_STDINT_H], [$HAVE_STDINT_H])

AC_CHECK_HEADER([wchar.h],[HAVE_WCHAR_H=1],[HAVE_WCHAR_H=0])
AC_SUBST([HAVE_WCHAR_H], [$HAVE_WCHAR_H])

# Configure substitutions for mpc_thread_config.h

AC_CHECK_HEADER([semaphore.h],[HAVE_SEMAPHORE_H=1],[HAVE_SEMAPHORE_H=0])
AC_SUBST([HAVE_SEMAPHORE_H], [$HAVE_SEMAPHORE_H])

AC_CHECK_HEADER([pthread.h],[HAVE_PTHREAD_H=1],[HAVE_PTHREAD_H=0])
AC_SUBST([HAVE_PTHREAD_H], [$HAVE_PTHREAD_H])

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

AC_CHECK_TYPES([__float128])

AX_GCC_BUILTIN([__builtin_return_address])
AX_GCC_BUILTIN([__builtin_frame_address])
AX_GCC_BUILTIN([__builtin_expect])

if test "x${ax_cv_have___builtin_expect}" = "xyes"; then
        HAVE_BUILTIN_EXPECT=1
else
        HAVE_BUILTIN_EXPECT=0
fi

AC_SUBST([HAVE_BUILTIN_EXPECT])

AC_C_CONST
AC_C_RESTRICT
AC_C_VOLATILE
AC_C_INLINE

# Check for regular TLS support
AX_TLS([AC_DEFINE([TLS_SUPPORT], [1], [TLS extension supported by compiler])],
       [AC_DEFINE([TLS_SUPPORT], [0], [TLS extension NOT supported by compiler])])

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

#### EXTERNAL FUNCTIONS #### {sec:funcs}
############################
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memcpy memmove memset statfs])
AC_CHECK_FUNCS([strcasecmp strchr strdup strerror strstr strtol snprintf vsnprintf])
AC_CHECK_FUNCS([gethostbyname gethostname])
AC_CHECK_FUNCS([gettimeofday])
AC_CHECK_FUNCS([mmap mremap munmap posix_memalign memalign])
AC_CHECK_FUNCS([setenv getenv])
AC_CHECK_FUNCS([uname])
AC_CHECK_FUNCS([regcomp])
AC_CHECK_FUNCS([socket connect accept])
AC_CHECK_FUNCS([dup2 fdatasync])
AC_CHECK_FUNCS([makecontext setcontext getcontext swapcontext])
AC_CHECK_SIZEOF(sigset_t,,[#include <signal.h>])




######################################
###### LOOKING FOR DEPENDENCIES ###### {sec:deps}
######################################

# Rpath helper
#
# $1: LDFLAGS to inspect
#
promote_to_rpath()
{
        # In MPC in order to fortify link/runtime dependencies
        # we prefer to add rpath as much as possible to be
        # more independent from the build env (closer to static build)
        # the role of this helper is to look for libdirs and 
        # add them as rpaths (if not already present)

        # Remove space in GCC's library search path args
        CURRENT_RPATH=""

        # Extract existing RPATH
        for arg in $@
        do
                if echo "$arg" | grep "^\\-Wl,\\-rpath=" > /dev/null; then
                        # Ensure path exists
                        RPATH=$(echo "$arg" | sed s/^-Wl,-rpath=//g)
                        if test -d "${RPATH}"; then
                                CURRENT_RPATH="${CURRENT_RPATH}:$RPATH"
                        fi
                fi
        done

        NEW_RPATH=""

        # Extract -L args
        for arg in $@
        do
                if echo "$arg" | grep "^-L" > /dev/null; then
                        arg=$(echo "$arg" | sed "s/-L /-L/g")
                        # Ensure path exists
                        LPATH=$(echo "$arg" | sed s/^-L//g)
                        if test -d "${LPATH}"; then
                                CNT=$(echo "$CURRENT_RPATH" | grep -c "${LPATH}")
                                #If path not in rpath then emit it
                                if test "x${CNT}" = "x0"; then
                                        NEW_RPATH="${NEW_RPATH} -Wl,-rpath=${LPATH}"
                                fi
                        fi
                fi
        done

        echo "$NEW_RPATH $@"
}
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
                        LDFLAGS=$(promote_to_rpath " -L$pref/$suffix/ -Wl,-rpath=$pref/$suffix/ -l$1 $LDFLAGS")

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

# Add a pkg-config search path if not already here
# $1 : search path to add
#
append_to_pkg_config_if_needed()
{
        TSOURCE=$(echo "$1" | xargs)
        for p in $(echo "$PKG_CONFIG_PATH" | sed "s/:/ /g")
        do
                TPATH=$(echo   "$p" | xargs)
                if test "x${TSOURCE}" = "x${TPATH}"; then
                        return
                fi
        done

        AC_MSG_NOTICE([${TSOURCE} prepended to pkg-config search paths])
        export PKG_CONFIG_PATH=${TSOURCE}:$PKG_CONFIG_PATH
}

#
# $1 header_name (ex. hwloc.h)
# $2 libname     (ex. hwloc)
# $3 looked up function (ex. hwloc_topology_init)
# $4 LIBCODE (ex XML, HWLOC, ...)
# $5 optionnal library version (need pkg-config)
# $6 optionnal lib_prefix
# $7 optionnal alternative lib name (when different from pkg_config)
#
AC_DEFUN([MPC_DETECT_DEPENDENCY],
         [
                AS_VAR_PUSHDEF([dep_found], [mpc_dep_found])
                AC_ARG_WITH([$2],
                [AS_HELP_STRING([--with-$2],
                                [Provide prefix for $2])],
                [whole_prefix=$withval],[whole_prefix="/usr/"])

                inc_prefix=""
                AC_ARG_WITH([$2-inc],
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
                alt_libname=$7
                function_name=$3
                var_prefix=$4
                version="$5"

                if test -z "$lib_prefix"
                then
                        lib_prefix="$whole_prefix/lib"
                fi

                if test -d "${lib_prefix}/pkgconfig"; then
                        append_to_pkg_config_if_needed "${lib_prefix}/pkgconfig"
                fi

                if test -n "$whole_prefix"
                then
                        if test -d "${whole_prefix}/lib/pkgconfig"; then
                                append_to_pkg_config_if_needed "${whole_prefix}/lib/pkgconfig"
                        fi
                fi

                if test -z "$inc_prefix"
                then
                        inc_prefix="$whole_prefix/include/"
                fi

                AC_SUBST([$4_CFLAGS])
                AC_SUBST([$4_LIBS])

                #echo "PREF : $whole_prefix LIB: $lib_prefix INC: $inc_prefix"

                # First try with PKG config
                AC_MSG_NOTICE([Locating $libname$version with pkg-config])
                PKG_CHECK_MODULES([$4], [$libname $version], [pkg_found=1], [pkg_found=0])

                if test "x$pkg_found" = "x1"
                then
                        # Ensure PKG config dependencies are also
                        # injected using RPATH
                        $4_LIBS=$(promote_to_rpath ${$4_LIBS})
                        AC_MSG_NOTICE([$4 FLAGS are ${$4_LIBS}])

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

                        if test -n "${alt_libname}"; then
                                libname="${alt_libname}"
                        fi

                        __check_or_infer_prefix_lib "$libname" "$function_name" "$lib_prefix" "$var_prefix"

                        if test "x$check_or_infer_prefix_include" = "xyes" -a "x$check_or_infer_prefix_lib" = "xyes"
                        then
                                check_or_infer_prefix_result="yes"
                        else
                                check_or_infer_prefix_result="no"
                        fi
                fi

                AC_MSG_CHECKING([$2 cflags])
                XXTMP=$(eval echo \${$4_CFLAGS})
                AC_MSG_RESULT([$XXTMP])

                AC_MSG_CHECKING([$2 ldflags])
                XXTMP=$(eval echo \${$4_LIBS})
                AC_MSG_RESULT([$XXTMP])

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

MPC_DETECT_DEPENDENCY([hwloc.h],[hwloc], [hwloc_topology_init], [HWLOC], [> 2.0.0])
MPC_REQUIRED_DEPENDENCY([hwloc], [$mpc_dep_found])

MPC_DETECT_DEPENDENCY([opa_config.h],[openpa], [OPA_Queue_init], [OPA], [], [], [opa])
MPC_REQUIRED_DEPENDENCY([openpa], [$mpc_dep_found])


#
# Detect MPC Allocator
#


AC_ARG_ENABLE([mpcalloc],
[AS_HELP_STRING([--enable-mpcalloc],
                [If the MPC allocator is to be used])],
[use_mpcalloc=$enableval],
[use_mpcalloc=maybe])

MPC_ALLOCATOR="Standard"
# Default preload
MPC_ALLOCATOR_PRELOAD="libmpcalloc.so"

if test ! "x${use_mpcalloc}" = "xno"; then

        MPC_DETECT_DEPENDENCY([mpcalloc.h], [mpcalloc], [sctk_malloc], [ALLOC])
        if test "x$mpc_dep_found" = "xyes"; then
                AC_DEFINE([MPC_Allocator], [1], [mpcalloc support is enabled])
                MPC_ALLOCATOR="mpcalloc"
                # Locate the MPC Allocator libdir to provide
                # a complete preload path
                ALLOC_PRELOAD_BIN="$(which mpcalloc_preload)"
                if test ! -z "$ALLOC_PRELOAD_BIN"; then
                # Locate PREFIX
                ALLOC_PREFIX=$(echo "$ALLOC_PRELOAD_BIN" | rev | cut -d / -f 3- | rev)
                CANDIDATE_ALLOC_LIB_PATH="${ALLOC_PREFIX}/lib/libmpcalloc.so"
                if test -e "${CANDIDATE_ALLOC_LIB_PATH}"; then
                        MPC_ALLOCATOR_PRELOAD="${CANDIDATE_ALLOC_LIB_PATH}"
                fi
                fi
        else
                if test  "x${use_mpcalloc}" = "xyes"; then
                        AC_MSG_ERROR([Could not locate MPC allocator which was explicitly enabled])
                fi
        fi


fi

AC_SUBST([MPC_ALLOCATOR])
AC_SUBST([MPC_ALLOCATOR_PRELOAD])

#
# Detect libffi / Active Message support
#

AC_ARG_ENABLE([am],
[AS_HELP_STRING([--enable-am],
                [Compile MPC active messages])],
[MPC_AM_ENABLED=$enableval],
[MPC_AM_ENABLED=maybe])

MPC_HAVE_AM="no"

MPC_DETECT_DEPENDENCY([ffi.h], [libffi], [ffi_prep_cif], [FFI])
HAVE_LIB_FFI="no"

if test "x$mpc_dep_found" = "xyes"; then
        AC_DEFINE([MPC_HAVE_LIBFFI], [1], [libffi support is enabled])

        # Enable AM if not explicitly disabled
        if test "x$MPC_AM_ENABLED" != "xno"; then
                MPC_AM_ENABLED=yes
        fi

        MPC_HAVE_AM="$MPC_AM_ENABLED"
        HAVE_LIB_FFI="yes"
else
        # Ensure AM were not REQUIRED as we do not have libffi
        if test "x$MPC_AM_ENABLED" = "xyes"; then
                AC_MSG_ERROR([Could not libffi required for Active Message support (--enable-am)])
        fi
        # Not possible to support AM
        MPC_AM_ENABLED="no"
fi

AC_MSG_CHECKING([Support for active messages])
AC_MSG_RESULT([$MPC_HAVE_AM])

AM_CONDITIONAL([MPC_HAVE_AM], [test "x$MPC_HAVE_AM" = "xyes"])

#
# Detect libvmem
#
mpc_libvmem="no"

# First use the pkgconfig naming scheme
MPC_DETECT_DEPENDENCY([libvmem.h], [libvmem], [vmem_create], [VMEM])

if test "x$mpc_dep_found" = "xyes"; then
        AC_DEFINE([MPC_HAVE_LIBVMEM], [1], [libvmem support is enabled])
        mpc_libvmem="yes"
fi

AC_MSG_CHECKING([Building libvmem support])
AC_MSG_RESULT([$mpc_libvmem])

#
# Detect libunwind
#
mpc_libunwind="no"

# First use the pkgconfig naming scheme
MPC_DETECT_DEPENDENCY([libunwind.h], [libunwind], [unw_init_local], [UNW])

if test "x$mpc_dep_found" = "xyes"; then
        AC_DEFINE([MPC_HAVE_LIBUNWIND], [1], [libunwind support is enabled])
        mpc_libunwind="yes"
fi

AC_MSG_CHECKING([Building libunwind support])
AC_MSG_RESULT([$mpc_libunwind])

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
# Detect LibFabric
#

ofi_enabled="no"

MPC_DETECT_DEPENDENCY([fabric.h], [libfabric], [ofi_wait_init], [OFI], [], [], [fabric])
if test "x$mpc_dep_found" = "xyes"; then
        MPC_NETWORK_LIST="$MPC_NETWORK_LIST ofi"
        AC_DEFINE([MPC_USE_OFI], [1], [Libfabric support is enabled])
        ofi_enabled="yes"
fi

AM_CONDITIONAL([OFI_DRIVER_ENABLED], [test "x$ofi_enabled" = "xyes"])

AC_SUBST([MPC_NETWORK_LIST])

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

# WITH SLURM is autodetect for either PMI1 or PMIx

AC_ARG_WITH([slurm],
[AS_HELP_STRING([--with-slurm],
                [Rely on SLURM-based PMI1 same as --with-pmi1])],
[mpc_hydra_enabled=no;detect_pmi_version=$withval],
[detect_pmi_version=no])

if test "x${detect_pmi_version}" = "xyes"; then
        mpc_hydra_enabled=no
        ERRMSG="Could not detect PMI type using '--with-slurm' please specify manually either '--with-pmi1' or '--with-pmix'"
        # Do we have srun ?
        if command -v "srun" > /dev/null 2>&1; then
                if srun --mpi=list 2>&1 | grep "pmix"; then
                        mpc_pmix_enabled="yes"
                        mpc_pmi1_enabled="no"
                elif srun --mpi=list 2>&1 | grep "pmi2"; then
                        mpc_pmi1_enabled="yes"
                        mpc_pmix_enabled="no"
                else
                        AC_MSG_ERROR([${ERRMSG}])
                fi
        else
                AC_MSG_ERROR([${ERRMSG}])
        fi
else
        # No definitive decision leave defaults
        mpc_pmi1_enabled="no"
        mpc_pmix_enabled="no"
fi

AC_ARG_WITH([pmi1],
[AS_HELP_STRING([--with-pmi1],
                [Rely on PMI1 (PMI1) same as --with-slurm])],
[mpc_hydra_enabled=no;mpc_pmi1_enabled=$withval],
[])

AC_ARG_WITH([pmix],
[AS_HELP_STRING([--with-pmix],
                [Compile MPC with pmix support (requires an external pmix capable launcher)])],
[mpc_hydra_enabled=no;mpc_pmix_enabled=$withval],
[])

ensure_only_one_launcher()
{
        LAUNCHER_COUNT=0
        test "x$mpc_hydra_enabled" != "xno" && LAUNCHER_COUNT=$(($LAUNCHER_COUNT + 1))
        test "x$mpc_pmi1_enabled" != "xno" && LAUNCHER_COUNT=$(($LAUNCHER_COUNT + 1))
        test "x$mpc_pmix_enabled" != "xno" && LAUNCHER_COUNT=$(($LAUNCHER_COUNT + 1))

        if test "x$LAUNCHER_COUNT" != "x1"; then
                AC_MSG_NOTICE([=================================])
                AC_MSG_NOTICE([HYDRA Enabled: $mpc_hydra_enabled])
                AC_MSG_NOTICE([PMI1 Enabled: $mpc_pmi1_enabled])
                AC_MSG_NOTICE([PMIX Enabled: $mpc_pmix_enabled])
                AC_MSG_NOTICE([=================================])
                AC_MSG_NOTICE([--with-slurm can try to detect between pmix and pmi1])
                AC_MSG_ERROR([You must pick up one launcher between Hydra, PMI1 and PMIX (--with-hydra or --with-pmi1 or --with-pmix)])
        fi
}

ensure_only_one_launcher

MPC_LAUNCHER_PROGRAM="none"

if test "x$mpc_pmi1_enabled" != "xno"; then
        MPC_LAUNCHER="PMI1"

        MPC_DETECT_DEPENDENCY([pmi.h],[pmi], [PMI_Finalize], [PMI])
        MPC_REQUIRED_DEPENDENCY([pmi], [$mpc_dep_found])
        AC_DEFINE([MPC_USE_PMI1], [1], [PMI1 will be used to launch parallel programs])
elif test "x$mpc_pmix_enabled" != "xno"; then
        MPC_LAUNCHER="PMIX"
        MPC_DETECT_DEPENDENCY([pmix.h],[pmix], [PMIx_Finalize], [PMIX])
        MPC_REQUIRED_DEPENDENCY([pmix], [$mpc_dep_found])
        AC_DEFINE([MPC_USE_PMIX], [1], [PMIX will be used to launch parallel programs])

        # In the PMIX configuration check for the PMIx Runtime
        AC_PATH_PROG([PRTERUN], [prterun], [no])
        
        if test "x$PRTERUN" = "xno"
	then
		# Try with the PRUN name if prterun is not found
        	AC_PATH_PROG([PRUN], [prun], [no])

		if test "x${PRUN}" != "xno"
		then
			PRTERUN="${PRUN}"
		fi
	fi
	
	if test "x$PRTERUN" = "xno"
        then
                AC_MSG_WARN([Could not locate 'PMIx Runtime PRTE' in PATH. support will not be built.])
        else
                MPC_LAUNCHER_PROGRAM="PRRTE"
                AC_DEFINE([MPC_USE_PRTE], [1], [PMIx Runtime PRTE will be considered to launch parallel programs])
        fi

elif test "x$mpc_hydra_enabled" != "xno"; then
        AC_PATH_PROG([MPIEXEC_HYDRA], [mpiexec.hydra], [no])
        if test "x$MPIEXEC_HYDRA" = "xno"
        then
                AC_MSG_ERROR([Could not locate Hydra wrapper 'mpiexec.hydra' in PATH. Please reconsider using this launcher.])
        fi
        # Here we look for hydra only if hydra has not been disabled
        MPC_LAUNCHER="HYDRA"

        AC_CHECK_FUNCS([socket setsockopt gethostbyname],[can_access_hydra_pmi_socket=no], [can_access_hydra_pmi_socket=yes])

        if test "x$can_access_hydra_pmi_socket" = "xno" ; then
                AC_DEFINE(USE_PMI_PORT,1,[HYDRA supports sockets])
        else
                AC_MSG_ERROR([Hydra requires socket support])
        fi

        MPC_LAUNCHER_PROGRAM="HYDRA"
        AC_DEFINE([MPC_USE_HYDRA], [1], [Hydra will be used to launch parallel programs])
else
        AC_MSG_ERROR([No launcher was selected use either --with-slurm, --with-hydra or --with-pmix])
fi

# For the srun launcher we want srun and one of the PMI compiled in

AC_PATH_PROG([SRUN_IN_PATH], [srun], [no])

SRUN_LAUNCH_SUPPORT="no"

if test "x${SRUN_IN_PATH}" != "xno"; then
        AC_MSG_CHECKING([SRUN launcher deployed])
        if test "x$MPC_LAUNCHER" = "xPMIX" -o "x$MPC_LAUNCHER" = "xPMI1"; then
                AC_MSG_RESULT([yes])
                SRUN_LAUNCH_SUPPORT="yes"
        else
                AC_MSG_RESULT([no])
        fi
fi

AM_CONDITIONAL([MPC_HAS_SRUN_SUPPORT], [test "x${SRUN_LAUNCH_SUPPORT}" = "xyes"])
AM_CONDITIONAL([HYDRA_LAUNCHER_ENABLED], [test "x$MPC_LAUNCHER_PROGRAM" = "xHYDRA"])
AM_CONDITIONAL([PRRTE_LAUNCHER_ENABLED], [test "x$MPC_LAUNCHER_PROGRAM" = "xPRRTE"])

AC_SUBST([PRTERUN])
AC_SUBST([MPIEXEC_HYDRA])
AC_SUBST([MPC_LAUNCHER])
AC_SUBST([MPC_LAUNCHER_PROGRAM])


AC_DEFINE_UNQUOTED([MPC_LAUNCHER], [$MPC_LAUNCHER], [Default launcher configured for MPC])

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


# Checks for optionnal headers

#TODO CHECK IB
#TODO CHECK WHY libunwind.h
#TODO CHECK PMI
#TODO CHECK STDBOOL
#TODO CHECK VALGRIND

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

AX_GCC_FUNC_ATTRIBUTE(visibility)

if test "x$ax_cv_have_func_attribute_visibility" = "xyes"; then
        AC_DEFINE([HAVE_VISIBILITY],[1],[Visibility attribute supported])
        HAVE_VISIBILITY=1
        AC_SUBST([HAVE_VISIBILITY])
fi

AX_GCC_FUNC_ATTRIBUTE(weak)
AX_GCC_FUNC_ATTRIBUTE(unused)

AX_GCC_FUNC_ATTRIBUTE(constructor)

if test "x$ax_cv_have_func_attribute_constructor" != "xyes"; then
        AC_MSG_ERROR([MPC relies on constructors for its initialization])
fi
extract_integer_define_value "signal.h" "NSIG" "no"

if test -z "$MACRO_VALUE"; then
        extract_integer_define_value "signal.h" "_NSIG" "no"
        if test -z "$MACRO_VALUE"; then
                MACRO_VALUE="32"
        fi
fi

AC_DEFINE_UNQUOTED([MPC_CONFIG_NSIG], [$MACRO_VALUE], [Value of NSIG from signal.h])
AC_SUBST([MPC_CONFIG_NSIG], [$MACRO_VALUE])

###################################
###### PTHREAD API DETECTION ###### {sec:pthread}
###################################

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
AC_CHECK_FUNCS(pthread_atfork)
AC_CHECK_FUNCS(pthread_cancel)
AC_CHECK_FUNCS(pthread_create)
AC_CHECK_FUNCS(pthread_detach)
AC_CHECK_FUNCS(pthread_equal)
AC_CHECK_FUNCS(pthread_exit)
AC_CHECK_FUNCS(pthread_getconcurrency)
AC_CHECK_FUNCS(pthread_getcpuclockid)
AC_CHECK_FUNCS(pthread_getschedparam)
AC_CHECK_FUNCS(pthread_getspecific)
AC_CHECK_FUNCS(pthread_join)
AC_CHECK_FUNCS(pthread_key_create)
AC_CHECK_FUNCS(pthread_key_delete)
AC_CHECK_FUNCS(pthread_once)

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

        AC_CHECK_FUNCS(pthread_attr_destroy)
        AC_CHECK_FUNCS(pthread_attr_getdetachstate)
        AC_CHECK_FUNCS(pthread_attr_getguardsize)
        AC_CHECK_FUNCS(pthread_attr_getinheritsched)
        AC_CHECK_FUNCS(pthread_attr_getschedparam)
        AC_CHECK_FUNCS(pthread_attr_getschedpolicy)
        AC_CHECK_FUNCS(pthread_attr_getscope)
        AC_CHECK_FUNCS(pthread_attr_getstack)
        AC_CHECK_FUNCS(pthread_attr_getstackaddr)
        AC_CHECK_FUNCS(pthread_attr_getstacksize)
        AC_CHECK_FUNCS(pthread_attr_init)
        AC_CHECK_FUNCS(pthread_attr_setdetachstate)
        AC_CHECK_FUNCS(pthread_attr_setguardsize)
        AC_CHECK_FUNCS(pthread_attr_setinheritsched)
        AC_CHECK_FUNCS(pthread_attr_setschedparam)
        AC_CHECK_FUNCS(pthread_attr_setschedpolicy)
        AC_CHECK_FUNCS(pthread_attr_setscope)
        AC_CHECK_FUNCS(pthread_attr_setstack)
        AC_CHECK_FUNCS(pthread_attr_setstackaddr)
        AC_CHECK_FUNCS(pthread_attr_setstacksize)
	AC_CHECK_FUNCS(pthread_attr_setaffinity_np)
fi

AC_CHECK_FUNCS(pthread_getaffinity_np)
AC_CHECK_FUNCS(pthread_setaffinity_np)

SIZEOF_CPU_SET_T=128
AC_CHECK_TYPE(cpu_set_t,is_cpuset=yes,is_cpuset=no,[#include <pthread.h>])

if test "x$is_cpuset" = "xyes" ; then
	AC_CHECK_SIZEOF(cpu_set_t,,[#include <pthread.h>])
	SIZEOF_CPU_SET_T=$ac_cv_sizeof_cpu_set_t
fi

AC_SUBST([SIZEOF_CPU_SET_T])

AC_CHECK_TYPE(pthread_barrier_t,is_barrier=yes,is_barrier=no,[#include <pthread.h>])

if test "$is_barrier" = "yes" ; then
        AC_CHECK_SIZEOF(pthread_barrier_t,,[#include <pthread.h>])
        AC_SUBST([SIZEOF_PTHREAD_BARRIER_T],[$ac_cv_sizeof_pthread_barrier_t])

        AC_CHECK_SIZEOF(pthread_barrierattr_t,,[#include <pthread.h>])
        AC_SUBST([SIZEOF_PTHREAD_BARRIERATTR_T],[$ac_cv_sizeof_pthread_barrierattr_t])

        AC_CHECK_FUNCS(pthread_barrier_destroy)
        AC_CHECK_FUNCS(pthread_barrier_init)
        AC_CHECK_FUNCS(pthread_barrier_wait)
        AC_CHECK_FUNCS(pthread_barrierattr_destroy)
        AC_CHECK_FUNCS(pthread_barrierattr_getpshared)
        AC_CHECK_FUNCS(pthread_barrierattr_init)
        AC_CHECK_FUNCS(pthread_barrierattr_setpshared)
fi

AC_CHECK_SIZEOF(pthread_once_t,,[#include <pthread.h>])
AC_SUBST([SIZEOF_PTHREAD_ONCE_T],[$ac_cv_sizeof_pthread_once_t])

AC_CHECK_TYPE(pthread_cond_t,is_cond=yes,is_cond=no,[#include <pthread.h>])
if test "$is_cond" = "yes" ; then
        AC_CHECK_SIZEOF(pthread_cond_t,,[#include <pthread.h>])
        AC_SUBST([SIZEOF_PTHREAD_COND_T],[$ac_cv_sizeof_pthread_cond_t])

        AC_CHECK_SIZEOF(pthread_condattr_t,,[#include <pthread.h>])
        AC_SUBST([SIZEOF_PTHREAD_CONDATTR_T],[$ac_cv_sizeof_pthread_condattr_t])

        AC_CHECK_FUNCS(pthread_cond_broadcast)
        AC_CHECK_FUNCS(pthread_cond_destroy)
        AC_CHECK_FUNCS(pthread_cond_init)
        AC_CHECK_FUNCS(pthread_cond_signal)
        AC_CHECK_FUNCS(pthread_cond_timedwait)
        AC_CHECK_FUNCS(pthread_cond_wait)
        AC_CHECK_FUNCS(pthread_condattr_destroy)
        AC_CHECK_FUNCS(pthread_condattr_getclock)
        AC_CHECK_FUNCS(pthread_condattr_getpshared)
        AC_CHECK_FUNCS(pthread_condattr_init)
        AC_CHECK_FUNCS(pthread_condattr_setclock)
        AC_CHECK_FUNCS(pthread_condattr_setpshared)
fi

AC_CHECK_TYPE(pthread_mutex_t,is_mutex=yes,is_mutex=no,[#include <pthread.h>])

if test "$is_mutex" = "yes" ; then 
        AC_CHECK_SIZEOF(pthread_mutex_t,,[#include <pthread.h>])
        AC_SUBST([SIZEOF_PTHREAD_MUTEX_T],[$ac_cv_sizeof_pthread_mutex_t])

        AC_CHECK_SIZEOF(pthread_mutexattr_t,,[#include <pthread.h>])
        AC_SUBST([SIZEOF_PTHREAD_MUTEXATTR_T],[$ac_cv_sizeof_pthread_mutexattr_t])

        AC_CHECK_FUNCS(pthread_mutex_destroy)
        AC_CHECK_FUNCS(pthread_mutex_getprioceiling)
        AC_CHECK_FUNCS(pthread_mutex_init)
        AC_CHECK_FUNCS(pthread_mutex_lock)
        AC_CHECK_FUNCS(pthread_mutex_setprioceiling)
        AC_CHECK_FUNCS(pthread_mutex_timedlock)
        AC_CHECK_FUNCS(pthread_mutex_trylock)
        AC_CHECK_FUNCS(pthread_mutex_unlock)
        AC_CHECK_FUNCS(pthread_mutexattr_destroy)
        AC_CHECK_FUNCS(pthread_mutexattr_getprioceiling)
        AC_CHECK_FUNCS(pthread_mutexattr_getprotocol)
        AC_CHECK_FUNCS(pthread_mutexattr_getpshared)
        AC_CHECK_FUNCS(pthread_mutexattr_gettype)
        AC_CHECK_FUNCS(pthread_mutexattr_init)
        AC_CHECK_FUNCS(pthread_mutexattr_setprioceiling)
        AC_CHECK_FUNCS(pthread_mutexattr_setprotocol)
        AC_CHECK_FUNCS(pthread_mutexattr_setpshared)
        AC_CHECK_FUNCS(pthread_mutexattr_settype)
fi

AC_CHECK_TYPE(pthread_rwlock_t,is_rwlock=yes,is_rwlock=no,[#include <pthread.h>])

if test "$is_rwlock" = "yes" ; then 
        AC_CHECK_SIZEOF(pthread_rwlock_t,,[#include <pthread.h>])
        AC_SUBST([SIZEOF_PTHREAD_RWLOCK_T],[$ac_cv_sizeof_pthread_rwlock_t])

        AC_CHECK_SIZEOF(pthread_rwlockattr_t,,[#include <pthread.h>])
        AC_SUBST([SIZEOF_PTHREAD_RWLOCKATTR_T],[$ac_cv_sizeof_pthread_rwlockattr_t])

        AC_CHECK_FUNCS(pthread_rwlock_destroy)
        AC_CHECK_FUNCS(pthread_rwlock_init)
        AC_CHECK_FUNCS(pthread_rwlock_rdlock)
        AC_CHECK_FUNCS(pthread_rwlock_timedrdlock)
        AC_CHECK_FUNCS(pthread_rwlock_timedwrlock)
        AC_CHECK_FUNCS(pthread_rwlock_tryrdlock)
        AC_CHECK_FUNCS(pthread_rwlock_trywrlock)
        AC_CHECK_FUNCS(pthread_rwlock_unlock)
        AC_CHECK_FUNCS(pthread_rwlock_wrlock)
        AC_CHECK_FUNCS(pthread_rwlockattr_destroy)
        AC_CHECK_FUNCS(pthread_rwlockattr_getpshared)
        AC_CHECK_FUNCS(pthread_rwlockattr_init)
        AC_CHECK_FUNCS(pthread_rwlockattr_setpshared)
fi

AC_CHECK_FUNCS(pthread_self)
AC_CHECK_FUNCS(pthread_setcancelstate)
AC_CHECK_FUNCS(pthread_setcanceltype)
AC_CHECK_FUNCS(pthread_setconcurrency)
AC_CHECK_FUNCS(pthread_setschedparam)
AC_CHECK_FUNCS(pthread_setschedprio)
AC_CHECK_FUNCS(pthread_setspecific)

AC_CHECK_TYPE(pthread_spinlock_t,is_spinlock=yes,is_spinlock=no,[#include <pthread.h>])

if test "$is_spinlock" = "yes" ; then
        AC_CHECK_SIZEOF(pthread_spinlock_t)
        AC_SUBST([SIZEOF_PTHREAD_SPINLOCK_T],[$ac_cv_sizeof_pthread_spinlock_t])

        AC_CHECK_FUNCS(pthread_spin_destroy)
        AC_CHECK_FUNCS(pthread_spin_init)
        AC_CHECK_FUNCS(pthread_spin_lock)
        AC_CHECK_FUNCS(pthread_spin_trylock)
        AC_CHECK_FUNCS(pthread_spin_unlock)
fi

AC_CHECK_FUNCS(pthread_testcancel)

AC_CHECK_HEADER([semaphore.h])

AC_CHECK_TYPE(sem_t,is_semaphore=yes,is_semaphore=no,[#include <semaphore.h>])

if test "$is_semaphore" = "yes" ; then 
        AC_CHECK_SIZEOF(sem_t,,[#include <semaphore.h>])
        AC_SUBST([SIZEOF_SEM_T],[$ac_cv_sizeof_sem_t])
        AC_CHECK_FUNCS(sem_init)
        AC_CHECK_FUNCS(sem_wait)
        AC_CHECK_FUNCS(sem_trywait)
        AC_CHECK_FUNCS(sem_post)
        AC_CHECK_FUNCS(sem_getvalue)
        AC_CHECK_FUNCS(sem_destroy)
        AC_CHECK_FUNCS(sem_open)
        AC_CHECK_FUNCS(sem_close)
        AC_CHECK_FUNCS(sem_unlink)
        AC_CHECK_FUNCS(sem_timedwait)

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

AC_CHECK_FUNCS(pthread_sigmask)
AC_CHECK_FUNCS(pthread_kill)
AC_CHECK_FUNCS(sched_get_priority_max)
AC_CHECK_FUNCS(sched_get_priority_min)

#
# Configure MPC_Arch
#

MPC_CONTEXT_SWITCH_SUPPORTED_ARCHITECTURE="x86_64"

AC_MSG_CHECKING([which context switches to use])

MPC_ARCH_CONTEXT_SWITCH="generic"

if test -z "${MPC_CONTEXT_SWITCH_SUPPORTED_ARCHITECTURE##*$MPC_TARGET_ARCHITECTURE*}"; then
        MPC_ARCH_CONTEXT_SWITCH="$MPC_TARGET_ARCHITECTURE"
        AC_MSG_RESULT($MPC_TARGET_ARCHITECTURE)
else
        AC_MSG_RESULT([Standard])
fi

# Note that by construction the following conditionnals are multually exclusive
AM_CONDITIONAL([MPC_ARCH_X86_64_CONTEXT_SWITCH], [test "x${MPC_ARCH_CONTEXT_SWITCH}" = "xx86_64"])

MPC_MICROTASK_SUPPORTED_ARCHITECTURE="x86_64"

AC_MSG_CHECKING([which Configuring optimized microtasks])

MPC_ARCH_MICROTASK_ARCH="generic"

if test -z "${MPC_MICROTASK_SUPPORTED_ARCHITECTURE##*$MPC_TARGET_ARCHITECTURE*}"; then
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

mpc_compiler_is_pgi="no"

AC_RUN_IFELSE([AC_LANG_PROGRAM([#include <stdlib.h>],[
#ifndef __PGIC__
exit(1);
#endif
])], [mpc_compiler_is_pgi="yes"])

AC_MSG_CHECKING([if compiler is pgi])

if test "x${mpc_compiler_is_pgi}" = "xyes"; then
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
                        else
                                if test "x${mpc_compiler_is_pgi}" = "xyes"; then
                                        mpc_compiler_family="pgi"
                                fi
                        fi
        fi

fi


###################################
###### MPC MOD CONFIGURATION ###### {sec:modular}
###################################

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

# Modules dependent on threads
MPC_OPENMP_ENABLED=yes
MPC_MPI_ENABLED=yes

if test "x${MPC_THREAD_ENABLED}" = "xyes"; then
        AC_DEFINE([MPC_Threads], [1], [MPC_THREAD module was enabled])
else
        AC_MSG_WARN([Disabling threads means disabling OpenMP])
        MPC_OPENMP_ENABLED=no
        MPC_MPI_ENABLED=no
fi

AM_CONDITIONAL([MPC_THREAD_ENABLED], [test "x${MPC_THREAD_ENABLED}" = "xyes"])
AC_SUBST([MPC_THREAD_ENABLED])

HAVE_MPC_THREAD=0

if test "x${MPC_THREAD_ENABLED}" = "xyes"; then
        HAVE_MPC_THREAD=1
fi

AC_SUBST([HAVE_MPC_THREAD])

# Message Passing is for retro compatibility
AC_ARG_ENABLE([message-passing],
[AS_HELP_STRING([--enable-message-passing],
                [Compile MPC with Message Passing Support (same as lowcomm)])],
[MPC_LOWCOMM_MP_ENABLED=$enableval],
[MPC_LOWCOMM_MP_ENABLED=yes])

AC_ARG_ENABLE([lowcomm],
[AS_HELP_STRING([--enable-lowcomm],
                [Compile MPC with Lowcomm Support])],
[MPC_LOWCOMM_ENABLED=$enableval],
[MPC_LOWCOMM_ENABLED=yes])

# Modules dependent on lowcomm
#MPC_MPI_ENABLED

if test "x${MPC_LOWCOMM_ENABLED}" = "xyes" -o "x${MPC_LOWCOMM_MP_ENABLE}" = "xyes"; then
        AC_DEFINE([MPC_Lowcomm], [1], [MPC_Lowcomm module was enabled])
        MPC_LOWCOMM_ENABLED="yes"
else
        AC_MSG_WARN([Disabling lowcomm means disabling MPC's MPI layer and its dependencies])
        MPC_MPI_ENABLED=no
fi

AM_CONDITIONAL([MPC_LOWCOMM_ENABLED], [test "x${MPC_LOWCOMM_ENABLED}" = "xyes"])
AC_SUBST([MPC_LOWCOMM_ENABLED])


AC_ARG_ENABLE([mpi],
[AS_HELP_STRING([--enable-mpi],
                [Compile MPC MPI interface])],
[MPC_MPI_ENABLED=$enableval],
[])

# Modules dependent on MPI
MPC_FORTRAN_ENABLED=yes
MPC_MPIIO_ENABLED=yes

if test "x${MPC_MPI_ENABLED}" = "xyes"; then
        AC_DEFINE([MPC_MPI], [1], [MPC_MPI module was enabled])
else
        AC_MSG_WARN([Disabling MPI means disabling Fortran and MPI-IO])
        MPC_FORTRAN_ENABLED=no
        MPC_MPIIO_ENABLED=no
fi

AM_CONDITIONAL([MPC_MPI_ENABLED], [test "x${MPC_MPI_ENABLED}" = "xyes"])
AC_SUBST([MPC_MPI_ENABLED])


AC_ARG_ENABLE([openmp],
[AS_HELP_STRING([--enable-openmp],
                [Compile MPC OpenMP interface])],
[MPC_OPENMP_ENABLED=$enableval],
[])

if test "x${MPC_OPENMP_ENABLED}" = "xyes"; then

        if test "x${MPC_THREAD_ENABLED}" = "xno"; then
                AC_MSG_ERROR([--enable-openmp requires --enable-threads])
        fi

        AC_DEFINE([MPC_OpenMP], [1], [MPC_OpenMP module was enabled])
fi

AM_CONDITIONAL([MPC_OPENMP_ENABLED], [test "x${MPC_OPENMP_ENABLED}" = "xyes"])
AC_SUBST([MPC_OPENMP_ENABLED])


#
# MPC_Fortran
#

# Create a temporary file
# Args:
#  - $1 : suffix (ex .c) (optionnal)
# Returns the file in TMP
create_temp_file()
{
    # HAS Suffix
    if test $# -gt 0; then
        SUFFIX=$1
        SUFFIX_ARG="--suffix=${SUFFIX}"
    fi

    TMP=$(mktemp ${SUFFIX_ARG} || true)

    if test ! -f "${TMP}"; then
        TMP="/tmp/tmp_$(date +%s%N).${SUFFIX}"
        touch "${TMP}"
    fi

}

# Read symbols from a binary file
# Args:
#  - $1 : binary to be read
# Returns SYMBOL_LIST (empty in case of error)
read_symbols()
{
    SYMBOL_LIST=""
    # Ensure a path was passed
    if test $# -eq 0; then
        echo "read_symbols expects an argument"
        return
    fi

    EXE_PATH="$1"

    if test ! -f "${EXE_PATH}"; then
        echo "Could not find ${EXE_PATH}"
        return
    fi

    if command -v nm > /dev/null 2>&1; then
        SYMBOL_LIST=$(nm "${EXE_PATH}" | cut -d " " -f 3)
    elif command -v strings > /dev/null 2>&1; then
        SYMBOL_LIST=$(strings "${EXE_PATH}")
    elif command -v cat > /dev/null 2>&1; then
        # Last resort use cat
        SYMBOL_LIST=$(cat "${EXE_PATH}")
    fi

}


check_fortran_naming_convention()
{
    create_temp_file ".f90"

    FSRC="$TMP"

cat << EOF > "${FSRC}"

    subroutine fortran_name( a )
        integer a
        a = 1
        return
    end

EOF

    AC_MSG_CHECKING([fortran symbol format])

    create_temp_file ".o"
    TMPO="$TMP"

    $FC -c "${FSRC}" -o "${TMPO}"

    if test "x$?" = "x0"; then

        read_symbols "${TMPO}"

        FORTRAN_NAMING=""

        if echo "${SYMBOL_LIST}" | grep "fortran_name__" > /dev/null 2>&1; then
            FORTRAN_NAMING="__"
            AC_DEFINE([FORTRANDOUBLEUNDERSCORE], [1], [Fortran naming uses double underscores])
        elif echo "${SYMBOL_LIST}" | grep "fortran_name_" > /dev/null 2>&1; then
            FORTRAN_NAMING="_"
            AC_DEFINE([FORTRANUNDERSCORE], [1], [Fortran naming uses underscores])
        elif echo "${SYMBOL_LIST}" | grep "fortran_name" > /dev/null 2>&1; then
            FORTRAN_NAMING="lowercase"
            AC_DEFINE([FORTRANLOWER], [1], [Fortran naming uses lowercase])
        elif echo "${SYMBOL_LIST}" | grep "FORTRAN_NAME" > /dev/null 2>&1; then
            FORTRAN_NAMING="uppercase"
            AC_DEFINE([FORTRANCAPS], [1], [Fortran naming uses uppercase])

        fi

        AC_MSG_RESULT([${FORTRAN_NAMING}])

    else
        AC_MSG_ERROR(["Failed to build test fortran program"])
    fi

    rm -fr "${FSRC}" "${TMPO}"
}

check_fortran_argument_mismatch()
{
    HAVE_FORTRAN_ARG_MISMATCH=""

    create_temp_file ".f90"

    FSRC="$TMP"

cat << EOF > "${FSRC}"
PROGRAM foo
END PROGRAM
EOF

    create_temp_file ".o"
    TMPO="$TMP"

    AC_MSG_CHECKING([presence of fortran -fallow-argument-mismatch])

    $FC -fallow-argument-mismatch -c "${FSRC}" -o "${TMPO}" > /dev/null 2>&1

    if test "x$?" = "x0"; then
        FORTRAN_ARG_MISMATCH_ARG="-fallow-argument-mismatch"
        AC_MSG_RESULT([yes])

    else
        FORTRAN_ARG_MISMATCH_ARG=""
        AC_MSG_RESULT([no])
    fi


    AC_SUBST([FORTRAN_ARG_MISMATCH_ARG])

    rm -fr "${FSRC}" "${TMPO}"
}


AC_ARG_ENABLE([fortran],
[AS_HELP_STRING([--enable-fortran],
                [Compile MPC Fortran support])],
[MPC_FORTRAN_ENABLED=$enableval],
[])

if test "x${MPC_FORTRAN_ENABLED}" = "xyes"; then

        if test "x${MPC_MPI_ENABLED}" = "xno"; then
                AC_MSG_ERROR([--enable-fortran requires --enable-mpi])
        fi

        AC_DEFINE([MPC_FORTRAN], [1], [MPC_Fortran module was enabled])

        check_fortran_naming_convention
        check_fortran_argument_mismatch
fi

AM_CONDITIONAL([MPC_FORTRAN_ENABLED], [test "x${MPC_FORTRAN_ENABLED}" = "xyes"])
AC_SUBST([MPC_FORTRAN_ENABLED])

if test "x${MPC_FORTRAN_ENABLED}" = "xyes"; then
        AC_DEFINE([MPC_FORTRAN_ENABLED], [1], [MPC_Fortran module was enabled])
fi

#
# MPI IO Support (ROMIO)
#

# TODO move it its own M4 file

AC_ARG_ENABLE([mpiio],
[AS_HELP_STRING([--enable-mpiio],
                [Compile MPI IO support in MPC (romio)])],
[MPC_MPIIO_ENABLED=$enableval],
[])

if test "x${MPC_MPIIO_ENABLED}" = "xyes"; then

        if test "x${MPC_MPI_ENABLED}" = "xno"; then
                AC_MSG_ERROR([--enable-mpio requires --enable-mpi])
        fi

        AC_DEFINE([MPC_MPIIO], [1], [MPC_MPIIO module was enabled])
fi

AM_CONDITIONAL([MPC_MPIIO_ENABLED], [test "x${MPC_MPIIO_ENABLED}" = "xyes"])
AC_SUBST([MPC_MPIIO_ENABLED])

AM_CONDITIONAL([BUILD_ROMIO_EMBEDDED], [true])
AM_CONDITIONAL([BUILD_F77_BINDINGS], [test "x${MPC_FORTRAN_ENABLED}" = "xyes"])
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
# And generate module lists and defines
#

ALL_MPC_LIBS="${ac_pwd}/src/MPC_Config/src/libmpcconfig.la"

MPC_MODULE_LIST=""

if test "x${MPC_THREAD_ENABLED}" = "xyes"; then
        ALL_MPC_LIBS="${ALL_MPC_LIBS} ${ac_pwd}/src/MPC_Launch/src/libmpclaunch.la"
        ALL_MPC_LIBS="${ALL_MPC_LIBS} ${ac_pwd}/src/MPC_Threads/src/libmpcthread.la"
        MPC_MODULE_LIST="${MPC_MODULE_LIST} MPC_THREAD_ENABLED"
fi

if test "x${MPC_OPENMP_ENABLED}" = "xyes"; then
        ALL_MPC_LIBS="${ALL_MPC_LIBS} ${ac_pwd}/src/MPC_OpenMP/src/libmpcomp.la"
        MPC_MODULE_LIST="${MPC_MODULE_LIST} MPC_OPENMP_ENABLED"
fi

if test "x${MPC_LOWCOMM_ENABLED}" = "xyes"; then
        ALL_MPC_LIBS="${ALL_MPC_LIBS} ${ac_pwd}/src/MPC_Lowcomm/src/libmpclowcomm.la"
        MPC_MODULE_LIST="${MPC_MODULE_LIST} MPC_LOWCOMM_ENABLED"
fi

if test "x${MPC_MPI_ENABLED}" = "xyes"; then
        ALL_MPC_LIBS="${ALL_MPC_LIBS} ${ac_pwd}/src/MPC_MPI/src/libmpcmpi.la"
        MPC_MODULE_LIST="${MPC_MODULE_LIST} MPC_MPI_ENABLED"
fi

if test "x${MPC_MPIIO_ENABLED}" = "xyes"; then
        ALL_MPC_LIBS="${ALL_MPC_LIBS} ${ac_pwd}/src/MPC_IO/src/libmpcio.la"
        MPC_MODULE_LIST="${MPC_MODULE_LIST} MPC_MPIIO_ENABLED"
fi

if test "x${MPC_FORTRAN_ENABLED}" = "xyes"; then
        MPC_MODULE_LIST="${MPC_MODULE_LIST} MPC_FORTRAN_ENABLED"
fi

MPC_MODULE_DEFINES=""

for m in $MPC_MODULE_LIST
do
        MPC_MODULE_DEFINES=" -D${m}"
done

CFLAGS="${CFLAGS} ${MPC_MODULE_DEFINES}"

AC_SUBST([ALL_MPC_LIBS])
AC_SUBST([MPC_MODULE_LIST])
AC_SUBST([MPC_MODULE_DEFINES])

################################
###### COMPILER DETECTION ###### {sec:compilers}
################################
#
# Configure MPC's compilers
#

# Set autopriv as default compiler if privatization is enabled and autpriv present

mpc_have_autopriv="no"

AC_CHECK_PROG([apcc],[apcc],[yes],[no])
AC_CHECK_PROG([apcpp],[ap++],[yes],[no])
AC_CHECK_PROG([apfortran],[apfortran],[yes],[no])

AC_MSG_CHECKING([for autopriv])

is_autopriv()
{
        RES="no"

        if test -z "$1"; then
                return
        fi

        ${1} "--ap-help" > /dev/null 2>&1

        if test "x$?" = "x0"; then
                RES="yes"
        fi

        echo $RES
}

if test "x${apcc}" = "xyes" -a "x${apcpp}" = "xyes" -a "x${apfortran}" = "xyes";
then
        # We have autopriv in path
        AC_MSG_RESULT([yes])

        #Now check that all compilers are privatizing
        AC_MSG_CHECKING([CC is privatizing])
        CC_PRIV=$(is_autopriv "$CC")
        AC_MSG_RESULT([$CC_PRIV])

        AC_MSG_CHECKING([CXX is privatizing])
        CXX_PRIV=$(is_autopriv "$CXX")
        AC_MSG_RESULT([$CXX_PRIV])

        AC_MSG_CHECKING([FC is privatizing])
        FC_PRIV=$(is_autopriv "$FC")
        AC_MSG_RESULT([$FC_PRIV])

        AC_MSG_CHECKING([All compilers are privatizing])
        if test "x${FC_PRIV}" = "xyes" -a "x${CXX_PRIV}" = "xyes" -a "x${CC_PRIV}" = "xyes"; then
                AC_MSG_RESULT([yes])
                mpc_have_autopriv=yes
        else
                AC_MSG_RESULT([no])
        fi

else
        AC_MSG_RESULT([no])
fi

AM_CONDITIONAL([MPC_HAVE_AUTOPRIV], [test "x${mpc_have_autopriv}" = "xyes"])

# If privatization supported process mode is not enforced
# however if no privatization MPC is forced in process-mode.

AC_ARG_ENABLE([process-mode],
[AS_HELP_STRING([--enable-process-mode],
                [Compile MPC in process mode])],
[enable_mpc_proces_mode=$enableval],
[enable_mpc_proces_mode=no])

# First assume extls not found
autopriv_support_found="no"

if test "x$enable_mpc_proces_mode" = "xno"; then

        MPC_DETECT_DEPENDENCY([extls.h],[autopriv], [extls_init], [EXTLS])
        if test "x$mpc_dep_found" = "xyes"; then
                AC_DEFINE([MPC_USE_EXTLS], [1], [Extls support is enabled])
                autopriv_support_found="yes"
        else
                # If no privatization is available we fallback to Process-Based model
                AC_MSG_NOTICE([No privatization support detected assuming Process-Based Model (no privatization)])
                enable_mpc_proces_mode="yes"
        fi
fi

MPC_IN_PROCESS_MODE="no"

if test "x${enable_mpc_proces_mode}" = "xyes"; then
        AC_DEFINE([MPC_IN_PROCESS_MODE], [1], [MPC compiled in process mode])
        MPC_IN_PROCESS_MODE="yes"
fi

AC_SUBST([MPC_IN_PROCESS_MODE])

# One last sanity check (extls but no privatizing compilers)
if test "x${autopriv_support_found}" = "xyes"; then
        if test "x${mpc_have_autopriv}" = "xno"; then
                echo "!!!!!!!!!!!!!!!!!!!!!!!!"
                AC_MSG_CHECKING([priv in $CC])
                AC_MSG_RESULT([$CC_PRIV])
                
                AC_MSG_CHECKING([priv in $CXX])
                AC_MSG_RESULT([$CXX_PRIV])
                
                AC_MSG_CHECKING([priv in $FC])
                AC_MSG_RESULT([$FC_PRIV])

                AC_MSG_WARN([Make sure to configure with CC=apcc CXX=ap++ FC=apfortran])
                AC_MSG_WARN([or consider passing --enable-process-mode to deactivate priv])
                AC_MSG_ERROR([Extls support was enabled but autopriv was not found])
        fi
fi


###########################
###### MISC. OPTIONS ###### {sec:custom}
###########################

#### Shell color
AC_ARG_ENABLE([shell-colors], [AS_HELP_STRING([--enable-shell-colors], [Activate Color Output in MPC])],
[mpc_shell_color=$enableval], [mpc_shell_color="no"])

AC_ARG_ENABLE([color], [AS_HELP_STRING([--enable-color], [Activate Color Output in MPC (same as --enable-shell-colors)])],
[mpc_shell_color=$enableval], [])

if test "x${mpc_shell_color}" = "xyes"; then
        AC_DEFINE([MPC_ENABLE_SHELL_COLORS], [1], [MPC Colored output])
fi

#### CMA support
MPC_ENABLE_CMA="no"
AC_ARG_ENABLE([cma], [AS_HELP_STRING([--enable-cma], [Activate Cross Memory Attach for SHM communications])],
[MPC_ENABLE_CMA=$enableval], [])

if test "x${MPC_ENABLE_CMA}" = "xyes"; then

        #Check the content of the YAMA file if present
        if test -f /proc/sys/kernel/yama/ptrace_scope; then
                AC_MSG_CHECKING([YAMA ptrace rights])
                YAMA_VAL=$(cat /proc/sys/kernel/yama/ptrace_scope)

                if test "x$YAMA_VAL" = "x0"; then
                        AC_MSG_RESULT([OK])
                else
                        AC_MSG_RESULT([FAIL])
                        AC_MSG_NOTICE([/proc/sys/kernel/yama/ptrace_scope shoud contain 0 for CMA support in MPC])
                        MPC_ENABLE_CMA="no"
                fi
        fi

        AC_MSG_CHECKING([enabling CMA])
        if test "x$MPC_ENABLE_CMA" = "xyes"; then
                AC_MSG_RESULT([yes])
                AC_DEFINE([MPC_USE_CMA], [1], [MPC CMA for SHM drive is enabled])
        else
                AC_MSG_RESULT([no])
        fi
fi

#### Enabling default optimizations to build MPC
AC_ARG_ENABLE([optim],
[AS_HELP_STRING([--enable-optim],
                [Enable standard optimizations to build MPC])],
[mpc_enable_opt=$enableval],
[mpc_enable_opt=yes])

MPC_PORTABLE_OPT_FLAGS=" -O3"

case $mpc_compiler_family in
        "gcc")
                MPC_OPT_FLAGS=" -O3"
        ;;
        "icc")
                MPC_OPT_FLAGS=" -O3"
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

AC_ARG_ENABLE([debug-mem],
[AS_HELP_STRING([--enable-debug-mem],
                [Compile MPC with ASAN checks])],
[mpc_debug_mem=$enableval],
[mpc_debug_mem=no])

AC_ARG_ENABLE([debug],
[AS_HELP_STRING([--enable-debug],
                [Compile MPC in debug mode])],
[mpc_debug=$enableval],
[mpc_debug=no])

if test "x${mpc_debug}" = "xno"; then
        AC_DEFINE([NDEBUG], [1], [Disable assert function])
fi

USING_ASAN="no"

if test "x${mpc_debug_mem}" = "xyes"; then

        case $mpc_compiler_family in
                "gcc"|"clang")
                        CFLAGS+=" -fsanitize=address"
                        AC_CHECK_LIB([asan], [__asan_report_load8])
                        USING_ASAN="yes"
                        # Also set the debug flag
                        mpc_debug="yes"
                ;;
                *)
                        AC_MSG_ERROR([Memory debugging is only supported with ASAN from clang/gcc])
                ;;
        esac
fi

AM_CONDITIONAL([USING_ASAN], [test "x$USING_ASAN" != "xyes"])
AC_SUBST([USING_ASAN])

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

if test "x${mpc_debug}" = "xyes"; then
        CFLAGS+=" $MPC_DEBUG_FLAGS"
fi


####################################
###### FINALIZATION & SUMMARY ###### {sec:finalize}
####################################
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
# Generate Conditionnal Substitutions for PKG-CONFIG
#

MPC_THREAD_PKGCONFIG_DEP=""

if test "x${MPC_THREAD_ENABLED}" = "xyes"; then
        MPC_THREAD_PKGCONFIG_DEP="libmpcthread = ${MPC_VERSION_MAJOR}.${MPC_VERSION_MINOR}.${MPC_VERSION_PATCH}"
fi

AC_SUBST([MPC_THREAD_PKGCONFIG_DEP])

MPC_IO_PKGCONFIG_LIB=""

if test "x${MPC_MPIIO_ENABLED}" = "xyes"; then
        MPC_IO_PKGCONFIG_LIB="-lmpcio"
fi

AC_SUBST([MPC_IO_PKGCONFIG_LIB])

MPC_FORTRAN_PKGCONFIG_LIB=""
MPC_FORTRAN_PKGCONFIG_CFLAGS=""

if test "x${MPC_FORTRAN_ENABLED}" = "xyes"; then
        MPC_FORTRAN_PKGCONFIG_LIB="-lmpcfortran"
        MPC_FORTRAN_PKGCONFIG_CFLAGS="-I${prefix}/include/${PACKAGE_TARNAME}/fortran/"
fi

AC_SUBST([MPC_FORTRAN_PKGCONFIG_LIB])
AC_SUBST([MPC_FORTRAN_PKGCONFIG_CFLAGS])

#
# Generate the Reconfigure Script
#
cat << EOF > ./reconfigure
#!/bin/sh
if test -n "\$(find ${ac_abs_confdir}/configure -newer \$0)"; then
        printf "Warning: Source configure is newer than reconfigure file (./autogen.sh || git check-out)\n"
        printf "Warning: Rebuilding over a different version of MPC can lead to unexpected errors.\n"
fi
if test -f "${PWD}/src/bin/mpc_build_env.sh"; then
        export MPC_CONFIG_RELOADED=yes
        . ${PWD}/src/bin/mpc_build_env.sh
fi

${ac_abs_confdir}/configure $ac_configure_args "\$@"
EOF

chmod +x ./reconfigure


# Generate configuration summary

if test "x${mpc_debug_messages}" = "xyes" -o "x${mpc_debug_mem}" = "xyes" -o "x${mpc_debug}" = "xyes"; then
        PERF_WARNING="WARNING: some debug options are turned on. This impacts performance."
else
        PERF_WARNING=""
fi

MPC_CONFIG_RESULT="\
########## MPC Configuration Script ##########\n\
# Installation into.... $prefix\n\
# Configure run on..... `date -d @$MPC_GENERATION_DATE`\n\
# Version built........ $MPC_VERSION_STRING\n\
# C Compiler........... $(basename $CC)\n\
# C++ Compiler......... $(basename $CXX)\n\
# Fortran Compiler..... $(basename $FC)\n\
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
# MPC_Openmp........... $MPC_OPENMP_ENABLED\n\
# MPC_Lowcomm.......... $MPC_LOWCOMM_ENABLED\n\
# MPC_MPI.............. $MPC_MPI_ENABLED\n\
# MPC_Fortran.......... $MPC_FORTRAN_ENABLED\n\
# MPC_MPIIO............ $MPC_MPIIO_ENABLED\n\
#
### OPTIONAL Interfaces ###\n\
#\n\
# Active Messages...... $MPC_HAVE_AM\n\
#\n\
### COMPILERS ###\n\
#\n\
# Privatization........ $autopriv_support_found\n\
# Process mode......... $enable_mpc_proces_mode\n\
# mpc_cc points to..... $(basename $CC)\n\
# mpc_cxx points to.... $(basename $CXX)\n\
# mpc_fXX points to.... $(basename $FC)\n\
#\n\
### NETWORKS ###\n\
#\n\
# Supported list....... $MPC_NETWORK_LIST\n\
# CMA over SHM......... $MPC_ENABLE_CMA\n\
#\n$MPC_MPIIO_CONFIGURATION\
### ACCELERATORS ###\n\
#\n\
# Cuda support......... $cuda_enabled\n\
#\n\
### OTHER OPTIONS SET ###\n\
#\n\
# Launcher............. $MPC_LAUNCHER\n\
# Launcher Program..... $MPC_LAUNCHER_PROGRAM\n\
# Allocator............ $MPC_ALLOCATOR\n\
# libunwind............ $mpc_libunwind\n\
# libvmem.............. $mpc_libvmem\n\
# libffi............... $HAVE_LIB_FFI\n\
# DMTPC c/r............ $MPC_HAVE_DMTCP\n\
#\n\
### DEBUGGING ###\n\
#\n\
# Embedded profiler.... $MPC_HAVE_PROFILER\n\
# Debug symbols........ $mpc_debug ($MPC_DEBUG_FLAGS)\n\
# Memory Debug......... $mpc_debug_mem\n\
# Debug messages....... $mpc_debug_messages\n\
# Colored output....... $mpc_shell_color\n\
# ${PERF_WARNING}\n\
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
           src/MPC_Common/include/mpc_common_types.h
           src/MPC_Common/include/mpc_common_spinlock.h
           src/MPC_Common/include/mpc_keywords.h
           src/MPC_Config/Makefile
           src/MPC_Config/src/Makefile
           src/MPC_Config/conf/Makefile
           src/MPC_Launch/Makefile
           src/MPC_Launch/src/Makefile
           src/MPC_Lowcomm/Makefile
           src/MPC_Lowcomm/src/Makefile
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
           src/MPC_IO/src/romio/Makefile
           src/MPC_Fortran/Makefile
           src/MPC_Fortran/src/Makefile
           src/include/mpc_threads_config.h
           src/include/mpc.h
           src/bin/Makefile
           src/bin/mpc_build_env.sh
           src/bin/mpc_cflags
           src/bin/mpc_ldflags
           src/bin/mpcrun
           src/bin/mpcvars.sh
           src/bin/mpc_status
           pkgconfig/Makefile
           pkgconfig/libmpclowcomm.pc
           pkgconfig/libmpcthread.pc
           pkgconfig/libmpcmpi.pc
           cmake/Makefile])



printf "\n$MPC_CONFIG_RESULT"

if test "x${MPC_CONFIG_RELOADED}" = "xyes"; then

echo "************ IMPORTANT NOTICE ****************"
echo "MPCs configure realized that you already ran"
echo "a configure in this directory and restored the"
echo "previous build environment. If it is not what"
echo "you wanted please manually reconfigure MPC"
echo "***********************************************"

fi
