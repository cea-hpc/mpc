#!/bin/bash
######################################################
#            PROJECT  : svMPC                        #
#            VERSION  : 0.0.0                        #
#            DATE     : 11/2013                      #
#            AUTHOR   : Valat Sébastien              #
#            LICENSE  : CeCILL-C                     #
######################################################

######################################################
#setup paths
PROJECT_SOURCE_DIR="`dirname "$0"`"
PROJECT_BUILD_DIR="$PWD"

######################################################
#avoid to build locally
if [ "${PROJECT_SOURCE_DIR}" = "." ] || [ "${PROJECT_BUILD_DIR}" = "${PROJECT_SOURCE_DIR}" ]; then
	PROJECT_BUILD_DIR="${PWD}/build"
	PROJECT_SOURCE_DIR="${PWD}"
	mkdir -p build
	cd build
fi

######################################################
#compute subdirs
#TODO: modifier extern-deps pour inclure les dir des deps + patches 
PROJECT_TEMPLATE_DIR="${PROJECT_SOURCE_DIR}/tools/templates"
PROJECT_PACKAGE_DIR="${PROJECT_SOURCE_DIR}/extern-deps"
#PROJECT_PATCH_DIR="${PROJECT_SOURCE_DIR}/patches"
PROJECT_HELP_DIR="${PROJECT_SOURCE_DIR}/tools/help"

######################################################
#include common
. "${PROJECT_SOURCE_DIR}/tools/Common.sh"


######################################################
#Set all modules to internal



# TODO : clean this and use it in config file
setModulesToInternal

######################################################
#Parse options
for arg in "$@"
do
	case "$arg" in
		################# SCTK_ARCH #################
		--with-hydra=*)
			extractParamValue 'HYDRA_PREFIX' 'with-hydra' "$arg"
			;;
		--hydra-*)
			extractAndAddPackageOption 'HYDRA_BUILD_PARAMETERS' 'hydra' "$arg"
			;;
		################# SCTK_ARCH #################
		--with-sctk-arch=*)
			extractParamValue 'SCTK_ARCH_PREFIX' 'with-sctk-arch' "$arg"
			;;
		--sctk-arch-*)
			extractAndAddPackageOption 'SCTK_ARCH_BUILD_PARAMETERS' 'sctk-arch' "$arg"
			;;
		#################### OPENPA #################
		--with-openpa=*)
			extractParamValue 'OPENPA_PREFIX' 'with-openpa' "$arg"
			;;
		--openpa-*)
			extractAndAddPackageOption 'OPENPA_BUILD_PARAMETERS' 'openpa' "$arg"
			;;
		################### MPC GDB #################
		--with-mpc-gdb=*)
			extractParamValue 'MPC_GDB_PREFIX' 'with-mpc-gdb' "$arg"
			;;
		--mpc-gdb-*)
			extractAndAddPackageOption 'MPC_GDB_BUILD_PARAMETERS' 'mpc-gdb' "$arg"
			;;
		--disable-mpc-gdb)
			MPC_GDB_PREFIX='disabled'
			;;
		################### MPC GCC #################
		--with-mpc-gcc=*)
			extractParamValue 'MPC_GCC_PREFIX' 'with-mpc-gcc' "$arg"
			;;
		--mpc-gcc-*)
			extractAndAddPackageOption 'MPC_GCC_BUILD_PARAMETERS' 'mpc-gcc' "$arg"
			;;
		--disable-mpc-gcc)
			MPC_GCC_PREFIX='disabled'
			;;
		##################### MPFR ##################
		--with-mpfr=*)
			extractParamValue 'MPFR_PREFIX' 'with-mpfr' "$arg"
			;;
		--mpfr-*)
			extractAndAddPackageOption 'MPFR_BUILD_PARAMETERS' 'mpfr' "$arg"
			;;
		##################### GMP ###################
		--with-gmp=*)
			extractParamValue 'GMP_PREFIX' 'with-gmp' "$arg"
			;;
		--gmp-*)
			extractAndAddPackageOption 'GMP_BUILD_PARAMETERS' 'gmp' "$arg"
			;;
		################## BINUTILS #################
		--with-mpc-binutils=*)
			extractParamValue 'BINUTILS_PREFIX' 'with-mpc-binutils' "$arg"
			;;
		--mpc-binutils-*)
			extractAndAddPackageOption 'BINUTILS_BUILD_PARAMETERS' 'mpc-binutils' "$arg"
			;;
		################## LIBXML2 ##################
		--with-libxml2=*)
			extractParamValue 'LIBXML2_PREFIX' 'with-libxml2' "$arg"
			;;
		--libxml2-*)
			extractAndAddPackageOption 'LIBXML2_BUILD_PARAMETERS' 'libxml2' "$arg"
			;;
		################### HWLOC ###################
		--with-hwloc=*)
			extractParamValue 'HWLOC_PREFIX' 'with-hwloc' "$arg"
			;;
		--hwloc-*)
			extractAndAddPackageOption 'HWLOC_BUILD_PARAMETERS' 'hwloc' "$arg"
			;;
		################## COMMON ###################
		--enable-multi-arch)
			MULTI_ARCH='true'
			;;
		--enable-color)
			ENABLE_COLOR='true'
			;;
		--enable-all-internals)
			ALL_INTERNALS='true'
			;;
		-v | --verbose | --verbose=1)
			export STEP_WRAPPER_VERBOSE=1;
			;;
		-vv | --verbose=2)
			export STEP_WRAPPER_VERBOSE=2;
			;;
		-vvv | --verbose=3)
			export STEP_WRAPPER_VERBOSE=3;
			;;
		-j*)
			MAKE_J="$arg"
			;;
		--prefix=*)
			extractParamValue 'PREFIX' 'prefix' "$arg"
			;;
		--help|-h|-?)
			showHelp
			;;
		*)
			echo "Invalid argument '$arg', please check your command line or get help with --help." 1>&2
			exit 1
	esac
done

######################################################
#Add prefix to local variable to also make the search
enablePrefixEnv "${PREFIX}"

######################################################
#force MPC
MPC_PREFIX="${INTERNAL_KEY}"
#TODO : wrie FindHydra
HYDRA_PREFIX="${INTERNAL_KEY}"
HYDRA_REAL_PREFIX="${PREFIX}"
HYDRA_SIMPLE_PREFIX="${INTERNAL_KEY}"
HYDRA_SIMPLE_REAL_PREFIX="${PREFIX}"

######################################################
#Print summary
echo "=================== SYMMARY =================="
echo "BUILD        : ${PROJECT_BUILD_DIR}"
echo "SOURCE       : ${PROJECT_SOURCE_DIR}"
echo "INSTALL      : ${PREFIX}"
echo "libxml2      : ${LIBXML2_PREFIX}"
echo "hwloc        : ${HWLOC_PREFIX}"
echo "binutils     : ${BINUTILS_PREFIX}"
echo "gmp          : ${GMP_PREFIX}"
echo "mpfr         : ${MPFR_PREFIX}"
echo "openpa       : ${OPENPA_PREFIX}"
echo "sctk-arch    : ${SCTK_ARCH_PREFIX}"
echo "mpc-gcc      : ${MPC_GCC_PREFIX}"
echo "mpc-gdb      : ${MPC_GDB_PREFIX}"
echo "hydra        : ${HYDRA_PREFIX}"
echo "hydra-simple : ${HYDRA_SIMPLE_PREFIX}"
echo "mpc          : ${MPC_PREFIX}"
echo "=============================================="

######################################################
#Create glue between packages
MPC_GCC_BUILD_PARAMETERS="'--with-gmp=${GMP_REAL_PREFIX}' '--with-mpfr=${MPFR_REAL_PREFIX}' ${MPC_GCC_BUILD_PARAMETERS}"
MPC_BUILD_PARAMETERS="'--with-libxml2=${LIBXML2_REAL_PREFIX}' '--with-openpa=${OPENPA_REAL_PREFIX}' '--with-hwloc=${HWLOC_REAL_PREFIX}' '--with-hydra-simple=${HYDRA_SIMPLE_REAL_PREFIX}' ${MPC_BUILD_PARAMETERS}"
MPFR_BUILD_PARAMETERS="'--with-gmp=${GMP_REAL_PREFIX}' ${MPFR_BUILD_PARAMETERS}"
HYDRA_SIMPLE_BUILD_PARAMETERS="'--with-hydra-mpl=${HYDRA_REAL_PREFIX}' ${HYDRA_SIMPLE_BUILD_PARAMETERS}"

######################################################
#Clear makefile to generate it
genMakefile()
{
	applyOnTemplate "${PROJECT_TEMPLATE_DIR}/Makefile.head.in" > Makefile
	installAutotoolsExternPackage 'libxml2' 'LIBXML2'  >> Makefile
	installAutotoolsExternPackage 'hwloc' 'HWLOC'  >> Makefile
	installAutotoolsExternPackage 'binutils' 'BINUTILS'  >> Makefile
	installAutotoolsExternPackage 'gmp' 'GMP'  >> Makefile
	installAutotoolsExternPackage 'mpfr' 'MPFR'  >> Makefile
	installAutotoolsExternPackage 'openpa' 'OPENPA'  >> Makefile
	installAutotoolsExternPackage 'gcc' 'MPC_GCC'  >> Makefile
	installAutotoolsExternPackage 'gdb' 'MPC_GDB'  >> Makefile
	installAutotoolsExternPackage 'cmake' 'CMAKE'  >> Makefile
	installAutotoolsExternPackage 'hydra' 'HYDRA'  >> Makefile
	installAutotoolsLocalPackage 'libsctk-arch' 'SCTK_ARCH' >> Makefile
	installAutotoolsLocalPackageHydraSimple 'hydra-simple' 'HYDRA_SIMPLE'  >> Makefile
	installAutotoolsLocalPackage 'mpc' 'MPC'  >> Makefile
	applyOnTemplate "${PROJECT_TEMPLATE_DIR}/Makefile.foot.in" >> Makefile
}

######################################################
genMakeForSubPrefix()
{
	local subprefix="$1"
	
	if [ ! -d "$subprefix" ]; then
		mkdir "$subprefix" || exit 1
	fi

	cd "$subprefix" || exit 1
	SUBPREFIX="/$subprefix"
	genMakefile
	cd .. || exit 1
}

######################################################
createRootMakefile()
{
	SUBPREFIXES="$*"
	applyOnTemplate "${PROJECT_TEMPLATE_DIR}/Makefile.multiprefix.root.in" > Makefile
	for SUBPREFIX in $*
	do
		applyOnTemplate "${PROJECT_TEMPLATE_DIR}/Makefile.multiprefix.subprefix.in" >> Makefile
	done
}

######################################################
if [ "$MULTI_ARCH" = 'true' ]; then
	genMakeForSubPrefix 'host'
	genMakeForSubPrefix 'target'
	createRootMakefile 'host' 'target'
else
	genMakefile
fi

######################################################
#some info
if [ ! -z "$MAKE_J" ]; then
	echo "Running in parallel with $MAKE_J..."
fi

#made verbose if seq
if [ "$MAKE_J" = "-j1" ] || [ -z "$MAKE_J" ]; then
	if [ -z "$STEP_WRAPPER_VERBOSE" ]; then export STEP_WRAPPER_VERBOSE=3; fi
fi

#do it
if [ "$STEP_WRAPPER_VERBOSE" = '3' ]; then
	make $MAKE_J || fatal "Finish with errors, please read previous messages."
else
	make --quiet $MAKE_J || fatal "Finish with errors, please read previous messages."
fi

######################################################
echo
echo "FINISHED, you can now use the MPC package installed into $PREFIX"
echo "You can now use MPC by source XXXXXXXXX (TODO) in your shell"
echo 
echo "For developpers, use this in mpc/ subdirectory : "
echo "mkdir build && cd build && ../configure $MPC_BUILD_PARAMETERS"
