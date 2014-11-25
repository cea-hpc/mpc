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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#include "mpc_extern32.h"


static inline int sctk_is_float_datatype( MPC_Datatype type)
{
    return ((type == MPC_FLOAT) || 
	    (type == MPC_DOUBLE) ||
            (type == MPC_LONG_DOUBLE) ||
            (type == MPC_DOUBLE_PRECISION) ||
            (type == MPC_COMPLEX) ||
            (type == MPC_DOUBLE_COMPLEX) ||
            (type == MPC_REAL4) ||
            (type == MPC_REAL8) || 
            (type == MPC_REAL16));
}



static inline void MPC_Extern32_convert( MPC_Datatype type, char * inout )
{
	if( !sctk_datatype_is_common(type) )
	{
		MPCX_Type_debug( type );
		sctk_fatal("Cannot convert a non-common data-type");
	}
	
	size_t type_size = sctk_common_datatype_get_size( type );
	
	char tmpbuff[512];
	assume( type_size < 512 );
	memcpy( tmpbuff, inout, type_size );
	
	if( sctk_is_float_datatype( type ) )
	{
		FLOAT_convert(type_size, tmpbuff, inout);
	}
	else
	{
		BASIC_convert(type_size, tmpbuff, inout);
	}
}

void MPC_Extern32_encode( MPC_Datatype * typevector , int type_vector_size, char * buff, MPC_Aint max_size )
{
	int typeIdx = 0;
	MPC_Aint current_offset = 0;
	
	char * current_entry = buff;
	
	while( 1 )
	{
		MPC_Datatype current_type = typevector[ typeIdx ];
		typeIdx = ( typeIdx + 1 ) % type_vector_size;
		
		/* Apply conversion */
		MPC_Extern32_convert( current_type, current_entry );
		
		current_offset += sctk_common_datatype_get_size( current_type );
		current_entry = buff + current_offset;
		
		if( max_size <= current_offset )
			break;
	}
	
	
	
	
}

