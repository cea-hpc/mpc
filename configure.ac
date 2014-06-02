#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([FULL-PACKAGE-NAME], [VERSION], [BUG-REPORT-ADDRESS])

# Checks for libraries.
AC_MSG_CHECKING([for fortran compiler])
AC_PROG_F77(f77 f90 gfortran g77 ifort fl32 fort77 xlf cf77 f90 xlf90)
AC_MSG_CHECKING([for Termcap library])
AC_CHECK_LIB(termcap, tgetent, , [AC_MSG_NOTICE([Termcap library not found, see http://ftp.gnu.org/gnu/termcap/])])
AC_MSG_CHECKING([for CLooG library])
AC_CHECK_LIB(cloog-ppl,cloog_clast_create, , [AC_MSG_NOTICE([CLooG library not found, see www.cloog.org])])
AC_MSG_CHECKING([for ISL library])
AC_CHECK_LIB(isl, isl_printer_to_str, , [AC_MSG_NOTICE([ISL library not found])])

AC_MSG_CHECKING([for infiniband])
AC_CHECK_LIB(ibverbs, ibv_get_device_list, [], AC_MSG_NOTICE([libibverbs not found.]))
#
# Check for whether to build slurm module
# 
AC_MSG_CHECKING([for whether to build slurm module])
AC_ARG_WITH([slurm],
AC_HELP_STRING([--with-slurm], 
	[support running mpc under SLURM allocation]),
	[ case "$withval" in
    	yes) ac_with_slurm=yes ;;
        no)  ac_with_slurm=no ;;
        *)   AC_MSG_RESULT([doh!])
             AC_MSG_ERROR([bad value "$withval" for --with-slurm]) ;;
    esac
    ]
)
AC_MSG_RESULT([${ac_with_slurm=no}]) 

if test "$ac_with_slurm" = "yes"; then
	AC_CHECK_LIB(slurm, slurm_load_jobs, , [AC_MSG_NOTICE([Cannot support slurm without libslurm.])])
fi

AC_OUTPUT
