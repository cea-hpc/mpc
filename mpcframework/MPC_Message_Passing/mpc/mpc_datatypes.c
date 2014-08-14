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
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#include "mpc_datatypes.h"

#include "mpc_reduction.h"
#include <string.h>
#include "mpcmp.h"

/************************************************************************/
/* Common Datatype                                                      */
/************************************************************************/

/** Common datatypes sizes ar initialized in \ref sctk_common_datatype_init */
static size_t __sctk_common_type_sizes[SCTK_COMMON_DATA_TYPE_COUNT ];


#define SCTK_INIT_TYPE_SIZE(datatype,t) __sctk_common_type_sizes[datatype] = sizeof(t) ; sctk_assert(datatype >=0 ); sctk_assert( sctk_datatype_is_common( datatype ) )

void sctk_common_datatype_init()
{
  SCTK_INIT_TYPE_SIZE (MPC_CHAR, char);
  SCTK_INIT_TYPE_SIZE (MPC_LOGICAL, int);
  SCTK_INIT_TYPE_SIZE (MPC_BYTE, unsigned char);
  SCTK_INIT_TYPE_SIZE (MPC_SHORT, short);
  SCTK_INIT_TYPE_SIZE (MPC_INT, int);
  SCTK_INIT_TYPE_SIZE (MPC_LONG, long);
  SCTK_INIT_TYPE_SIZE (MPC_FLOAT, float);
  SCTK_INIT_TYPE_SIZE (MPC_DOUBLE, double);
  SCTK_INIT_TYPE_SIZE (MPC_UNSIGNED_CHAR, unsigned char);
  SCTK_INIT_TYPE_SIZE (MPC_UNSIGNED_SHORT, unsigned short);
  SCTK_INIT_TYPE_SIZE (MPC_UNSIGNED, unsigned int);
  SCTK_INIT_TYPE_SIZE (MPC_UNSIGNED_LONG, unsigned long);
  SCTK_INIT_TYPE_SIZE (MPC_LONG_DOUBLE, long double);
  SCTK_INIT_TYPE_SIZE (MPC_LONG_LONG_INT, long long);
  SCTK_INIT_TYPE_SIZE (MPC_UNSIGNED_LONG_LONG_INT, unsigned long long);
  SCTK_INIT_TYPE_SIZE (MPC_INTEGER1, sctk_int8_t);
  SCTK_INIT_TYPE_SIZE (MPC_INTEGER2, sctk_int16_t);
  SCTK_INIT_TYPE_SIZE (MPC_INTEGER4, sctk_int32_t);
  SCTK_INIT_TYPE_SIZE (MPC_INTEGER8, sctk_int64_t);
  SCTK_INIT_TYPE_SIZE (MPC_REAL4, float);
  SCTK_INIT_TYPE_SIZE (MPC_REAL8, double);
  SCTK_INIT_TYPE_SIZE (MPC_REAL16, long double);
  SCTK_INIT_TYPE_SIZE (MPC_FLOAT_INT, mpc_float_int);
  SCTK_INIT_TYPE_SIZE (MPC_LONG_INT, mpc_long_int);
  SCTK_INIT_TYPE_SIZE (MPC_DOUBLE_INT, mpc_double_int);
  SCTK_INIT_TYPE_SIZE (MPC_SHORT_INT, mpc_short_int);
  SCTK_INIT_TYPE_SIZE (MPC_2INT, mpc_int_int);
  SCTK_INIT_TYPE_SIZE (MPC_2FLOAT, mpc_float_float);
  SCTK_INIT_TYPE_SIZE (MPC_COMPLEX, mpc_float_float);
  SCTK_INIT_TYPE_SIZE (MPC_2DOUBLE_PRECISION, mpc_double_double);
  SCTK_INIT_TYPE_SIZE (MPC_DOUBLE_COMPLEX, mpc_double_double);
  __sctk_common_type_sizes[MPC_PACKED] = 0;
}


size_t sctk_common_datatype_get_size( MPC_Datatype datatype )
{
	if( !sctk_datatype_is_common( datatype ) )
	{
		sctk_error("Unknown datatype provided to %s\n", __FUNCTION__ );
		abort();
	}
	
	return __sctk_common_type_sizes[ datatype ];
}

/************************************************************************/
/* Contiguous Datatype                                                  */
/************************************************************************/

void sctk_contiguous_datatype_init( sctk_contiguous_datatype_t * type , size_t id_rank , size_t element_size, size_t count, sctk_datatype_t datatype )
{
	sctk_debug( "Cont create");
	type->id_rank = id_rank;
	type->size = element_size * count;
	type->count =  count;
	type->element_size = element_size;
	type->datatype = datatype;
	OPA_store_int(&type->ref_count, 1);
}

void sctk_contiguous_datatype_release( sctk_contiguous_datatype_t * type )
{
	sctk_debug( "Cont free REF : %d", sctk_atomics_load_int( &type->ref_count ) );
	
	if( OPA_decr_and_test_int(&type->ref_count) )
	{
		/* Counter == 0 then free */
		memset( type, 0 , sizeof( sctk_contiguous_datatype_t ) );
	}
}

/************************************************************************/
/* Derived Datatype                                                     */
/************************************************************************/
void sctk_derived_datatype_init( sctk_derived_datatype_t * type ,
								 unsigned long count,
                                 mpc_pack_absolute_indexes_t * begins,
                                 mpc_pack_absolute_indexes_t * ends,
                                 sctk_datatype_t * datatypes,
                                 mpc_pack_absolute_indexes_t lb, int is_lb,
						         mpc_pack_absolute_indexes_t ub, int is_ub )
{
			sctk_debug( "Derived create");
			/* We now allocate the offset pairs */
			type->begins = (mpc_pack_absolute_indexes_t *) sctk_malloc (count * sizeof(mpc_pack_absolute_indexes_t));
			type->ends = (mpc_pack_absolute_indexes_t *) sctk_malloc (count * sizeof(mpc_pack_absolute_indexes_t));
			
			/*EXPAT*/
			if( datatypes )
				type->datatypes = (sctk_datatype_t *) sctk_malloc (count * sizeof(sctk_datatype_t));
			else
				type->datatypes = NULL;

			/*EXPAT*/
			if( !type->begins || !type->ends /*|| !type->datatypes*/ )
			{
				sctk_fatal("Failled to allocate derived type content" );
			}

			/* And we fill them from the parameters */

			memcpy (type->begins, begins, count * sizeof (mpc_pack_absolute_indexes_t));
			memcpy (type->ends, ends, count * sizeof (mpc_pack_absolute_indexes_t));
			
			/*EXPAT*/
			if( datatypes )
				memcpy (type->datatypes, datatypes, count * sizeof (sctk_datatype_t));

			/* Fill the rest of the structure */
			type->size = 0;
			type->count = count;
			OPA_store_int(&type->ref_count, 1);
			
			/* Here we compute the total size of the type
			 * by summing sections */
			unsigned long j;
			for (j = 0; j < count; j++)
			{
				type->size += type->ends[j] - type->begins[j] + 1;
			}
			
			/* Set lower and upper bound parameters */
			type->ub = ub;
			type->lb = lb;
			type->is_ub = is_ub;
			type->is_lb = is_lb;
}


void sctk_derived_datatype_release( sctk_derived_datatype_t * type )
{
	sctk_debug( "Derived free REF : %d", sctk_atomics_load_int( &type->ref_count ) );
	
	
	/* Here we decrement the refcounter atomically */
	if ( OPA_decr_and_test_int(&type->ref_count) )
	{
		/*EXPAT*/
		if( type->datatypes )
		{
			/* First call free on each embedded derived type
			 * but we must do this only once per type, therefore
			 * we accumulate counter for each type and only
			 * call on those which are non-zero */
			 short is_datatype_present[ MPC_TYPE_COUNT ];
			 memset( is_datatype_present, 0 , sizeof( short ) * MPC_TYPE_COUNT );
			
			/* Accumulate present datatypes */
			int i;
			for( i = 0 ; i < type->count ; i++ )
			{
				if( sctk_datatype_kind( type->datatypes[i] ) == MPC_DATATYPES_UNKNOWN )
				{
					sctk_fatal ( "An erroneous datatype was provided");
				}
				
				is_datatype_present[ type->datatypes[i] ] = 1;
			}
			
			/* Increment the refcounters of present datatypes */
			for( i = 0 ; i < type->count ; i++ )
			{
				if( is_datatype_present[ i ] )
					PMPC_Type_free( &type->datatypes[i] );
			}
		}
		
		
		/* Counter == 0 then free */
		sctk_free (type->begins);
		sctk_free (type->ends);
		sctk_free (type->datatypes);
		
		memset( type, 0 , sizeof( sctk_derived_datatype_t ) );
		
		sctk_free (type);
	}
}
