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


/* 8 bits uinteger */
static inline void uinteger_8_16( uint8_t in, uint16_t * out )
{
	*out = in;
}

static inline void uinteger_8_32( uint8_t in, uint16_t * out )
{
	*out = in;
}

static inline void uinteger_8_64( uint8_t in, uint16_t * out )
{
	*out = in;
}

static inline void uinteger_8_convert( uint8_t in, void * out, size_t target )
{
	
	switch( target )
	{
		case 16:
			uinteger_8_16( in, (uint16_t *)out );
		break;
		case 32:
			uinteger_8_32( in, (uint32_t *)out );
		break;
		case 64:
			uinteger_8_64( in, (uint64_t *)out );
		break;
		default:
			sctk_fatal("No such converter 8 to %d", target );
	}	
}



/* 16 bits uinteger */
static inline void uinteger_16_8( uint16_t in, uint8_t * out )
{
	*out = in;
}

static inline void uinteger_16_32( uint16_t in, uint32_t * out )
{
	*out = in;
}

static inline void uinteger_16_64( uint16_t in, uint64_t * out )
{
	*out = in;
}

static inline void uinteger_16_convert( uint16_t in, void * out, size_t target )
{
	
	switch( target )
	{
		case 8:
			uinteger_16_8( in, (uint8_t *)out );
		break;
		case 32:
			uinteger_16_32( in, (uint32_t *)out );
		break;
		case 64:
			uinteger_16_64( in, (uint64_t *)out );
		break;
		default:
			sctk_fatal("No such converter 16 to %d", target );
	}	
}

/* 32 bits uinteger */
static inline void uinteger_32_8( uint32_t in, uint8_t * out )
{
	*out = in;
}

static inline void uinteger_32_16( uint32_t in, uint16_t * out )
{
	*out = in;
}

static inline void uinteger_32_64( uint32_t in, uint64_t * out )
{
	*out = in;
}

static inline void uinteger_32_convert( uint32_t in, void * out, size_t target )
{
	
	switch( target )
	{
		case 8:
			uinteger_32_8( in, (uint8_t *)out );
		break;
		case 16:
			uinteger_32_16( in, (uint16_t *)out );
		break;
		case 64:
			uinteger_32_64( in, (uint64_t *)out );
		break;
		default:
			sctk_fatal("No such converter 16 to %d", target );
	}	
}

/* 64 bits uinteger */
static inline void uinteger_64_8( uint64_t in, uint8_t * out )
{
	*out = in;
}

static inline void uinteger_64_16( uint64_t in, uint16_t * out )
{
	*out = in;
}

static inline void uinteger_64_32( uint64_t in, uint32_t * out )
{
	*out = in;
}

static inline void uinteger_64_convert( uint64_t in, void * out, size_t target )
{
	
	switch( target )
	{
		case 8:
			uinteger_64_8( in, (uint8_t *)out );
		break;
		case 16:
			uinteger_64_16( in, (uint16_t *)out );
		break;
		case 32:
			uinteger_64_32( in, (uint32_t *)out );
		break;
		default:
			sctk_fatal("No such converter 16 to %d", target );
	}	
}

/* 8 bits integer */
static inline void integer_8_16( int8_t in, int16_t * out )
{
	*out = in;
}

static inline void integer_8_32( int8_t in, int16_t * out )
{
	*out = in;
}

static inline void integer_8_64( int8_t in, int16_t * out )
{
	*out = in;
}

static inline void integer_8_convert( int8_t in, void * out, size_t target )
{
	
	switch( target )
	{
		case 16:
			integer_8_16( in, (int16_t *)out );
		break;
		case 32:
			integer_8_32( in, (int32_t *)out );
		break;
		case 64:
			integer_8_64( in, (int64_t *)out );
		break;
		default:
			sctk_fatal("No such converter 8 to %d", target );
	}	
}



/* 16 bits integer */
static inline void integer_16_8( int16_t in, int8_t * out )
{
	*out = in;
}

static inline void integer_16_32( int16_t in, int32_t * out )
{
	*out = in;
}

static inline void integer_16_64( int16_t in, int64_t * out )
{
	*out = in;
}

static inline void integer_16_convert( int16_t in, void * out, size_t target )
{
	
	switch( target )
	{
		case 8:
			integer_16_8( in, (int8_t *)out );
		break;
		case 32:
			integer_16_32( in, (int32_t *)out );
		break;
		case 64:
			integer_16_64( in, (int64_t *)out );
		break;
		default:
			sctk_fatal("No such converter 16 to %d", target );
	}	
}

/* 32 bits integer */
static inline void integer_32_8( int32_t in, int8_t * out )
{
	*out = in;
}

static inline void integer_32_16( int32_t in, int16_t * out )
{
	*out = in;
}

static inline void integer_32_64( int32_t in, int64_t * out )
{
	*out = in;
}

static inline void integer_32_convert( int32_t in, void * out, size_t target )
{
	
	switch( target )
	{
		case 8:
			integer_32_8( in, (int8_t *)out );
		break;
		case 16:
			integer_32_16( in, (int16_t *)out );
		break;
		case 64:
			integer_32_64( in, (int64_t *)out );
		break;
		default:
			sctk_fatal("No such converter 16 to %d", target );
	}	
}

/* 64 bits integer */
static inline void integer_64_8( int64_t in, int8_t * out )
{
	*out = in;
}

static inline void integer_64_16( int64_t in, int16_t * out )
{
	*out = in;
}

static inline void integer_64_32( int64_t in, int32_t * out )
{
	*out = in;
}

static inline void integer_64_convert( int64_t in, void * out, size_t target )
{
	
	switch( target )
	{
		case 8:
			integer_64_8( in, (int8_t *)out );
		break;
		case 16:
			integer_64_16( in, (int16_t *)out );
		break;
		case 32:
			integer_64_32( in, (int32_t *)out );
		break;
		default:
			sctk_fatal("No such converter 16 to %d", target );
	}	
}




/* integer converter */
void sctk_integer_convert_width( void * in, size_t in_size, int unsgn, void * out, size_t out_size )
{
	if( in_size == out_size )
	{
		/* Noting to do */
		memcpy( out, in, in_size );
		return;
	}
	
	if( unsgn)
	{
		switch( in_size )
		{
			case 8:
				uinteger_8_convert( *( (uint8_t*) in), out, out_size );
			break;
			case 16:
				uinteger_16_convert( *( (uint16_t*) in), out, out_size );
			break;
			case 32:
				uinteger_32_convert( *( (uint32_t*) in), out, out_size );
			break;
			case 64:
				uinteger_64_convert( *( (uint64_t*) in), out, out_size );
			break;
		}
	}
	else
	{
		switch( in_size )
		{
			case 8:
				integer_8_convert( *( (uint8_t*) in), out, out_size );
			break;
			case 16:
				integer_16_convert( *( (uint16_t*) in), out, out_size );
			break;
			case 32:
				integer_32_convert( *( (uint32_t*) in), out, out_size );
			break;
			case 64:
				integer_64_convert( *( (uint64_t*) in), out, out_size );
			break;
		}
	}
	
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

