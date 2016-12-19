#!/usr/bin/env python

from xml.dom import minidom
import sys

if len(sys.argv) != 2:
	sys.stderr.write("Error please provide the mpitxml file\n")
	exit(1)

xml_file = sys.argv[1]


doc = minidom.parse(xml_file)


#
# CATEGORIES
#


#Key points to parent category
categories={}
cvar_categories=[]
categories_desc={}

def get_cvar_cat():
	l = len(cvar_categories)
	if l == 0:
		return ""
	else:
		return cvar_categories[l-1]

def get_cvar_pcat():
	l = len(cvar_categories)
	if l < 2:
		return "vars"
	else:
		return cvar_categories[l-2]

def enter_cat( cat , desc = ""):
	if cat == "#text":
		return
	cvar_categories.append( cat )
	ec = categories.get( cat );
	if ec != None:
		if ec != get_cvar_pcat():
			sys.stderr.write( "Error Var category " + cat + " exists in different contexts " + get_cvar_pcat() + " and " + ec +"\n" )
			exit(1)
	else:
		sys.stderr.write( "New category " + cat + " in " + get_cvar_pcat() + "\n" )
		categories[cat]=get_cvar_pcat()
		categories_desc[cat] = desc


def leave_cat( cat ):
	if cat == "#text":
		return
	cvar_categories.pop()


def reset_cat():
	cvar_categories=[]


#
# KEYS
#

key_array=[]

def get_key( name ):
	for i in range(0, len(key_array)):
		if key_array[i] == name:
			return 1
	
	return 0

def reg_key( name ):
	key_array.append( name )

#
# CVARS WALK
#

cvar_array=[]


def check_verbosity( verb ):
	if verb.find("MPI_T_VERBOSITY_") == -1:
		return 1
	a = verb.replace("MPI_T_VERBOSITY_","")
	if a in ["USER_BASIC", "USER_DETAIL", "USER_ALL", "TUNER_BASIC", "TUNER_DETAIL", "TUNER_ALL", "MPIDEV_BASIC", "MPIDEV_DETAIL", "MPIDEV_ALL"]:
		return 0
	return 1

def check_datatype( verb ):
	if verb.find("MPI_") == -1:
		return 1
	a = verb.replace("MPI_","")
	if a in ["INT", "UNSIGNED", "UNSIGNED_LONG", "UNSIGNED_LONG_LONG", "COUNT", "CHAR", "DOUBLE"]:
		return 0
	return 1



def check_bind( verb ):
	if verb.find("MPI_T_BIND_") == -1:
		return 1
	a = verb.replace("MPI_T_BIND_","")
	if a in ["NO_OBJECT", "MPI_COMM", "MPI_DATATYPE", "MPI_ERRHANDLER", "MPI_FILE", "MPI_GROUP", "MPI_OP","MPI_REQUEST", "MPI_WIN", "MPI_MESSAGE", "MPI_INFO"]:
		return 0
	return 1


def check_scope_cvar( verb ):
	if verb.find("MPI_T_SCOPE_") == -1:
		return 1
	a = verb.replace("MPI_T_SCOPE_","")
	if a in ["CONSTANT", "READONLY", "LOCAL", "GROUP", "GROUP_EQ", "ALL", "ALL_EQ"]:
		return 0
	return 1



def new_cvar( node ):
	ret={}
	name = node.getElementsByTagName("name")
	
	if name:
		ret["name"]=name[0].firstChild.nodeValue
		if get_key( ret["name"] ):
			sys.stderr.write( "ERROR : " + ret["name"] + " is a duplicate MPIT VAR key\n" )
		else:
			reg_key( ret["name"] )
	else:
		sys.stderr.write( "ERROR cvars must have a name\n")
		exit(1)
	
	sys.stderr.write( "CVAR '" + ret["name"] + "' in category "+ get_cvar_cat() + " context " + get_cvar_pcat() + "\n")
	
	verbosity = node.getElementsByTagName("verbosity")
	
	if verbosity:
		ret["verbosity"]=verbosity[0].firstChild.nodeValue
		if check_verbosity(ret["verbosity"]):
			sys.stderr.write( "ERROR : " + ret["verbosity"] + " is invalid value for verbosity\n")
			exit(1)
	else:
		sys.stderr.write( "ERROR cvars must have a verbosity(" +  ret["name"] + ")\n")
		exit(1)	

	
	datatype = node.getElementsByTagName("datatype")
		
	if datatype:
		ret["datatype"]=datatype[0].firstChild.nodeValue
		if check_datatype(ret["datatype"]):
			sys.stderr.write( "ERROR : " + ret["datatype"] + " is invalid value for datatype\n")
			exit(1)
	else:
		sys.stderr.write( "ERROR cvars must have a datatype(" +  ret["name"] + ")\n")
		exit(1)	

	desc = node.getElementsByTagName("desc")
	
	if desc:
		ret["desc"]=desc[0].firstChild.nodeValue
	else:
		sys.stderr.write( "ERROR cvars must have a desc(" +  ret["name"] + ")\n")
		exit(1)

	bind = node.getElementsByTagName("bind")
		
	if bind:
		ret["bind"]=bind[0].firstChild.nodeValue
		if check_bind(ret["bind"]):
			sys.stderr.write( "ERROR : " + ret["bind"] + " is invalid value for bind\n")
			exit(1)
	else:
		sys.stderr.write( "ERROR cvars must have a bind(" +  ret["name"] + ")\n")
		exit(1)	
	
	scope = node.getElementsByTagName("scope")
		
	if scope:
		ret["scope"]=scope[0].firstChild.nodeValue
		if check_scope_cvar(ret["scope"]):
			sys.stderr.write( "ERROR : " + ret["scope"] + " is invalid value for scope\n")
			exit(1)
	else:
		sys.stderr.write( "ERROR cvars must have a scope(" +  ret["name"] + ")\n")
		exit(1)	
	
	ret["category"] = get_cvar_cat()
	ret["parent_category"] = get_cvar_pcat()
	cvar_array.append(ret)
	

def unfold_cvar( node ):
	name = node.nodeName
	if node.attributes:
		ndesc = node.attributes.get("desc")
		if ndesc:
			desc = ndesc.value
		else:
			desc = ""
	else:
		desc = ""
	if name == "cvar":
		new_cvar(node)
	else:
		#Set context as category */
		enter_cat(name,desc)
		if node.hasChildNodes():
			child = node.childNodes
			for c in child:
				unfold_cvar( c )
		leave_cat(name)
		
# Process CVARS

pvar_array=[]


def check_01_pvar( ro ):
	if ro in ["0", "1"]:
		return 0
	return 1


def check_class( eclass ):
	if eclass.find("MPI_T_PVAR_CLASS_") == -1:
		return 1
	a = eclass.replace("MPI_T_PVAR_CLASS_","")
	if a in ["STATE", "LEVEL", "SIZE", "PERCENTAGE", "HIGHWATERMARK", "LOWWATERMARK", "COUNTER", "AGGREGATE", "TIMER", "GENERIC"]:
		return 0
	return 1



def new_pvar( node ):
	ret={}
	name = node.getElementsByTagName("name")
	
	if name:
		ret["name"]=name[0].firstChild.nodeValue
		if get_key( ret["name"] ):
			sys.stderr.write( "ERROR : " + ret["name"] + " is a duplicate MPIT VAR key\n")
		else:
			reg_key( ret["name"] )
	else:
		sys.stderr.write( "ERROR pvars must have a name\n")
		exit(1)
	
	sys.stderr.write( "PVAR '" + ret["name"] + "' in category "+ get_cvar_cat() + " context " + get_cvar_pcat() + "\n")
	
	verbosity = node.getElementsByTagName("verbosity")
	
	if verbosity:
		ret["verbosity"]=verbosity[0].firstChild.nodeValue
		if check_verbosity(ret["verbosity"]):
			sys.stderr.write( "ERROR : " + ret["verbosity"] + " is invalid value for verbosity\n")
			exit(1)
	else:
		sys.stderr.write( "ERROR pvars must have a verbosity(" +  ret["name"] + ")\n")
		exit(1)	

	
	eclass = node.getElementsByTagName("class")
		
	if eclass:
		ret["class"]=eclass[0].firstChild.nodeValue
		if check_class(ret["class"]):
			sys.stderr.write( "ERROR : " + ret["class"] + " is invalid value for class\n")
			exit(1)
	else:
		sys.stderr.write( "ERROR pvars must have a class(" +  ret["name"] + ")\n")
		exit(1)	

	
	datatype = node.getElementsByTagName("datatype")
		
	if datatype:
		ret["datatype"]=datatype[0].firstChild.nodeValue
		if check_datatype(ret["datatype"]):
			sys.stderr.write( "ERROR : " + ret["datatype"] + " is invalid value for datatype\n")
			exit(1)
	else:
		sys.stderr.write( "ERROR pvars must have a datatype(" +  ret["name"] + ")\n")
		exit(1)	

	desc = node.getElementsByTagName("desc")
	
	if desc:
		ret["desc"]=desc[0].firstChild.nodeValue
	else:
		sys.stderr.write( "ERROR pvars must have a desc(" +  ret["name"] + ")\n")
		exit(1)

	bind = node.getElementsByTagName("bind")
		
	if bind:
		ret["bind"]=bind[0].firstChild.nodeValue
		if check_bind(ret["bind"]):
			sys.stderr.write( "ERROR : " + ret["bind"] + " is invalid value for bind\n")
			exit(1)
	else:
		sys.stderr.write( "ERROR pvars must have a bind(" +  ret["name"] + ")\n")
		exit(1)	

	readonly = node.getElementsByTagName("readonly")
		
	if readonly:
		ret["readonly"]=readonly[0].firstChild.nodeValue
		if check_01_pvar(ret["readonly"]):
			sys.stderr.write( "ERROR : " + ret["readonly"] + " is invalid value for readonly\n")
			exit(1)
	else:
		sys.stderr.write( "ERROR pvars must have a readonly(" +  ret["name"] + ")\n")
		exit(1)	
	
	continuous = node.getElementsByTagName("continuous")	
		
	if continuous:
		ret["continuous"]=continuous[0].firstChild.nodeValue
		if check_01_pvar(ret["continuous"]):
			sys.stderr.write( "ERROR : " + ret["continuous"] + " is invalid value for continuous\n")
			exit(1)
	else:
		sys.stderr.write( "ERROR pvars must have a continuous(" +  ret["name"] + ")\n")
		exit(1)	
	
	atomic = node.getElementsByTagName("atomic")	
		
	if atomic:
		ret["atomic"]=atomic[0].firstChild.nodeValue
		if check_01_pvar(ret["atomic"]):
			sys.stderr.write( "ERROR : " + ret["atomic"] + " is invalid value for atomic\n")
			exit(1)
	else:
		sys.stderr.write( "ERROR pvars must have a atomic(" +  ret["name"] + ")")
		exit(1)	
	
	ret["category"] = get_cvar_cat()
	ret["parent_category"] = get_cvar_pcat()
	pvar_array.append(ret)

def unfold_pvar( node ):
	name = node.nodeName
	if node.attributes:
		ndesc = node.attributes.get("desc")
		if ndesc:
			desc = ndesc.value
		else:
			desc = ""
	else:
		desc = ""
	if name == "pvar":
		new_pvar(node)
	else:
		#Set context as category */
		enter_cat(name, desc)
		if node.hasChildNodes():
			child = node.childNodes
			for c in child:
				unfold_pvar( c )
		leave_cat(name)


cvars_nodes=doc.getElementsByTagName("cvars")
reset_cat()
for cvars in cvars_nodes:
    unfold_cvar( cvars )
    
# Process PVARS

pvars_nodes=doc.getElementsByTagName("pvars")
reset_cat()
for pvars in pvars_nodes:
    unfold_pvar( pvars )
   
   
#DOGEN

print """
/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - /!\THIS FILE IS GENERATED BY ./MPC_Tools/mpc_gen_mpit            # */
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */

/*
 *
 * NEVER EDIT THIS FILE EDIT THE mpit.xml in your module instead
 * And then run ./MPC_Tools/mpc_gen_mpit
 *
 */



















 /* As you read this, did I tell you that this file is auto-generated ? ;-) */

"""

#NOW LIST CATEGORIES

print "/* Listing Categories */"

for c in categories:
	print "CATEGORIES(" + c.upper() + "," + categories[c].upper() + ", \"" + categories_desc[c] + "\" )"

print "/* Listing CVARS */"

for i in range(0,len(cvar_array)):
	cvar = cvar_array[i]
	print "CVAR(" + cvar["name"] + "," + cvar["verbosity"] + "," + cvar["datatype"] + ", \"" + cvar["desc"] + "\", " +  cvar["bind"] + ", " + cvar["scope"] + ", " + cvar["category"].upper() + " )"

print "/* Listing PVARS */"

for i in range(0,len(pvar_array)):
	pvar = pvar_array[i]
	print "PVAR(" + pvar["name"] + "," + pvar["verbosity"]+ "," + pvar["class"] + "," + pvar["datatype"] + ", \"" + pvar["desc"] + "\", " +  pvar["bind"] + ", " + pvar["readonly"]  + ", " + pvar["continuous"]  + ", " + pvar["atomic"]  + ", " + pvar["category"].upper() + " )"

