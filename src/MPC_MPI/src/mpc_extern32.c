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


static void FLOAT_convert(int type_byte_size, char *src, char *dest) {
  if (IS_BIG_ENDIAN == 0) {
    switch (type_byte_size) {
    case 4: {
      long d;
      BASIC_convert32(src, (char *)&d);
      *((float *)dest) = (float)d;
    } break;
    case 8: {
      BASIC_convert64(src, dest);
      break;
    }
    case 12: {
      BASIC_convert96(src, dest);
      break;
    }
    case 16: {
      BASIC_convert128(src, dest);
    } break;
    }
  } else {
    memcpy(dest, src, type_byte_size);
  }
}

static inline int sctk_is_float_datatype( mpc_lowcomm_datatype_t type)
{
    return ((type == MPC_LOWCOMM_FLOAT) || 
	    (type == MPC_LOWCOMM_DOUBLE) ||
            (type == MPC_LOWCOMM_LONG_DOUBLE) ||
            (type == MPC_LOWCOMM_DOUBLE_PRECISION) ||
            (type == MPC_COMPLEX) ||
            (type == MPC_DOUBLE_COMPLEX) ||
            (type == MPC_LOWCOMM_REAL4) ||
            (type == MPC_LOWCOMM_REAL8) || 
            (type == MPC_LOWCOMM_REAL16));
}


/* 8 bits uinteger */
static inline void uinteger_8_16( uint8_t in, uint16_t * out )
{
	*out = in;
}

static inline void uinteger_8_32( uint8_t in, uint32_t * out )
{
	*out = in;
}

static inline void uinteger_8_64( uint8_t in, uint64_t * out )
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
			mpc_common_debug_fatal("No such converter 8 to %d", target );
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
			mpc_common_debug_fatal("No such converter 16 to %d", target );
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
			mpc_common_debug_fatal("No such converter 16 to %d", target );
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
			mpc_common_debug_fatal("No such converter 16 to %d", target );
	}	
}

/* 8 bits integer */
static inline void integer_8_16( int8_t in, int16_t * out )
{
	*out = in;
}

static inline void integer_8_32( int8_t in, int32_t * out )
{
	*out = in;
}

static inline void integer_8_64( int8_t in, int64_t * out )
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
			mpc_common_debug_fatal("No such converter 8 to %d", target );
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
			mpc_common_debug_fatal("No such converter 16 to %d", target );
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
			mpc_common_debug_fatal("No such converter 16 to %d", target );
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
			mpc_common_debug_fatal("No such converter 16 to %d", target );
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




static inline void MPC_Extern32_encode( mpc_lowcomm_datatype_t type, char * in, char * out )
{
	if( !mpc_lowcomm_datatype_is_common(type) )
	{
		mpc_common_debug_fatal("Cannot convert a non-common data-type");
	}
	
	/* Prepare for width Conversion */
	size_t type_size = mpc_lowcomm_datatype_common_get_size( type );
	size_t type_extern_size = MPC_Extern32_common_type_size( type );
	
	if( type_size != type_extern_size )
	{
		if( sctk_is_float_datatype( type ) )
		{
			mpc_common_debug_warning("UNEXPECTED: Float size conversion %d -> %d", type_size, type_extern_size );
		}
		else
		{
			sctk_integer_convert_width( (void *) in, type_size*8, MPC_Unsigned_type( type ), (void *)out, type_extern_size*8 );
		}
	}
	else
	{
		memcpy( out, in, type_extern_size );
	}
	
	/* At this point data should be in the OUT buffer */
	
	/* Put in a tmp buff on stack */
	char tmpbuff[512];
	assume( type_size < 512 );
	memcpy( tmpbuff, out, type_extern_size );
	
	/* Apply indianess conversion on the OUT buffer (on the extern type) */
	if( sctk_is_float_datatype( type ) )
	{
		FLOAT_convert(type_extern_size, tmpbuff, out);
	}
	else
	{
		BASIC_convert(type_extern_size, tmpbuff, out);
	}
}

static inline void MPC_Extern32_decode( mpc_lowcomm_datatype_t type, char * in, char * out )
{
	
	size_t type_size = mpc_lowcomm_datatype_common_get_size( type );
	size_t type_extern_size = MPC_Extern32_common_type_size( type );

	/* Apply indianess conversion and store in OUT */
	if( sctk_is_float_datatype( type ) )
	{
		FLOAT_convert(type_extern_size, in, out);
	}
	else
	{
		BASIC_convert(type_extern_size, in, out);
	}	
	
	/* Prepare for width Conversion */
	char tmpbuff[512];
	assume( type_extern_size < 512 );

			
	if( type_size != type_extern_size )
	{
		if( sctk_is_float_datatype( type ) )
		{
			mpc_common_debug_warning("UNEXPECTED: Float size conversion %d -> %d", type_size, type_extern_size );
		}
		else
		{
			memcpy( tmpbuff, out, type_extern_size );
			sctk_integer_convert_width( (void *) tmpbuff, type_extern_size*8, MPC_Unsigned_type( type ), (void *)out,  type_size*8);
		}
	}
}



void MPC_Extern32_convert( mpc_lowcomm_datatype_t * typevector ,
						   int type_vector_size, 
						   char * native_buff, 
						   size_t max_native_size, 
						   char * extern_buff, 
						   size_t max_extern_size , 
						   int encode )
{
	int typeIdx = 0;
	size_t current_offset = 0;
	size_t extern_offset = 0;
	
	char * current_entry = native_buff;
	char * current_extern_entry = extern_buff;
	
	while( 1 )
	{
		mpc_lowcomm_datatype_t current_type = typevector[ typeIdx ];
		typeIdx = ( typeIdx + 1 ) % type_vector_size;
		
		/* Apply conversion */
		
		if( encode )
		{
			MPC_Extern32_encode( current_type, current_entry, current_extern_entry);
		}
		else
		{
			MPC_Extern32_decode( current_type, current_extern_entry, current_entry );
		}
		
		/* Move in the input */
		current_offset += mpc_lowcomm_datatype_common_get_size( current_type );
		current_entry = native_buff + current_offset;
		
		/* Move in the output */
		extern_offset += MPC_Extern32_common_type_size( typevector[ typeIdx ] );
		current_extern_entry = (void*)(extern_buff + extern_offset);
	
		
		if( max_native_size <= current_offset )
			break;
	
		if( max_extern_size <= extern_offset )
			break;
	}
}


