#!/bin/sh
# This script generates and installs the following Fortran files :
# - mpif.h
# - mpi.mod

#EXTRACT SCRIPT ENV
SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

#PREPARE BUILDING CONTEXT

PYTHON="/usr/bin/env python"
MPCFC="mpc_f77"
MPCCC="mpc_cc"
AR="ar"
INSTALL="./TEST/"
MPIHFILE="${SCRIPTPATH}/../../MPC_MPI/include/mpc_mpi.h"

#These are the files used for generation

MPIFH=""
MPIMODF=""
MPICONSTF=""
MPISIZEOF=""
MPIBASEF=""

MPIF08CTYPE=""
MPIF08CIF=""
MPIF08MTYPE=""
MPIF08C=""
MPIF08CONST=""
MPIF08=""

#Can the file be generated ?
GENERATE=1

die()
{
	printf "(EE) $@\n" 1>&2
	exit 2
}

warn()
{
	printf "(WW) $@\n" 1>&2
}

info()
{
	printf "(II) $@\n"
}

# This is the emegency function
# it uses a pregenerated module
# in case something goes wrong
# with the automatic generation

generate_from_backup()
{
	warn "$1"
	warn "Fallback to pregenerated files"
	#Here we fallback to pregenerated files
	MPIFH="$SCRIPTPATH/pregenerated/mpif.h"
	MPIMODF="$SCRIPTPATH/pregenerated/mpi.f90"
	MPISIZEOF="$SCRIPTPATH/pregenerated/mpi_sizeofs.f90"
	MPICONSTF="$SCRIPTPATH/pregenerated/mpi_constants.f90"
	MPIBASEF="$SCRIPTPATH/pregenerated/mpi_base.f90"

	MPIF08CTYPE="$SCRIPTPATH/pregenerated/mpi_f08_ctypes.f90"
	MPIF08CIF="$SCRIPTPATH/pregenerated/mpi_f08_c_iface.c"
	MPIF08MTYPE="$SCRIPTPATH/pregenerated/mpi_f08_types.f90"
	MPIF08C="$SCRIPTPATH/pregenerated/mpi_f08_c.f90"
	MPIF08CONST="$SCRIPTPATH/pregenerated/mpi_f08_constants.f90"
	MPIF08="$SCRIPTPATH/pregenerated/mpi_f08.f90"

	GENERATE=0
}




checkFortranGenEnv()
{
	rm -fr ${INSTALL}/fortrangen.log
	rm -fr ${INSTALL}/fortrangen_script.log
	
	#CHECK ENV DEPENDENCIES

	#CHEK for PYTHON
	RET=`${PYTHON} -c "print 'ok'"`
	rm -f $TEMP

	if test "x$RET" != "xok"; then
		generate_from_backup "Python command-line does not work as expected"
		return
	fi	

	info "Found and running Python"
	
	TEMP=`mktemp`

cat << EOF > $TEMP
#!/usr/bin/env python 

use json
print "ok"

EOF

	RET=""
	RET=`${PYTHON} -c "print 'ok'"`
	if test "x$RET" != "xok"; then
		generate_from_backup "Python does not seem to handle JSON support."
		return
	fi
	rm -f $TEMP
	
	info "Found JSON support for Python"
	
	TEMP=`mktemp`
	TEMP="${TEMP}.f90"
cat << EOF > $TEMP
PROGRAM TESTAPP

END PROGRAM TESTAPP
EOF

	TEMP2=`mktemp`
	$MPCFC $TEMP -o $TEMP2  2>> ./fortrangen.log
	if test "x$?" != "x0"; then
		generate_from_backup "Failed to compile Fortran program."
		return
	fi
	rm -f $TEMP

	$TEMP2  2>> ./fortrangen.log
	if test "x$?" != "x0"; then
                generate_from_backup "Failed to run a Fortran program."
		return
	fi        
	rm -f $TEMP2
	
	info "Found capable Fortran compiler"
	
	TEMP=`mktemp`.f90
cat << EOF > $TEMP
MODULE TEST_MOD
IMPLICIT NONE

END MODULE TEST_MOD

PROGRAM TESTAPP
END PROGRAM TESTAPP
EOF

	TEMP2=`mktemp`
	$MPCFC $TEMP -o $TEMP2  2>> ./fortrangen.log
	if test "x$?" != "x0" -o ! -e "test_mod.mod"; then
		generate_from_backup "Failed to compile a simple Fortran Module."
		return
	fi	
	rm -f $TEMP $TEMP2 test_mod.mod

	info "Fortran compiler can successfully build Fortran modules."

	TEMP=`mktemp`.c
cat << EOF > $TEMP
#include <mpc.h>
int main(int argc, char ** argv )
{
	return 0;
}
EOF

	TEMP2=`mktemp`
	$MPCCC $TEMP -o $TEMP2  2>> ./fortrangen.log

	if test "x$?" != "x0"; then
		generate_from_backup "Failed to build a C program."
		return
	fi
	rm -f $TEMP $TEMP2
	info "Found a capable C Compiler"

}

genfortranfiles()
{
	TEMP=`mktemp`
	echo "$ $MPCCC ${SCRIPTPATH}/gen_iface.c -o ${TEMP}"
	$MPCCC ${SCRIPTPATH}/gen_iface.c -o ${TEMP} 2>> ./fortrangen.log
	${TEMP} > constants.json 2>> ./fortrangen.log

	if test "x$?" != "x0" -o ! -e "constants.json"; then
		generate_from_backup "Failed to generate constants JSON file."
		return
	fi
	rm -f $TEMP
	
	${MPCCC} -E ${MPIHFILE} -o preparsed_mpih.dat 2>> ./fortrangen.log
	
	if test "x$?" != "x0" -o ! -e "preparsed_mpih.dat"; then
		generate_from_backup "Failed to generate the preparsed mpi.h file"
		return
	fi
	info "Generated pre-parsed mpi.h file"
	
	${PYTHON} ${SCRIPTPATH}/genmod.py -f "${MPCFC}" -c "${MPCCC}" -m "${MPIHFILE}"

	if test "x$?" != "x0"; then
		generate_from_backup "Fortran module/header generation failed."
		return
	fi
}

genfortranmods()
{
	mpc_inc_path=""
	test "x$GENERATE" = "x0" && mpc_inc_path="-I$SCRIPTPATH/pregenerated"
	#We do not want to privatize Fortran constants
	export AP_UNPRIVATIZED_VARS=mpi_in_place:mpi_status_ignore:mpi_statuses_ignore:$AP_UNPRIVATIZED_VARS

	# Here we run the compilation three times.
	# This is due to old Fortran compilers, not able to resolve module dependency at once.
	# Each call resolves a dependency, leading to 3 compilatoins in a row.
	# TODO: A loop, retrying to compile while return code != 0
	$MPCFC -g -fpic -c ${MPIBASEF} ${MPICONSTF} ${MPIMODF} ${MPISIZEOF} ${MPIF08CTYPE} ${MPIF08MTYPE} ${MPIF08C} ${MPIF08CONST} ${MPIF08}  2>> ./fortrangen.log
	$MPCFC -g -fpic -c ${MPIBASEF} ${MPICONSTF} ${MPIMODF} ${MPISIZEOF} ${MPIF08CTYPE} ${MPIF08MTYPE} ${MPIF08C} ${MPIF08CONST} ${MPIF08}  2>> ./fortrangen.log
	$MPCFC -g -fpic -c ${MPIBASEF} ${MPICONSTF} ${MPIMODF} ${MPISIZEOF} ${MPIF08CTYPE} ${MPIF08MTYPE} ${MPIF08C} ${MPIF08CONST} ${MPIF08}  2>> ./fortrangen.log
	
	test "x$?" != "x0" && die "Failed to build pre-computed Fortran modules"
	info "Built pre-computed Fortran modules"
	
	$MPCFC -g -fpic -I. ${mpc_inc_path} -c $SCRIPTPATH/predef_types.f $SCRIPTPATH/predef_types_08.f90  2>> ./fortrangen.log
	test "x$?" != "x0" && die "Failed to build pre-defined Fortran modules (types)"
	info "Built pre-defined Fortran modules (types)"
	
	$MPCCC -g -fpic -c ${MPIF08CIF}  2>> ./fortrangen.log
	test "x$?" != "x0" && die "Failed to build Fortran 08 C interface."
	info "Built MPI interface for Fortran 08"
		
	$MPCFC -g -fpic -shared -o libmpcfwrapper.so *.o  2>> ./fortrangen.log
	test "x$?" != "x0" && die "Failed to build MPI Wrapper C library"
	info "Built MPI Wrapper library"

	#Compiled twice for the same reasons as above
	$MPCFC -c $SCRIPTPATH/pregenerated/omp.f90  2>> ./fortrangen.log
	$MPCFC -c $SCRIPTPATH/pregenerated/omp.f90  2>> ./fortrangen.log
	test "x$?" != "x0" && die "Failed to build OpenMP for Fortran"
	info "Built OpenMP for Fortran module"
}


#MAIN FILE
proceedwithfortranfilegeneration()
{
	printf "\n########################################################"
	printf "\n#   MPC Fortran Header and Module Generation Script    #"
	printf "\n########################################################\n"


	checkFortranGenEnv

	#CAN WE STILL GENERATE ?
	if test "x${GENERATE}" = "x1"; then
		printf "\n########################################################"
		printf "\n#   Fortran Generation script will generate Headers    #"
		printf "\n########################################################\n"
		
		genfortranfiles
		
		for f in mpi_base.f90 mpi_constants.f90 mpi_sizeofs.f90  mpi.f90 mpif.h mpi_f08_ctypes.f90 mpi_f08_types.f90 mpi_f08_c.f90 mpi_f08_constants.f90 mpi_f08.f90 mpi_f08_c_iface.c
		do
			if test -e "$f"; then
				info "${f}: found"
			else
				generate_from_backup "${f}: NOT found."
				break
			fi
			
			#If we are here all generated files are existing
			MPIFH="${PWD}/mpif.h"
			MPIMODF="${PWD}/mpi.f90"
			MPICONSTF="${PWD}/mpi_constants.f90"
			MPISIZEOF="${PWD}/mpi_sizeofs.f90"
			MPIBASEF="${PWD}/mpi_base.f90"
		
			MPIF08CTYPE="${PWD}/mpi_f08_ctypes.f90"
			MPIF08MTYPE="${PWD}/mpi_f08_types.f90"
			MPIF08CIF="${PWD}/mpi_f08_c_iface.c"
			MPIF08C="${PWD}/mpi_f08_c.f90"
			MPIF08CONST="${PWD}/mpi_f08_constants.f90"
			MPIF08="${PWD}/mpi_f08.f90"
		done
	fi

	printf "\n\n########################################################"
	printf "\n#  Fortran Generation script about to compile modules  #"
	printf "\n########################################################\n"

	genfortranmods
	
	printf "\n\n########################################################"
	printf "\n#  Fortran Generation script checking built modules    #"
	printf "\n########################################################\n"

	for f in \
	mpi_base.mod \
	mpi_constants.mod \
	mpi_sizeofs.mod \
	mpi.mod \
	mpi_f08_c.mod \
	mpi_f08_constants.mod \
	mpi_f08_ctypes.mod \
	mpi_f08_types.mod \
	mpi_f08.mod \
	omp_lib.mod \
	omp_lib_kinds.mod \
	libmpcfwrapper.so
	do
		if test -e "$f"; then
			info "${f}: Found."
		else
			warn "${f}: NOT Found. (Fortran code using it might not be able to compile)"
		fi	
	done

	printf "\n\n########################################################"
	printf "\n#  Installing Fortran Files                            #"
	printf "\n########################################################\n"

	test ! -w ${INSTALL} && die "Unable to copy generated files into $INSTALL: No write permissions."

	mkdir -p ${INSTALL}/include
	mkdir -p ${INSTALL}/lib

	#copy include
	cp ${MPIFH} ${INSTALL}/include

	#copy lib:
	mv omp_lib.mod ${INSTALL}/lib
	mv omp_lib_kinds.mod ${INSTALL}/lib
	mv mpi.mod ${INSTALL}/lib
	mv mpi_constants.mod ${INSTALL}/lib
	mv mpi_base.mod ${INSTALL}/lib
	mv mpi_sizeofs.mod ${INSTALL}/lib
	mv mpi_f08_c.mod ${INSTALL}/lib
	mv mpi_f08_ctypes.mod ${INSTALL}/lib
	mv mpi_f08_types.mod ${INSTALL}/lib
	mv mpi_f08_constants.mod ${INSTALL}/lib
	mv mpi_f08.mod ${INSTALL}/lib
	mv libmpcfwrapper.so ${INSTALL}/lib
}

test "x${#}" != "x4" && die "
======================
MPC Fortran Generator
======================

${0} [MPC_CC] [MPC_F77] [MPIH] [INSTALL_PATH]
"

MPCCC="$1"
MPCFC="$2"
MPIHFILE="$3"
INSTALL="$4"

test ! -f "${MPIHFILE}" && die "Could not locate the mpi.h file in ${MPIHFILE}"


BUILDIR=`mktemp -d`
OLDIR=${PWD}
#move in temporary dir, because this script creates a lot of artefacts
info "Moving to $BUILDIR"
cd $BUILDIR

echo "" > fortrangen.log

proceedwithfortranfilegeneration

cd $OLDIR
#rm -rf $BUILDIR


