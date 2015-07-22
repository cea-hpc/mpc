#!/usr/bin/env python 
import json
import sys
import argparse
import os
import string

scriptpath = os.path.dirname(os.path.realpath(sys.argv[0]))

'''
Argument Parsing
'''

argparser = argparse.ArgumentParser(description='This is the MPC fortran Module Generator')

argparser.add_argument('-c', '--comp', action='store', help='The C compiler used to preprocess the MPI.h header')
argparser.add_argument('-m', '--mpih', action='store', help='Path to the \'mpi.h\' header file to be preprocessed')

args = argparser.parse_args()
#print args

if (args.comp == None) or (args.mpih == None) :
	print "ERROR Missing argument see '" + os.path.basename(sys.argv[0]) + " -h'"

'''
Load Knowledge Bases
'''

# ALL MPI function footprints
with open(scriptpath + '/mpi_interface.json') as data_file:    
    mpi_interface = json.load(data_file)

# The C to Fortran data-type conversion table
with open(scriptpath + '/typesc2f.json') as data_file:    
    typesc2f = json.load(data_file)

# Load current MPC constants from the output of the "gen_iface.c" file
with open('./constants.json') as data_file:    
    mpcconstants = json.load(data_file)



module_file_data = "";


"""
 Write Copyright HEADER
"""

def genheader():
	return  ('! ############################# MPC License ############################## \n'
		'! # Wed Nov 19 15:19:19 CET 2008                                         # \n'
		'! # Copyright or (C) or Copr. Commissariat a l\'Energie Atomique          # \n'
		'! #                                                                      # \n'
		'! # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # \n'
		'! # This file is part of the MPC Runtime.                                # \n'
		'! #                                                                      # \n'
		'! # This software is governed by the CeCILL-C license under French law   # \n'
		'! # and abiding by the rules of distribution of free software.  You can  # \n'
		'! # use, modify and/ or redistribute the software under the terms of     # \n'
		'! # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # \n'
		'! # following URL http://www.cecill.info.                                # \n'
		'! #                                                                      # \n'
		'! # The fact that you are presently reading this means that you have     # \n'
		'! # had knowledge of the CeCILL-C license and that you accept its        # \n'
		'! # terms.                                                               # \n'
		'! #                                                                      # \n'
		'! # Authors:                                                             # \n'
		'! #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # \n'
		'! #   - AUTOGENERATED by modulegen.py                                    # \n'
		'! #                                                                      # \n'
		'! ######################################################################## \n'
		'\n'
		'! /!\ DO NOT EDIT THIS FILE IS AUTOGENERATED BY modulegen.py\n'
		'\n');

#
#  THIS IS mpif.h
#
#

print "\n#################################"
print "Generating the mpif.h Header File"
print "#################################\n"

def genConstantsFromMPC(genmpif=0):
	ret = ""
	print "Processing MPC constants..."
	for const in mpcconstants:
		#Skip the last empty elem */
		if const.get("name") == None:
			continue;
		name =  const["name"]
		decl =  const["decl"]
		ismpif =  const["mpif_only"]
		
		#process the mpif part only if enabled
		if ismpif and ( genmpif == 0):
			continue;
		
		print "\tGEN\t" + name + "..."
		ret += decl;
	return ret;


module_file_data = genheader()

module_file_data += genConstantsFromMPC( 1 );

"""
 Open Output header FILE
"""

f = open("mpif.h", "w" )

f.write( module_file_data );

f.close()


#
#
#  THIS IS THE CONSTANT FORTRAN MODULE
#
#
print "\n#######################################"
print "Generating the CONSTANTS Fortran Module"
print "#######################################\n"

module_file_data = genheader()




"""
 Write Module START
"""

module_file_data +=("MODULE MPI_CONSTANTS\n"
		"IMPLICIT NONE\n\n");

"""
 Write Constant data-types
"""


module_file_data += genConstantsFromMPC();

module_file_data += "\n\n";

module_file_data += (
'TYPE :: MPI_Status\n'
'	INTEGER :: MPI_SOURCE, MPI_TAG, MPI_ERROR\n'
'	INTEGER :: count_lo\n'
'	INTEGER :: count_hi_and_cancelled\n'
'END TYPE MPI_Status\n\n');


const_types = [ "MPI_Comm", "MPI_Datatype", "MPI_Group", "MPI_Win", "MPI_File", "MPI_Op", "MPI_Errhandler", "MPI_Request", "MPI_Message", "MPI_Info" ];

for t in const_types:
	module_file_data += ('TYPE :: ' + t + '\n'
			 '\tINTEGER :: VAL\n'
			 'END TYPE ' + t + '\n\n');



#
# HERE WE ARE DONE WITH THE CONSTANT MODULE FILE
#

module_file_data +="END MODULE MPI_CONSTANTS\n";


"""
 Open Output Module FILE
"""

f = open("mpi_constants.f90", "w" )

f.write( module_file_data );

f.close()


#
#
#  THIS IS THE BASE FORTRAN MODULE
#
#

print "\n##################################"
print "Generating the BASE Fortran Module"
print "##################################\n"


module_file_data = genheader()


"""
 Write Module START
"""

module_file_data +=("MODULE MPI_BASE\n"
		"IMPLICIT NONE\n\n");




"""
Common DECL
"""

module_file_data += "\n\nINTERFACE\n\n";


module_file_data += ('\n'
'SUBROUTINE MPI_INIT(ierror)\n'
'	INTEGER ierror\n'
'END SUBROUTINE MPI_INIT\n'
'\n'
'SUBROUTINE MPI_INIT_THREAD(v0,v1,ierror)\n'
'	INTEGER v0, v1, ierror\n'
'END SUBROUTINE MPI_INIT_THREAD\n'
'\n'
'FUNCTION MPI_WTIME()\n'
'	DOUBLE PRECISION MPI_WTIME\n'
'END FUNCTION MPI_WTIME\n'
'\n'
'FUNCTION MPI_WTICK()\n'
'	DOUBLE PRECISION MPI_WTICK\n'
'END FUNCTION MPI_WTICK\n'
'\n'
'FUNCTION PMPI_WTIME()\n'
'	DOUBLE PRECISION PMPI_WTIME\n'
'END FUNCTION PMPI_WTIME\n'
'\n'
'FUNCTION PMPI_WTICK()\n'
'	DOUBLE PRECISION PMPI_WTICK\n'
'END FUNCTION PMPI_WTICK\n'
'\n'
'SUBROUTINE MPI_NULL_DELETE_FN(COMM, KEYVAL, ATTRIBUTE_VAL,&\n'
'		EXTRA_STATE, IERROR)\n'
'	USE MPI_CONSTANTS,ONLY: MPI_ADDRESS_KIND\n'
'	INTEGER COMM, KEYVAL, IERROR\n'
'	INTEGER(KIND=MPI_ADDRESS_KIND) ATTRIBUTE_VAL, EXTRA_STATE\n'
'END SUBROUTINE MPI_NULL_DELETE_FN\n'
'\n'
'SUBROUTINE MPI_DUP_FN(OLDCOMM, KEYVAL, EXTRA_STATE,&\n'
'		ATTRIBUTE_VAL_IN, ATTRIBUTE_VAL_OUT, FLAG, IERROR)\n'
'	USE MPI_CONSTANTS,ONLY: MPI_ADDRESS_KIND\n'
'	INTEGER OLDCOMM, KEYVAL, IERROR\n'
'	INTEGER(KIND=MPI_ADDRESS_KIND) EXTRA_STATE, ATTRIBUTE_VAL_IN, ATTRIBUTE_VAL_OUT\n'
'	LOGICAL FLAG\n'
'END SUBROUTINE MPI_DUP_FN\n'
'\n'
'SUBROUTINE MPI_NULL_COPY_FN(OLDCOMM, KEYVAL, EXTRA_STATE,&\n'
'		ATTRIBUTE_VAL_IN, ATTRIBUTE_VAL_OUT, FLAG, IERROR)\n'
'	USE MPI_CONSTANTS,ONLY: MPI_ADDRESS_KIND\n'
'	INTEGER OLDCOMM, KEYVAL, IERROR\n'
'	INTEGER(KIND=MPI_ADDRESS_KIND) EXTRA_STATE, ATTRIBUTE_VAL_IN, ATTRIBUTE_VAL_OUT\n'
'	LOGICAL FLAG\n'
'END SUBROUTINE MPI_NULL_COPY_FN\n'
'\n'
'SUBROUTINE MPI_COMM_NULL_DELETE_FN(COMM, COMM_KEYVAL, ATTRIBUTE_VAL,&\n'
'		EXTRA_STATE, IERROR)\n'
'	USE MPI_CONSTANTS,ONLY: MPI_ADDRESS_KIND\n'
'	INTEGER COMM, COMM_KEYVAL, IERROR\n'
'	INTEGER(KIND=MPI_ADDRESS_KIND) ATTRIBUTE_VAL, EXTRA_STATE\n'
'END SUBROUTINE MPI_COMM_NULL_DELETE_FN\n'
'\n'
'SUBROUTINE MPI_COMM_DUP_FN(OLDCOMM, COMM_KEYVAL, EXTRA_STATE,&\n'
'		ATTRIBUTE_VAL_IN, ATTRIBUTE_VAL_OUT, FLAG, IERROR)\n'
'	USE MPI_CONSTANTS,ONLY: MPI_ADDRESS_KIND\n'
'	INTEGER OLDCOMM, COMM_KEYVAL, IERROR\n'
'	INTEGER(KIND=MPI_ADDRESS_KIND) EXTRA_STATE, ATTRIBUTE_VAL_IN, ATTRIBUTE_VAL_OUT\n'
'	LOGICAL FLAG\n'
'END SUBROUTINE MPI_COMM_DUP_FN\n'
'\n'
'SUBROUTINE MPI_COMM_NULL_COPY_FN(OLDCOMM, COMM_KEYVAL, EXTRA_STATE,&\n'
'		ATTRIBUTE_VAL_IN, ATTRIBUTE_VAL_OUT, FLAG, IERROR)\n'
'	USE MPI_CONSTANTS,ONLY: MPI_ADDRESS_KIND\n'
'	INTEGER OLDCOMM, COMM_KEYVAL, IERROR\n'
'	INTEGER(KIND=MPI_ADDRESS_KIND) EXTRA_STATE, ATTRIBUTE_VAL_IN, ATTRIBUTE_VAL_OUT\n'
'	LOGICAL FLAG\n'
'END SUBROUTINE MPI_COMM_NULL_COPY_FN\n'
'\n'
'SUBROUTINE MPI_TYPE_NULL_DELETE_FN(DATATYPE, TYPE_KEYVAL, ATTRIBUTE_VAL,&\n'
'		EXTRA_STATE, IERROR)\n'
'	USE MPI_CONSTANTS,ONLY: MPI_ADDRESS_KIND\n'
'	INTEGER DATATYPE, TYPE_KEYVAL, IERROR\n'
'	INTEGER(KIND=MPI_ADDRESS_KIND) ATTRIBUTE_VAL, EXTRA_STATE\n'
'END SUBROUTINE MPI_TYPE_NULL_DELETE_FN\n'
'\n'
'SUBROUTINE MPI_TYPE_DUP_FN(OLDTYPE, TYPE_KEYVAL, EXTRA_STATE,&\n'
'		ATTRIBUTE_VAL_IN, ATTRIBUTE_VAL_OUT, FLAG, IERROR)\n'
'	USE MPI_CONSTANTS,ONLY: MPI_ADDRESS_KIND\n'
'	INTEGER OLDTYPE, TYPE_KEYVAL, IERROR\n'
'	INTEGER(KIND=MPI_ADDRESS_KIND) EXTRA_STATE, ATTRIBUTE_VAL_IN, ATTRIBUTE_VAL_OUT\n'
'	LOGICAL FLAG\n'
'END SUBROUTINE MPI_TYPE_DUP_FN\n'
'\n'
'SUBROUTINE MPI_TYPE_NULL_COPY_FN(OLDTYPE, TYPE_KEYVAL, EXTRA_STATE,&\n'
'		ATTRIBUTE_VAL_IN, ATTRIBUTE_VAL_OUT, FLAG, IERROR)\n'
'USE MPI_CONSTANTS,ONLY: MPI_ADDRESS_KIND\n'
'	INTEGER OLDTYPE, TYPE_KEYVAL, IERROR\n'
'	INTEGER(KIND=MPI_ADDRESS_KIND) EXTRA_STATE, ATTRIBUTE_VAL_IN, ATTRIBUTE_VAL_OUT\n'
'	LOGICAL FLAG\n'
'END SUBROUTINE MPI_TYPE_NULL_COPY_FN\n'
'\n'
'SUBROUTINE MPI_WIN_NULL_DELETE_FN(WIN, WIN_KEYVAL, ATTRIBUTE_VAL,&\n'
'		EXTRA_STATE, IERROR)\n'
'	USE MPI_CONSTANTS,ONLY: MPI_ADDRESS_KIND\n'
'	INTEGER WIN, WIN_KEYVAL, IERROR\n'
'	INTEGER(KIND=MPI_ADDRESS_KIND) ATTRIBUTE_VAL, EXTRA_STATE\n'
'END SUBROUTINE MPI_WIN_NULL_DELETE_FN\n'
'\n'
'SUBROUTINE MPI_WIN_DUP_FN(OLDWIN, WIN_KEYVAL, EXTRA_STATE,&\n'
'		ATTRIBUTE_VAL_IN, ATTRIBUTE_VAL_OUT, FLAG, IERROR)\n'
'	USE MPI_CONSTANTS,ONLY: MPI_ADDRESS_KIND\n'
'	INTEGER OLDWIN, WIN_KEYVAL, IERROR\n'
'	INTEGER(KIND=MPI_ADDRESS_KIND) EXTRA_STATE, ATTRIBUTE_VAL_IN, ATTRIBUTE_VAL_OUT\n'
'	LOGICAL FLAG\n'
'END SUBROUTINE MPI_WIN_DUP_FN\n'
'\n'
'SUBROUTINE MPI_WIN_NULL_COPY_FN(OLDWIN, WIN_KEYVAL, EXTRA_STATE,&\n'
'		ATTRIBUTE_VAL_IN, ATTRIBUTE_VAL_OUT, FLAG, IERROR)\n'
'	USE MPI_CONSTANTS,ONLY: MPI_ADDRESS_KIND\n'
'	INTEGER OLDWIN, WIN_KEYVAL, IERROR\n'
'	INTEGER(KIND=MPI_ADDRESS_KIND) EXTRA_STATE, ATTRIBUTE_VAL_IN, ATTRIBUTE_VAL_OUT\n'
'	LOGICAL FLAG\n'
'END SUBROUTINE MPI_WIN_NULL_COPY_FN\n'
);



"""
 Write Interface
"""


print "Processing MPI Functions\n"


for f in mpi_interface:
	functionName = f;
	capsfunctioname = f.upper();
	
	names = [];
	types = [];
	wasprefixed = [];
	
	uses = [];
	
	print "\tGEN\t" + functionName + "..."
	
	for arg in mpi_interface[ functionName ]:
		name = arg[1];
		ctype = arg[0];
		ftype = typesc2f.get( ctype );
		
		if ftype == None:
			#Type can't be converted to Fortran ABORT
			ftype = "NONE"
			print "\t ERROR failed to translate :",  ctype + " " + name + "==> " + ftype 
			exit(1);
		
		#If we are here we managed to do the type translation
		
		#Make sure the name is not a prefix in the type
		if "!!VAR!!" in ftype:
			ftype = ftype.replace("!!VAR!!", name)
			wasprefixed.append( 1 );
		else:
			wasprefixed.append( 0 );
		
		
		types.append( ftype );
		names.append( name );
		
		#Check for constants
		if "MPI_ADDRESS_KIND" in ftype:
			uses.append("MPI_ADDRESS_KIND");
		if "MPI_COUNT_KIND" in ftype:
			uses.append("MPI_COUNT_KIND");
		if "MPI_STATUS_SIZE" in ftype:
			uses.append("MPI_STATUS_SIZE");
		if "MPI_OFFSET_KIND" in ftype:
			uses.append("MPI_OFFSET_KIND");
		
		
		
	
	#Emit the subroutine code
	module_file_data+= "\n\nSUBROUTINE " + capsfunctioname +"(";
	#FUCTION FOOTPRINT
	for i in range(0,len(names)):
		module_file_data+= names[i];
		if i < (len(names) - 1) :
			module_file_data += ","
			if (((i+1)%3) == 0) and (i):
				module_file_data+= "&\n\t"
	module_file_data+=")\n"
	#USE CONSTANTS IF NEEDED
	#Hacky way to get unique elements
	uses = list( set( uses ));
	if len(uses):
		string_uses = ""
		for i in range(0,len(uses)):
			string_uses += uses[i]
			if i < (len(uses) -1):
				string_uses += ","
		module_file_data+= "\tUSE MPI_CONSTANTS,ONLY: " + string_uses + "\n"
	
	#DUMP ARGS
	if len(names) != len(types) or  len(types) != len(wasprefixed):
		print "SOMETHING WENT WRONG with argument manipulation in " + functionName + " ==> " + names + types

	for i in range( 0, len( names ) ):
		if wasprefixed[i]:
			module_file_data+= "\t" + types[i] + "\n";
		else:
			module_file_data+= "\t" + types[i] + "\t" + names[i] + "\n";


	module_file_data+= "END SUBROUTINE " + capsfunctioname + "\n"

module_file_data += "\n\nEND INTERFACE\n\n";

"""
 Write Module END
"""

module_file_data +="END MODULE MPI_BASE\n";


"""
 Open Output Module FILE
"""

f = open("mpi_base.f90", "w" )

f.write( module_file_data );

f.close()

#
#
#  THIS IS THE MPI FORTRAN MODULE
#
#

print "\n################################"
print "Generating the MPI Fortran Module"
print "################################\n"


module_file_data = genheader()


"""
 Write Module START
"""

module_file_data +=("MODULE MPI\n\n");


module_file_data += "USE MPI_BASE\n"
module_file_data += "USE MPI_CONSTANTS\n"


"""
 Write Module END
"""

module_file_data +="\nEND MODULE MPI\n";


"""
 Open Output Module FILE
"""

f = open("mpi.f90", "w" )

f.write( module_file_data );

f.close()
