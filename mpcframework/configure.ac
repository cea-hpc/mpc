AC_INIT([mpc], [4.0], [france@paratools.com])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_LANG(C)
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_SRCDIR([MPC_Arch/src/timer.c])
AC_CONFIG_HEADERS([mpc_config.h])
AM_SILENT_RULES([yes])
LT_PREREQ([2.4.6])


LT_INIT([disable-static])
LT_LANG([C])
LT_INIT
AC_PROG_CXX

# Define MPC_VERSION

AC_DEFINE([SCTK_VERSION_MAJOR], [4], [MPC Major Version 3])
AC_DEFINE([SCTK_VERSION_MINOR], [0], [MPC Minor Version 3])
AC_DEFINE([SCTK_VERSION_PRE], [0], [MPC Pre Version 3])
AC_DEFINE([SCTK_VERSION_REVISION], [0], [MPC Revision Version 3])

AC_DEFINE_UNQUOTED([MPC_PREFIX_PATH], ["$prefix"], [Prefix Path for MPC])

# General Configuration

MPC_SUPPORTED_ARCHITECTURES_LIST="x86_64 ia64 i686 arm aarch64 powerpc64"

# Programs Checks
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_CC
AM_PROG_AS

# Optionnal XML generators

AC_CHECK_PROG([xsltproc],[xsltproc],[yes],[no])
AC_CHECK_PROG([xmllint],[xmllint],[yes],[no])

AM_CONDITIONAL([CAN_GENERATE_CONFIG], [test "x$xsltproc" = xyes -o "x$xmllint" = xyes])

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

        for pref in $provided_prefix $prefix_candidate_path
        do
                for suffix in $prefix_candidate_sub_include_path
                do
                        AC_MSG_CHECKING([for '$looked_up_header' in '$pref/$suffix'])
                        if test -f "$pref/$suffix/$looked_up_header"
                        then
                                AC_MSG_RESULT([yes])
                                # Set include path for matched header
                                CPPFLAGS+=" -I$pref/$suffix/"

                                AX_RESET_HEADERS_CACHE([$looked_up_header])
                                AC_CHECK_HEADER([$looked_up_header],[],[AC_MSG_ERROR([Failed to autodetect '$looked_up_header' in '$pref/$suffix'])])
                                check_or_infer_prefix_include=yes
                                return
                        fi
                        AC_MSG_RESULT([no])
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

        for pref in $provided_prefix $prefix_candidate_path
        do
                for suffix in $prefix_candidate_sub_lib_path
                do
                        lib_found=0


                        OLD_LDFLAGS=$LDFLAGS

                        LDFLAGS+=" -L$pref/$suffix -l$1"

                        ___unset_lib_cache
                        AC_CHECK_LIB($looked_up_lib, $looked_up_function, [lib_found=1], [lib_found=0])
                        AC_MSG_CHECKING([Looking for '$looked_up_lib' in '$pref/$suffix'])

                        if test "x$lib_found" = "x1"
                        then
                                AC_MSG_RESULT([yes])
                                check_or_infer_prefix_lib=yes
                                return
                        fi

                        AC_MSG_RESULT([no])
                        LDFLAGS=$OLD_LDFLAGS

                done
        done
}

#
# $1 header_name (ex. hwloc.h)
# $2 libname     (ex. hwloc)
# $3 looked up function (ex. hwloc_topology_init)
# $4 LIBCODE (ex XML, HWLOC, ...)
# $5 optionnal library version (need pkg-config)
#
AC_DEFUN([MPC_DETECT_DEPENDENCY],
         [
                AS_VAR_PUSHDEF([dep_found], [mpc_dep_found])
                AC_ARG_WITH([$2],
                [AS_HELP_STRING([--with-$2],
                                [Provide prefix for $2])],
                [prefix=$withval],[])

                AC_ARG_WITH([$2-include],
                [AS_HELP_STRING([--with-$2-inc],
                                [Provide include path for $2])],
                [inc_prefix=$withval],[])

                AC_ARG_WITH([$2-libs],
                [AS_HELP_STRING([--with-$2-libs],
                                [Provide library path for $2])],
                [lib_prefix=$withval],[])

                check_or_infer_prefix_result="no"
                header_name=$1
                libname=$2
                function_name=$3
                version="$5"

                if test -n "$lib_prefix"
                then
                        lib_prefix=$prefix
                fi

                if test -n "$inc_prefix"
                then
                        inc_prefix=$prefix
                fi

                # First try with PKG config
                PKG_CHECK_MODULES([$4], [$libname $version], [pkg_found=1], [pkg_found=0])

                if test "x$pkg_found" = "x1"
                then
                        #Insert flags
                        CPPFLAGS+=" ${$4_CFLAGS}"

                        LDFLAGS+=" ${$4_LIBS}"
                        AC_MSG_NOTICE([$libname found using pkg-config])
                        #AC_CHECK_LIB($libname,$function_name,[], [AC_MSG_ERROR([Could not locate '$function_name' in '$libname'])])
                        check_or_infer_prefix_result="yes"
                else
                        __check_or_infer_prefix_include $header_name $inc_prefix
                        __check_or_infer_prefix_lib $libname $function_name $lib_prefix

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
                        AC_MSG_NOTICE([consider using --with-$1=PREFIX])
                        AC_MSG_NOTICE([consider using --with-$1-libs=LIBS])
                        AC_MSG_NOTICE([consider using --with-$1-include=INCS])
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

AC_MSG_CHECKING([MPC target alocator])

if test "x$mpc_dep_found" = "xyes"; then
        AC_MSG_RESULT([mpcalloc])
        AC_DEFINE([MPC_Allocator], [1], [mpcalloc support is enabled])

        mpc_allocator="mpcalloc"
else
        AC_MSG_RESULT([Standard allocator])
fi

#
# Network list
#

mpc_net_list="tcp shm"

#
# Detect libibverbs presence
#

infiniband_enabled="no"

MPC_DETECT_DEPENDENCY([infiniband/verbs.h], [ibverbs], [ibv_query_qp], [IBV])

AC_MSG_CHECKING([Checking for libiverbs to enable verbs support])

if test "x$mpc_dep_found" = "xyes"; then
        AC_MSG_RESULT([yes])
        mpc_net_list="$mpc_net_list verbs"
        AC_DEFINE([MPC_USE_INFINIBAND], [1], [Infiniband support is enabled])
        infiniband_enabled="yes"
else
        AC_MSG_RESULT([no])
fi

AM_CONDITIONAL([INFINIBAND_DRIVER_ENABLED], [test "x$infiniband_enabled" = "xyes"])

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
                [Rely on SLURM's PMI])],
[mpc_hydra_enabled=no; mpc_slurm_enabled=$withval],
[mpc_slurm_enabled=no])

if test "x$mpc_hydra_enabled" = "xyes" -a  "x$mpc_slurm_enabled" = "xyes"
then
        AC_MSG_ERROR([You mush choose between Hydra and Slurm (--with-hydra or --with-slurm)])
fi

if test "x$mpc_hydra_enabled" = "xyes" -o  "x$mpc_slurm_enabled" = "xyes"
then
        #All good
        dummy=1337
else
        AC_MSG_ERROR([You mush activate either Hydra and Slurm (--with-hydra or --with-slurm)])
fi

if test "x$mpc_slurm_enabled" = "xyes"; then

        mpc_launcher="Slurm PMI"
        MPC_DETECT_DEPENDENCY([pmi.h],[pmi], [PMI_Finalize], [PMI])
        MPC_REQUIRED_DEPENDENCY([pmi], [$mpc_dep_found])

        AC_DEFINE([MPC_USE_SLURM], [1], [Slurm PMI will be used to launch parallel programs])
else
        AC_CHECK_PROG([HAVE_MPIEXEC], [mpiexec.hydra], [yes], [no])

        if test "x$HAVE_MPIEXEC" = "xno"
        then
                AC_MSG_ERROR([Could not locate Hydra's 'mpiexec.hydra' in PATH])
        fi

        # Here we look for hydra only if hydra has not been disabled
        mpc_launcher="Hydra"

        AC_CHECK_FUNCS([socket setsockopt gethostbyname],[can_access_hydra_pmi_socket=no], [can_access_hydra_pmi_socket=yes])

        if test "x$can_access_hydra_pmi_socket" = "xno" ; then
                AC_DEFINE(USE_PMI_PORT,1,[HYDRA supports sockets])
        else
                AC_MSG_ERROR([Hydra requires socket support])
        fi

        AC_DEFINE([MPC_USE_HYDRA], [1], [Hydra will be used to launch parallel programs])
fi

AM_CONDITIONAL([HYDRA_CLIENT_LIB_ENABED], [test "x$mpc_launcher" = "xHydra"])

# Checks for header files.

AC_HEADER_STDC

AC_CHECK_HEADERS([aio.h])
AC_CHECK_HEADERS([asm/unistd.h])
AC_CHECK_HEADERS([assert.h])
AC_CHECK_HEADERS([ctype.h])
AC_CHECK_HEADERS([errno.h])
AC_CHECK_HEADERS([execinfo.h])
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
AC_CHECK_HEADERS([pthread.h])
AC_CHECK_HEADERS([regex.h])
AC_CHECK_HEADERS([sched.h])
AC_CHECK_HEADERS([semaphore.h])
AC_CHECK_HEADERS([setjmp.h])
AC_CHECK_HEADERS([signal.h])
AC_CHECK_HEADERS([stddef.h])
AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([stdint.h])
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

# Resolve Host CPU

AC_CANONICAL_HOST
MPC_TARGET_ARCHITECTURE=$host_cpu
AC_SUBST([MPC_TARGET_ARCHITECTURE])

# Check if architecture is in support list

AC_MSG_CHECKING([Checking if target architecture is supported])

is_in_list()
{
    echo "$2" | grep -q "$1"
    return $?
}

if is_in_list "$MPC_TARGET_ARCHITECTURE" "$MPC_SUPPORTED_ARCHITECTURES_LIST"; then
        AC_MSG_RESULT($MPC_TARGET_ARCHITECTURE)
else
        AC_MSG_RESULT(Unsupported)
        AC_MSG_ERROR(Your architecture is unsupported)
fi


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

AC_MSG_CHECKING([macro __func__])

AC_COMPILE_IFELSE(AC_LANG_PROGRAM([#include <stdio.h>],
                                  [fprintf(stderr,"%s",__func__);
                                   return 0;]),
                res_macro=yes,res_macro=no)

if test "$res_macro" = "yes" ; then 
   MPC_ARCH_DETECTED_FUNCTION="__func__"
   AC_MSG_RESULT(yes)
   AC_SUBST(MPC_ARCH_DETECTED_FUNCTION)
else
   AC_MSG_RESULT(no)
   AC_MSG_ERROR([Unable to detect function name])
fi

AC_MSG_CHECKING([macro __STRING])

AC_COMPILE_IFELSE(AC_LANG_PROGRAM([#include <stdio.h>],
                                  [fprintf(stderr,"%s",__STRING(TOTO));
                                   return 0;]),
                res_macro=yes,res_macro=no)

if test "$res_macro" = "yes" ; then 
  MPC_ARCH_DETECTED_STRING="__STRING"
   AC_MSG_RESULT(yes)
   AC_SUBST(MPC_ARCH_DETECTED_STRING)
else
   AC_MSG_RESULT(no)
   AC_MSG_ERROR([Unable to convert to string])
fi

# Checking for ENVIRON variable


AC_MSG_CHECKING([if environ variable is present])

AC_COMPILE_IFELSE(AC_LANG_PROGRAM([#include <stdlib.h>
                                   #include <stdio.h>
                                   extern char ** environ;
                                   ],
                                [fprintf(stderr,"%p\n",environ);]),
                                res_macro=yes,res_macro=no)

if test "$res_macro" = "yes" ; then 
   AC_MSG_RESULT(yes)
   AC_DEFINE([HAVE_ENVIRON_VAR],[1],[The environ variable is present])
else
   AC_MSG_RESULT(no)
fi


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
AC_CHECK_SIZEOF(double)
AC_CHECK_SIZEOF(size_t)
AC_CHECK_SIZEOF(void*)
AC_CHECK_SIZEOF(long long)

# Checks for standard library functions.

AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memcpy memmove memset])
AC_CHECK_FUNCS([strcasecmp strchr strdup strerror strstr strtol snprintf])
AC_CHECK_FUNCS([gethostbyname gethostname])
AC_CHECK_FUNCS([gettimeofday])
AC_CHECK_FUNCS([munmap])
AC_CHECK_FUNCS([setenv getenv])
AC_CHECK_FUNCS([uname])
AC_CHECK_FUNCS([regcomp])
AC_CHECK_FUNCS([socket connect accept])
AC_CHECK_FUNCS([dup2 fdatasync])
AC_CHECK_FUNCS([makecontext setcontext getcontext swapcontext])

# Check for pThread

AC_MSG_NOTICE(Check Pthreads functions)
AC_CHECK_HEADER([pthread.h])
AC_CHECK_LIB(pthread,pthread_create,LIBS="$LIBS -lpthread")
AC_CHECK_SIZEOF(pthread_t,,[#include <pthread.h>])
AC_CHECK_SIZEOF(sigset_t,,[#include <signal.h>])

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

AC_MSG_CHECKING([Checking if PTHREAD_KEYS_MAX is defined])

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
else
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

        AC_MSG_CHECKING([Experimentally determined PTHREAD_KEYS_MAX to be])

        AC_DEFINE_UNQUOTED([MPC_THREAD_KEYS_MAX], [$key_count], [Experimentally defined PTHREAD_KEYS_MAX])

        AC_MSG_RESULT([$key_count])

fi


AC_CHECK_TYPE(pthread_attr_t,is_thread=yes,is_thread=no,[#include <pthread.h>])

if test "$is_thread" = "yes" ; then
        AC_CHECK_SIZEOF(pthread_attr_t,,[#include <pthread.h>])
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
        AC_CHECK_SIZEOF(pthread_barrierattr_t,,[#include <pthread.h>])
        AC_CHECK_FUNCS(pthread_barrier_destroy )
        AC_CHECK_FUNCS(pthread_barrier_init )
        AC_CHECK_FUNCS(pthread_barrier_wait )
        AC_CHECK_FUNCS(pthread_barrierattr_destroy )
        AC_CHECK_FUNCS(pthread_barrierattr_getpshared )
        AC_CHECK_FUNCS(pthread_barrierattr_init )
        AC_CHECK_FUNCS(pthread_barrierattr_setpshared )
fi

AC_CHECK_TYPE(pthread_cond_t,is_cond=yes,is_cond=no,[#include <pthread.h>])
if test "$is_cond" = "yes" ; then
        AC_CHECK_SIZEOF(pthread_cond_t,,[#include <pthread.h>])
        AC_CHECK_SIZEOF(pthread_condattr_t,,[#include <pthread.h>])
        AC_CHECK_SIZEOF(pthread_once_t,,[#include <pthread.h>])
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
        AC_CHECK_SIZEOF(pthread_mutexattr_t,,[#include <pthread.h>])
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
        AC_CHECK_SIZEOF(pthread_rwlockattr_t,,[#include <pthread.h>])
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
fi

AC_CHECK_FUNCS(pthread_sigmask )
AC_CHECK_FUNCS(pthread_kill )
AC_CHECK_FUNCS(sched_get_priority_max )
AC_CHECK_FUNCS(sched_get_priority_min )

#
# Configure Privatization Support
#

mpc_privatization_support="no"

MPC_DETECT_DEPENDENCY([extls.h],[extls], [extls_init], [EXTLS])

AC_MSG_CHECKING([Enabling extended TLS support])

if test "x$mpc_dep_found" = "xyes"; then
        AC_MSG_RESULT([yes])
        AC_DEFINE([MPC_USE_EXTLS], [1], [Extls support is enabled])


        mpc_privatization_support="yes"
else
        AC_MSG_RESULT([no])
fi

#
# Configure MPC_Arch
#

MPC_CONTEXT_SWITCH_SUPPORTED_ARCHITECTURE="x86_64"

AC_MSG_CHECKING([Configuring optimized context switches])

MPC_ARCH_CONTEXT_SWITCH="generic"

if is_in_list "$MPC_TARGET_ARCHITECTURE" "$MPC_CONTEXT_SWITCH_SUPPORTED_ARCHITECTURE"; then
        MPC_ARCH_CONTEXT_SWITCH="$MPC_TARGET_ARCHITECTURE"
        AC_MSG_RESULT($MPC_TARGET_ARCHITECTURE)
else
        AC_MSG_RESULT(Using standard context switches)
fi

# Note that by construction the following conditionnals are multually exclusive
AM_CONDITIONAL([MPC_ARCH_X86_64_CONTEXT_SWITCH], [test "x${MPC_ARCH_CONTEXT_SWITCH}" = "xx86_64"])

MPC_MICROTASK_SUPPORTED_ARCHITECTURE="x86_64"

AC_MSG_CHECKING([Configuring optimized microtasks])

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


#
# Modular MPC configuration
#

AC_ARG_ENABLE([threads],
[AS_HELP_STRING([--enable-threads],
                [Compile MPC with pthread and user-level thread support])],
[mpc_thread_enabled=$enableval],
[mpc_thread_enabled=yes])

if test "x${mpc_thread_enabled}" = "xyes"; then
        AC_DEFINE([MPC_Threads], [1], [MPC_THREAD module was enabled])
fi

AM_CONDITIONAL([MPC_THREAD_ENABLED], [test "x${mpc_thread_enabled}" = "xyes"])


AC_ARG_ENABLE([message-passing],
[AS_HELP_STRING([--enable-message-passing],
                [Compile MPC with Message Passing Support])],
[mpc_lowcom_enabled=$enableval],
[mpc_lowcom_enabled=yes])

if test "x${mpc_lowcom_enabled}" = "xyes"; then
        AC_DEFINE([MPC_Message_Passing], [1], [MPC_MESSAGE_PASSING module was enabled])
fi

AM_CONDITIONAL([MPC_MESSAGE_PASSING_ENABLED], [test "x${mpc_lowcom_enabled}" = "xyes"])


AC_ARG_ENABLE([mpi],
[AS_HELP_STRING([--enable-mpi],
                [Compile MPC's MPI interface])],
[mpc_mpi_enabled=$enableval],
[mpc_mpi_enabled=yes])

if test "x${mpc_mpi_enabled}" = "xyes"; then
        AC_DEFINE([MPC_MPI], [1], [MPC_MPI module was enabled])
fi

AM_CONDITIONAL([MPC_MPI_ENABLED], [test "x${mpc_mpi_enabled}" = "xyes"])

#
# Final Output
#

AC_OUTPUT([MPC_Arch/include/mpc_arch.h])

AC_OUTPUT([Makefile MPC_Arch/Makefile MPC_Arch/src/Makefile
           MPC_Common/Makefile MPC_Common/src/Makefile MPC_Config/Makefile
           MPC_Launch/src/Makefile MPC_Launch/Makefile
           MPC_Message_Passing/src/Makefile MPC_Message_Passing/Makefile
           MPC_MPI/src/Makefile MPC_MPI/Makefile
           MPC_Config/src/Makefile MPC_Topology/Makefile MPC_Topology/src/Makefile
           MPC_Threads/Makefile MPC_Threads/src/Makefile])

echo "========== MPC Configuration =========="
echo "PREFIX: $prefix"
echo ""
echo "Process Launcher: $mpc_launcher"
echo "Allocator: $mpc_allocator"
echo "Privatization support: $mpc_privatization_support"
echo ""
echo "Enabled networks: $mpc_net_list"
echo ""
echo "The following modules will be built:"
echo ""
echo "MPC_Thread : $mpc_thread_enabled"
echo "MPC_Message_passing : $mpc_lowcom_enabled"
echo "MPC_MPI : $mpc_mpi_enabled"
echo "======================================="

