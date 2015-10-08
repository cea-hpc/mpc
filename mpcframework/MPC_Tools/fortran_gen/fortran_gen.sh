#!/bin/sh


# This script generates and installs the following Fortran files :
# - mpif.h
# - mpi.mod

#EXTRACT SCRIPT ENV
SCRIPT="$0"
SCRIPTPATH=`dirname $SCRIPT`

#PREPARE BUILDING CONTEXT

PYTHON="/usr/bin/env python"
MPCFC="mpc_f77"
MPCCC="mpc_cc"
INSTALL="./TEST/"
MPIHFILE="${SCRIPTPATH}/../../MPC_MPI/include/mpc_mpi.h"

if test ! -e "${MPIHFILE}"; then
	echo "Could not locate the mpi.h file in ${MPIHFILE}"
	return
fi

#These are the files used for generation

MPIFH=""
MPIMODF=""
MPICONSTF=""
MPISIZEOF=""
MPIBASEF=""

#Can the file be generated ?
GENERATE=1

# This is the emegency function
# it uses a pregenerated module
# in case something goes wrong
# with the automatic generation

generate_from_backup()
{
	echo "(!!)\t\tFallback to pre-generated files"
	#Here we fallback to pregenerated files
	MPIFH="$SCRIPTPATH/pregenerated/mpif.h"
	MPIMODF="$SCRIPTPATH/pregenerated/mpi.f90"
	MPISIZEOF="$SCRIPTPATH/pregenerated/mpi_sizeofs.f90"
	MPICONSTF="$SCRIPTPATH/pregenerated/mpi_constants.f90"
	MPIBASEF="$SCRIPTPATH/pregenerated/mpi_base.f90"
		
	GENERATE=0
}




checkFortranGenEnv()
{
	rm -fr ./fortrangen.log
	
	#CHECK ENV DEPENDENCIES

	#CHEK for PYTHON
	RET=`${PYTHON} -c "print 'ok'"`

	echo -n "(i) Checking for Python...\t"

	if test "x$RET" != "xok"; then
		echo "(E) Could not find Python or Python is not working"
		generate_from_backup
		return
	else
		echo "Found and Running"
	fi

	rm -f $TEMP

	#CHECK FOR JSON SUPPORT
	echo -n "(i) Checking for Python JSON...\t"
	
	
	TEMP=`mktemp`

cat << EOF > $TEMP
#!/usr/bin/env python 

use json
print "ok"

EOF

	RET=""
	RET=`${PYTHON} -c "print 'ok'"`
	if test "x$RET" != "xok"; then
		echo "(E) Could not find JSON support in Python"
		generate_from_backup
		return
	else
		echo "Found and Running"
	fi


	rm -f $TEMP

	#TEST FORTRAN
	
	echo -n "(i) Checking for Fortran compiler...   "
	
	TEMP=`mktemp`
	TEMP="${TEMP}.f90"
cat << EOF > $TEMP
PROGRAM TESTAPP

END PROGRAM TESTAPP
EOF

	TEMP2=`mktemp`
	$MPCFC $TEMP -o $TEMP2  2>> ./fortrangen.log

	if test "x$?" != "x0"; then
		echo "(E) Failed to compile a simple Fortran MAIN"
		echo "(E) See ./fortrangen.log"
		generate_from_backup
	else
		echo "Found and Running"
	fi

	rm -f $TEMP $TEMP2


	echo -n "(i) Checking for Fortran Modules...   "

TEMP=`mktemp`
TEMP="${TEMP}.f90"
cat << EOF > $TEMP


MODULE TEST_MOD
IMPLICIT NONE

END MODULE TEST_MOD

PROGRAM TESTAPP
END PROGRAM TESTAPP
EOF

	TEMP2=`mktemp`
	$MPCFC $TEMP -o $TEMP2  2>> ./fortrangen.log

	if test "x$?" != "x0"; then
		echo "(E) Failed to compile a simple Fortran Module"
		echo "(E) See ./fortrangen.log"
		generate_from_backup
	else
		echo "Compiled"
		
		if test -e "test_mod.mod"; then
			echo "(i)\tSucessfully generated a test fortran module file"
		else
			echo "(E)\tCould not generate a test module file"
			generate_from_backup
		fi
		
	fi

	rm -f $TEMP $TEMP2 test_mod.mod

#TEST FOR C COMPILER
	
	echo -n "(i) Checking for C compiler...   "
	
	TEMP=`mktemp`
	TEMP="${TEMP}.c"
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
		echo "Failed to compile a simple C MAIN"
		echo "(E) See ./fortrangen.log"
		generate_from_backup
	else
		echo "Found and Running"
	fi

	#rm -f $TEMP $TEMP2

}

genfortanfiles()
{
	#First compile the constants code
	echo -n "(i) Compiling constants file...   "
	
	TEMP=`mktemp`
	$MPCCC ${SCRIPTPATH}/gen_iface.c -o ${TEMP} 2>> ./fortrangen.log

	if test "x$?" != "x0"; then
		echo "(E) Failed to compile constant generation C file"
		echo "(E) See ./fortrangen.log"
		generate_from_backup
		return
	else
		echo "Generated"
	fi
	
	#and now generate the constants
	echo -n "(i) Generating constants.json...   "
	
	${TEMP} > constants.json 2>> ./fortrangen.log
	
	if test -e "constants.json"; then
		echo "Generated"
	else
		echo "(E) Failed to generate the constants.json file"
		echo "(E) See ./fortrangen.log"
		generate_from_backup
		return
	fi
	
	#Now preparse MPI.h to allow function detection
	echo -n "(i) Generating preparsed mpi.h ...   "
	
	${MPCCC} -E ${MPIHFILE} -o preparsed_mpih.dat 2>> ./fortrangen.log
	
	if test -e "preparsed_mpih.dat"; then
		echo "Generated"
	else
		echo "(E) Failed to generate the preparsed mpi.h file"
		echo "(E) See ./fortrangen.log"
		generate_from_backup
		return
	fi
	
	#And now launch the python generation phase
	${PYTHON} ${SCRIPTPATH}/genmod.py -f "${MPCFC}" -c "${MPCCC}" -m "${MPIHFILE}" 
	#2>> ./fortrangen.log

	if test "x$?" != "x0"; then
		echo "(E) Fortran module/header generation failed"
		echo "(E) See ./fortrangen.log"
		generate_from_backup
		return
	else
		echo "(i) **** GENERATION SUCCESSFUL **** "
	fi
	
}

genfortranmods()
{
	#Generate a main using MPI for sanity check
	echo -n "(i) Compiling Fortran modules...   "

	#Note that it is important to compile twice (Fortran MAGIC dependency resolution)
	$MPCFC -c ${MPIBASEF} ${MPICONSTF} ${MPIMODF} ${MPISIZEOF}  2>> ./fortrangen.log
	$MPCFC -c ${MPIBASEF} ${MPICONSTF} ${MPIMODF} ${MPISIZEOF}  2>> ./fortrangen.log
	
	if test "x$?" != "x0"; then
		echo "(E) Module compilation failed"
		echo "(E) See ./fortrangen.log"
		generate_from_backup
		return
	else
		echo "Done"
	fi	
	
	rm -f $TEMP $TEMP2
	
}


#MAIN FILE
proceedwithfortranfilegeneration()
{
	echo "\n########################################################"
	echo "#   MPC Fortran Header and Module Generation Script    #"
	echo "########################################################\n"


	checkFortranGenEnv

	#CAN WE STILL GENERATE ?
	if test "x${GENERATE}" = "x1"; then
		echo "\n########################################################"
		echo "#   Fortran Generation script will generate Headers    #"
		echo "########################################################\n"
		
		genfortanfiles
		
		#Check that everything is there
		echo "(i) Checking generated files...   "
		
		for f in mpi_base.f90 mpi_constants.f90 mpi_sizeofs.f90  mpi.f90 mpif.h
		do
			if test -e "$f"; then
				echo "(i)\t${f}... Found."
			else
				echo "(E)\t${f}... NOT Found."
				generate_from_backup
				return
			fi
			
			#If we are here all generated files are existing
			MPIFH="${PWD}/mpif.h"
			MPIMODF="${PWD}/mpi.f90"
			MPICONSTF="${PWD}/mpi_constants.f90"
			MPISIZEOF="${PWD}/mpi_sizeofs.f90"
			MPIBASEF="${PWD}/mpi_base.f90"
			
		done
	else
		echo "\n########################################################"
		echo "#  (E) Fortran Generation script uses Fallback Headers   #"
		echo "########################################################\n"
		generate_from_backup
	fi


	echo "\n########################################################"
	echo "#  Fortran Generation script about to compile modules    #"
	echo "########################################################\n"

	genfortranmods

	echo "(i) Checking Module files...   "
		
	for f in mpi_base.mod mpi_constants.mod mpi_sizeofs.mod mpi.mod
	do
		if test -e "$f"; then
			echo "(i)\t${f}... Found."
		else
			echo "(E)\t${f}... NOT Found."
			echo "(FATAL) Fortran module compilation failed"
			echo "   You might not be able to run F90 codes"
			return
		fi	
	done

	echo "\n########################################################"
	echo "#  Installing Fortran Files                              #"
	echo "########################################################\n"

	cp ${MPIFH} ${INSTALL}/
	cp mpi.mod ${INSTALL}/
	cp mpi_constants.mod ${INSTALL}/
	cp mpi_base.mod ${INSTALL}/
	cp mpi_sizeofs.mod ${INSTALL}/
	cp mpi_sizeofs.o ${INSTALL}/../../lib/

	echo "(i) Checking installed files...   "
		
	for f in mpi_base.mod mpi_constants.mod mpi_sizeofs.mod ../../lib/mpi_sizeofs.o mpi.mod mpif.h
	do
		NAME=`basename ${f}` 
		if test -e "${INSTALL}/${f}"; then
			echo "(i)\t${NAME}... Found."
		else
			echo "(E)\t${NAME}... NOT Found."
			echo "(FATAL) Fortran module compilation failed"
			echo "   You might not be able to run F90 codes"
			return
		fi	
	done
	
	echo " #### FORTRAN DONE ####"
}


if test "x${#}" != "x5"; then

cat << EOF
======================
MPC Fortran Generator
======================

${0} [PYTHON] [MPC_CC] [MPC_F77] [MPIH] [INCLUDE_INSTALL]

EOF

exit 1

else
	PYTHON="$1"
	MPCCC="$2"
	MPCFC="$3"
	MPIHFILE="$4"
	INSTALL="$5"
fi


proceedwithfortranfilegeneration

