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

#ifndef MPC_EXTERN32_H
#define MPC_EXTERN32_H

#include <string.h>
#include "mpcmp.h"
#include "mpc_common.h"
#include "sctk_stdint.h"
#include "mpc_datatypes.h"

/* This file defines the extern32 data-rep following
 * MPI standard 1.3: 13.5.2 External Data Representation: “external32”
 * page 534. We start by defining all the conversion functions
 * before applying them to flattened representations
 * 
 * Note that user defined data-rep are handled in ROMIO*/




/** Macro to detect endianess */
#define IS_BIG_ENDIAN (*(sctk_uint16_t *)"\0\xff" < 0x100)

/** ==========================================================
 * These conversion functions are adapted from MPICH 
 *  (C) 2001 by Argonne National Laboratory.
 */


/*
  set to 1: uses manual swapping routines
            for 16/32 bit data types
  set to 0: uses system provided swapping routines
            for 16/32 bit data types
*/
#define MANUAL_BYTESWAPS 1


#if (MANUAL_BYTESWAPS == 0)
#include <netinet/in.h>
#endif

#define BITSIZE_OF(type)    (sizeof(type) * CHAR_BIT)

#if (MANUAL_BYTESWAPS == 1)
static void  BASIC_convert32(char *src, char * dest)      
{
	sctk_uint32_t tmp_src = *((sctk_uint32_t *) src);

	*((sctk_uint32_t *) dest) = (((tmp_src >> 24) & 0x000000FF) |
				     ((tmp_src >>  8) & 0x0000FF00) |
				     ((tmp_src <<  8) & 0x00FF0000) |
				     ((tmp_src << 24) & 0xFF000000));
}
#else
static void BASIC_convert32(char *src, char *dest)
{                                    
    dest = htonl(*((uint32_t *)src));
}
#endif


#if (MANUAL_BYTESWAPS == 1)
static void BASIC_convert16(char *src, char *dest)
{
 	sctk_uint16_t tmp_src = *((sctk_uint16_t *) src);
   
	*((sctk_uint16_t *) dest) = (((tmp_src >> 8) & 0x00FF) |
	    			     ((tmp_src << 8) & 0xFF00));
}
#else
static void BASIC_convert16(char *src, char *dest)
{     
    dest = htons((uint16_t)src);
}
#endif


static inline void BASIC_convert64(char *src, char *dest)
{
    uint32_t tmp_src[2];
    uint32_t tmp_dest[2];

    tmp_src[0] = (sctk_uint32_t)(*((sctk_uint64_t *)src) >> 32);
    tmp_src[1] = (sctk_uint32_t)((*((sctk_uint64_t *)src) << 32) >> 32);

    BASIC_convert32((char *)&tmp_src[0], (char *)&tmp_dest[0]);
    BASIC_convert32((char *)&tmp_src[1], (char *)&tmp_dest[1]);

    *((sctk_uint64_t *)dest) = (sctk_uint64_t)tmp_dest[0];
    *((sctk_uint64_t *)dest) <<= 32;
    *((sctk_uint64_t *)dest) |= (sctk_uint64_t)tmp_dest[1];
}

static inline void BASIC_convert96(char *src, char *dest)
{
    sctk_uint32_t tmp_src[3];
    sctk_uint32_t tmp_dest[3];
    char *ptr = dest;

    tmp_src[0] = (sctk_uint32_t)(*((sctk_uint64_t *)src) >> 32);
    tmp_src[1] = (sctk_uint32_t)((*((sctk_uint64_t *)src) << 32) >> 32);
    tmp_src[2] = (sctk_uint32_t)
        (*((uint32_t *)((char *)src + sizeof(sctk_uint64_t))));

    BASIC_convert32((char *)&tmp_src[0], (char *)&tmp_dest[0]);
    BASIC_convert32((char *)&tmp_src[1], (char *)&tmp_dest[1]);
    BASIC_convert32((char *)&tmp_src[2], (char *)&tmp_dest[2]);

    *((sctk_uint32_t *)ptr) = tmp_dest[0];
    ptr += sizeof(sctk_uint32_t);
    *((sctk_uint32_t *)ptr) = tmp_dest[1];
    ptr += sizeof(sctk_uint32_t);
    *((sctk_uint32_t *)ptr) = tmp_dest[2];
}

static inline void BASIC_convert128(char *src, char *dest)
{
    sctk_uint64_t tmp_src[2];
    sctk_uint64_t tmp_dest[2];
    char *ptr = dest;

    tmp_src[0] = *((sctk_uint64_t *)src);
    tmp_src[1] = *((sctk_uint64_t *)((char *)src + sizeof(sctk_uint64_t)));

    BASIC_convert64((char *) &tmp_src[0], (char *) &tmp_dest[0]);
    BASIC_convert64((char *) &tmp_src[1], (char *) &tmp_dest[1]);

    *((sctk_uint64_t *)ptr) = tmp_dest[0];
    ptr += sizeof(sctk_uint64_t);
    *((sctk_uint64_t *)ptr) = tmp_dest[1];
}


static inline void BASIC_convert(int type_byte_size, char *src, char *dest)               
{
	if( IS_BIG_ENDIAN == 0 )
	{
		switch(type_byte_size)                     
		{                                          
			case 1:                                
			    dest = src;                        
			    break;                             
			case 2:                                
			    BASIC_convert16(src, dest);        
			    break;                             
			case 4:                                
			    BASIC_convert32(src, dest);        
			    break;                             
			case 8:                                
			    BASIC_convert64( src, dest);
			    break;                             
		}
	}
	else
	{
		memcpy( dest, src, type_byte_size );
	}
                          
}


/*
  Notes on the IEEE floating point format
  ---------------------------------------

  external32 for floating point types is big-endian IEEE format.

  ---------------------
  32 bit floating point
  ---------------------
  * big endian byte order
  struct be_ieee754_single_precision
  {
  unsigned int sign_neg:1;
  unsigned int exponent:8;
  unsigned int mantissa:23;
  };

  * little endian byte order
  struct le_ieee754_single_precision
  {
  unsigned int mantissa:23;
  unsigned int exponent:8;
  unsigned int sign_neg:1;
  };
  ---------------------

  ---------------------
  64 bit floating point
  ---------------------
  * big endian byte order
  struct be_ieee754_double_precision
  {
  unsigned int sign_neg:1;
  unsigned int exponent:11;
  unsigned int mantissa0:20;
  unsigned int mantissa1:32;
  };

  * little endian byte order
  * big endian float word order
  struct le_ieee754_double_precision
  {
  unsigned int mantissa0:20;
  unsigned int exponent:11;
  unsigned int sign_neg:1;
  unsigned int mantissa1:32;
  };

  * little endian byte order
  * little endian float word order
  struct le_ieee754_double_precision
  {
  unsigned int mantissa1:32;
  unsigned int mantissa0:20;
  unsigned int exponent:11;
  unsigned int sign_neg:1;
  };
  ---------------------

  ---------------------
  96 bit floating point
  ---------------------
  * big endian byte order
  struct be_ieee854_double_extended
  {
  unsigned int negative:1;
  unsigned int exponent:15;
  unsigned int empty:16;
  unsigned int mantissa0:32;
  unsigned int mantissa1:32;
  };

  * little endian byte order
  * big endian float word order
  struct le_ieee854_double_extended
  {
  unsigned int exponent:15;
  unsigned int negative:1;
  unsigned int empty:16;
  unsigned int mantissa0:32;
  unsigned int mantissa1:32;
  };

  * little endian byte order
  * little endian float word order
  struct le_ieee854_double_extended
  {
  unsigned int mantissa1:32;
  unsigned int mantissa0:32;
  unsigned int exponent:15;
  unsigned int negative:1;
  unsigned int empty:16;
  };
  ---------------------

  128 bit floating point implementation notes
  ===========================================

  "A 128-bit long double number consists of an ordered pair of
  64-bit double-precision numbers. The first member of the
  ordered pair contains the high-order part of the number, and
  the second member contains the low-order part. The value of the
  long double quantity is the sum of the two 64-bit numbers."

  From http://nscp.upenn.edu/aix4.3html/aixprggd/genprogc/128bit_long_double_floating-point_datatype.htm
  [ as of 09/04/2003 ]
*/


static void FLOAT_convert(int type_byte_size,char * src, char * dest)
{
	if( IS_BIG_ENDIAN == 0 )
	{
		switch(type_byte_size)                    
		{                                         
			case 4:                               
			{                                     
				long d;                            
				BASIC_convert32( src, (char *)&d);     
				*((float*)dest) = (float)d;                   
			}                                     
			break;                                
			case 8:                               
			{                                     
				BASIC_convert64(src, dest);
			}                                     
			case 12:                              
			{                                     
				BASIC_convert96(src, dest);    
			}                                     
			case 16:                              
			{                                     
				BASIC_convert128(src, dest);   
			}                                     
			break;
		}
	}
	else
	{
		memcpy( dest, src, type_byte_size );
	}                              
}


/** ==========================================================
 * END of MPICH related code
 */


/** \brief This function returns the extern32 size of a given data-type
 *  \param common_type Type which size is requested
 *
 *  \return the extern32 size
 *
 *  \warning the data-type mush be a common one !
 */
static inline size_t MPC_Extern32_common_type_size( MPC_Datatype common_type )
{
	if( !sctk_datatype_is_common( common_type ) )
	{
		sctk_fatal( "MPC_Extern32_common_type_size only handle common types");
	}


	/* Note that MPC_COMPLEX(8,16,32) are
	 * handled through MPC_C_COMPLEX* (alias) */
	
	switch( common_type )
	{
		case MPC_PACKED:
		case MPC_BYTE:
		case MPC_CHAR:
		case MPC_UNSIGNED_CHAR:
		case MPC_SIGNED_CHAR:
		case MPC_C_BOOL:
		case MPC_INT8_T:
		case MPC_UINT8_T:
		case MPC_CHARACTER:
		case MPC_INTEGER1:
			return 1;

		case MPC_WCHAR:
		case MPC_SHORT:
		case MPC_UNSIGNED_SHORT:
		case MPC_INT16_T:
		case MPC_UINT16_T:
		case MPC_INTEGER2:
			return 2;
		case MPC_INT:
		case MPC_INTEGER:
		case MPC_LOGICAL:
		case MPC_UNSIGNED:
		case MPC_LONG:
		case MPC_UNSIGNED_LONG:
		case MPC_FLOAT:
		case MPC_INT32_T:
		case MPC_UINT32_T:
		case MPC_REAL:
		case MPC_INTEGER4:
		case MPC_REAL4:
			return 4;
		case MPC_LONG_LONG_INT:
		case MPC_UNSIGNED_LONG_LONG_INT:
		case MPC_DOUBLE:
		case MPC_DOUBLE_PRECISION:
		case MPC_INT64_T:
		case MPC_UINT64_T:
		case MPC_AINT:
		case MPC_COUNT:
		case MPC_OFFSET:
		case MPC_C_COMPLEX:
		case MPC_C_FLOAT_COMPLEX:
		case MPC_INTEGER8:
		case MPC_REAL8:
			return 8;
		case MPC_LONG_DOUBLE:
		case MPC_C_DOUBLE_COMPLEX:
		case MPC_INTEGER16:
		case MPC_REAL16:
			return 16;
		case MPC_C_LONG_DOUBLE_COMPLEX:
			return 32;
		
	}

	return 0;
}

/** \brief This function returns 1 if the type is unsigned
 *  \param common_type Type which signedness is requested
 *
 *  \return 1 if signed
 *
 *  \warning the data-type mush be a common one !
 */
static inline int MPC_Unsigned_type( MPC_Datatype common_type )
{
	if( !sctk_datatype_is_common( common_type ) )
	{
		sctk_fatal( "MPC_Extern32_common_type_size only handle common types");
	}


	/* Note that MPC_COMPLEX(8,16,32) are
	 * handled through MPC_C_COMPLEX* (alias) */
	
	switch( common_type )
	{
		case MPC_UNSIGNED_CHAR:
		case MPC_UINT8_T:
		case MPC_UNSIGNED_SHORT:
		case MPC_UINT16_T:
		case MPC_UNSIGNED:
		case MPC_UNSIGNED_LONG:
		case MPC_UINT32_T:
		case MPC_UNSIGNED_LONG_LONG_INT:
		case MPC_UINT64_T:
			return 1;
		break;
		default:
			return 0;
		break;
	}
	
	return 0;
}


void MPC_Extern32_convert( MPC_Datatype * typevector ,
						   int type_vector_size, 
						   char * native_buff, 
						   MPC_Aint max_native_size, 
						   char * extern_buff, 
						   MPC_Aint max_extern_size , 
						   int encode );
#endif
