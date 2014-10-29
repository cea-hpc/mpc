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

#include "uthash.h"
#include "mpc_reduction.h"
#include <string.h>
#include "mpcmp.h"
#include <sctk_ethread_internal.h>
#include "sctk_stdint.h"
#include "sctk_wchar.h"
#include "mpc_common.h"

/************************************************************************/
/* GLOBALS                                                              */
/************************************************************************/

/** Common datatypes sizes ar initialized in \ref sctk_common_datatype_init */
static size_t * __sctk_common_type_sizes;

static const char * const type_combiner_names[MPC_COMBINER_COUNT__] =
{
    "MPC_COMBINER_UNKNOWN",
    "MPC_COMBINER_NAMED",
    "MPC_COMBINER_DUP",
    "MPC_COMBINER_CONTIGUOUS",
    "MPC_COMBINER_VECTOR",
    "MPC_COMBINER_HVECTOR",
    "MPC_COMBINER_INDEXED",
    "MPC_COMBINER_HINDEXED",
    "MPC_COMBINER_INDEXED_BLOCK",
    "MPC_COMBINER_HINDEXED_BLOCK",
    "MPC_COMBINER_STRUCT",
    "MPC_COMBINER_SUBARRAY",
    "MPC_COMBINER_DARRAY",
    "MPC_COMBINER_F90_REAL",
    "MPC_COMBINER_F90_COMPLEX",
    "MPC_COMBINER_F90_INTEGER",
    "MPC_COMBINER_RESIZED",
    "MPC_COMBINER_HINDEXED_INTEGER",
    "MPC_COMBINER_STRUCT_INTEGER",
    "MPC_COMBINER_HVECTOR_INTEGER"
};


const char * const sctk_datype_combiner(MPC_Type_combiner combiner)
{
	if( (combiner < MPC_COMBINER_COUNT__) && ( 0 <= combiner ) )
	{
		return type_combiner_names[ combiner ];
	}
	
	return type_combiner_names[ MPC_COMBINER_UNKNOWN ];
}


/************************************************************************/
/* Datatype Init and Release                                            */
/************************************************************************/

void sctk_datatype_init()
{
	__sctk_common_type_sizes = sctk_malloc( sizeof( size_t ) * SCTK_COMMON_DATA_TYPE_COUNT );
	assume( __sctk_common_type_sizes != NULL );
	sctk_common_datatype_init();
}

void sctk_datatype_release()
{
	sctk_datype_name_release();
	sctk_free( __sctk_common_type_sizes );
}

/************************************************************************/
/* Common Datatype                                                      */
/************************************************************************/

/* We need this funtions are MPI_* types are macro replaced by MPC_* ones
 * and the standard wants MPI_* so we replace ... */
void sctk_common_datatype_set_name_helper( MPC_Datatype datatype, char * name )
{
	char * tmp = strdup( name );
	tmp[2] = 'I';
	sctk_datype_set_name( datatype, tmp );
	free( tmp );
}



void __init_a_composed_common_types(MPC_Datatype target_type, MPC_Aint disp, MPC_Datatype type_a, MPC_Datatype type_b, size_t struct_size )
{
	struct sctk_task_specific_s *ts = __MPC_get_task_specific ();

	/* Compute data-type sizes */
	size_t sa, sb;
	
	PMPC_Type_size(type_a, &sa);
	PMPC_Type_size(type_b, &sb);
	
	/* Allocate type context */
 	mpc_pack_absolute_indexes_t * begins = sctk_malloc( sizeof(mpc_pack_absolute_indexes_t) * 2 );
	mpc_pack_absolute_indexes_t * ends = sctk_malloc( sizeof(mpc_pack_absolute_indexes_t) * 2 );
	MPC_Datatype * types = sctk_malloc( sizeof(MPC_Datatype) * 2 );
	
	assume( begins != NULL );
	assume( ends != NULL );
	assume( types != NULL );
	
	/* Fill type context according to collected data */
	begins[0] = 0;
	begins[1] = disp;
	
	/* YES datatypes bounds are inclusive ! */
	ends[0] = sa -1;
	ends[1] = disp + sb - 1;
	
	types[0] = type_a;
	types[1] = type_b;

	int * blocklengths = sctk_malloc( 2 * sizeof( int ) );
	MPC_Aint * displacements = sctk_malloc( 2 * sizeof( void * ) );
	
	assume( blocklengths != NULL );
	assume( displacements != NULL );
	
	blocklengths[0] = 1;
	blocklengths[1] = 1;
	
	displacements[0] = begins[0];
	displacements[1] = begins[1];

	/* Create the derived data-type */
	PMPC_Derived_datatype_on_slot( target_type, begins, ends, types, 2, 0, 0, 0, 0);

	/* Fill its context */
	struct Datatype_External_context dtctx;
	sctk_datatype_external_context_clear( &dtctx );
	dtctx.combiner = MPC_COMBINER_STRUCT;
	dtctx.count = 2;
	dtctx.array_of_blocklenght = blocklengths;
	dtctx.array_of_displacements_addr = displacements;
	dtctx.array_of_types = types;
	MPC_Datatype_set_context( target_type, &dtctx);

	/* We do this to handle the padding as we
	 * want to guarantee the matching with
	 * an used defined struct */
	PMPC_Type_set_size(target_type, struct_size );
}

void init_composed_common_types()
{
	MPC_Aint disp;
	MPC_Datatype tmp;
	/* MPC_FLOAT_INT (SCTK_DERIVED_DATATYPE_BASE */
	mpc_float_int foo_0;
	disp = ((char *)&foo_0.b - (char *)&foo_0.a);
	__init_a_composed_common_types( MPC_FLOAT_INT, disp, MPC_FLOAT, MPC_INT, sizeof(mpc_float_int) );
	sctk_common_datatype_set_name_helper( MPC_FLOAT_INT, "MPI_FLOAT_INT" );
	tmp = MPC_FLOAT_INT;
	PMPC_Type_commit( &tmp );
	
	/* MPC_LONG_INT (SCTK_DERIVED_DATATYPE_BASE + 1 */
	mpc_long_int foo_1;
	disp = ((char *)&foo_1.b - (char *)&foo_1.a);
	__init_a_composed_common_types( MPC_LONG_INT, disp, MPC_LONG, MPC_INT, sizeof(mpc_long_int) );
	sctk_common_datatype_set_name_helper( MPC_LONG_INT, "MPI_LONG_INT" );
	tmp = MPC_LONG_INT;
	PMPC_Type_commit( &tmp );
	
	/* MPC_DOUBLE_INT  (SCTK_DERIVED_DATATYPE_BASE + 2 */
	mpc_double_int foo_2;
	disp = ((char *)&foo_2.b - (char *)&foo_2.a);
	__init_a_composed_common_types( MPC_DOUBLE_INT, disp, MPC_DOUBLE, MPC_INT , sizeof(mpc_double_int) );
	sctk_common_datatype_set_name_helper( MPC_DOUBLE_INT, "MPI_DOUBLE_INT" );
	tmp = MPC_DOUBLE_INT;
	PMPC_Type_commit( &tmp );
	
	/* MPC_SHORT_INT  (SCTK_DERIVED_DATATYPE_BASE + 3 */
	mpc_short_int foo_3;
	disp = ((char *)&foo_3.b - (char *)&foo_3.a);
	__init_a_composed_common_types( MPC_SHORT_INT, disp, MPC_SHORT, MPC_INT , sizeof(mpc_short_int) );
	sctk_common_datatype_set_name_helper( MPC_SHORT_INT, "MPI_SHORT_INT" );
	tmp = MPC_SHORT_INT;
	PMPC_Type_commit( &tmp );
	
	/* MPC_2INT  (SCTK_DERIVED_DATATYPE_BASE + 4 */
	mpc_int_int foo_4;
	disp = ((char *)&foo_4.b - (char *)&foo_4.a);
	__init_a_composed_common_types( MPC_2INT, disp, MPC_INT, MPC_INT , sizeof(mpc_int_int) );
	sctk_common_datatype_set_name_helper( MPC_2INT, "MPI_2INT" );
	tmp = MPC_2INT;
	PMPC_Type_commit( &tmp );
	
	/* MPC_2FLOAT  (SCTK_DERIVED_DATATYPE_BASE + 5 */
	mpc_float_float foo_5;
	disp = ((char *)&foo_5.b - (char *)&foo_5.a);
	__init_a_composed_common_types( MPC_2FLOAT, disp, MPC_FLOAT, MPC_FLOAT , sizeof(mpc_float_float) );
	sctk_common_datatype_set_name_helper( MPC_2FLOAT, "MPI_2FLOAT" );
	tmp = MPC_2FLOAT;
	PMPC_Type_commit( &tmp );
	
	/* MPC_COMPLEX  (SCTK_DERIVED_DATATYPE_BASE + 6 */
	__init_a_composed_common_types( MPC_COMPLEX, disp, MPC_FLOAT, MPC_FLOAT  , sizeof(mpc_float_float));
	sctk_common_datatype_set_name_helper( MPC_COMPLEX, "MPI_COMPLEX" );
	tmp = MPC_COMPLEX;
	PMPC_Type_commit( &tmp );
	
	/* MPC_COMPLEX8  (SCTK_DERIVED_DATATYPE_BASE + 10 */
	__init_a_composed_common_types( MPC_COMPLEX8, disp, MPC_FLOAT, MPC_FLOAT  , sizeof(mpc_float_float));
	sctk_common_datatype_set_name_helper( MPC_COMPLEX8, "MPI_COMPLEX8" );
	tmp = MPC_COMPLEX8;
	PMPC_Type_commit( &tmp );
	
	/* MPC_2DOUBLE_PRECISION  (SCTK_DERIVED_DATATYPE_BASE + 7 */
	mpc_double_double foo_6;
	disp = ((char *)&foo_6.b - (char *)&foo_6.a);
	__init_a_composed_common_types( MPC_2DOUBLE_PRECISION, disp, MPC_DOUBLE, MPC_DOUBLE , sizeof(mpc_double_double) );
	sctk_common_datatype_set_name_helper( MPC_2DOUBLE_PRECISION, "MPI_2DOUBLE_PRECISION" );
	tmp = MPC_2DOUBLE_PRECISION;
	PMPC_Type_commit( &tmp );
	
	/* MPC_COMPLEX16  (SCTK_DERIVED_DATATYPE_BASE + 11 */
	__init_a_composed_common_types( MPC_COMPLEX16, disp, MPC_DOUBLE, MPC_DOUBLE  , sizeof(mpc_double_double));
	sctk_common_datatype_set_name_helper( MPC_COMPLEX16, "MPI_COMPLEX16" );
	tmp = MPC_COMPLEX16;
	PMPC_Type_commit( &tmp );
	
	/* MPC_DOUBLE_COMPLEX  (SCTK_DERIVED_DATATYPE_BASE + 13 */
	__init_a_composed_common_types( MPC_DOUBLE_COMPLEX, disp, MPC_DOUBLE, MPC_DOUBLE  , sizeof(mpc_double_double));
	sctk_common_datatype_set_name_helper( MPC_DOUBLE_COMPLEX, "MPI_DOUBLE_COMPLEX" );
	tmp = MPC_DOUBLE_COMPLEX;
	PMPC_Type_commit( &tmp );
	
	/* MPC_LONG_DOUBLE_INT  (SCTK_DERIVED_DATATYPE_BASE + 8 */
	mpc_long_double_int foo_7;
	disp = ((char *)&foo_7.b - (char *)&foo_7.a);
	__init_a_composed_common_types( MPC_LONG_DOUBLE_INT, disp, MPC_LONG_DOUBLE, MPC_INT  , sizeof(mpc_long_double_int));
	sctk_common_datatype_set_name_helper( MPC_LONG_DOUBLE_INT, "MPI_LONG_DOUBLE_INT" );
	tmp = MPC_LONG_DOUBLE_INT;
	PMPC_Type_commit( &tmp );
	
	/* MPC_UNSIGNED_LONG_LONG_INT  (SCTK_DERIVED_DATATYPE_BASE + 9 */
	mpc_unsigned_long_long_int foo_8;
	disp = ((char *)&foo_8.b - (char *)&foo_8.a);
	__init_a_composed_common_types( MPC_UNSIGNED_LONG_LONG_INT, disp, MPC_UNSIGNED_LONG_LONG, MPC_INT , sizeof(mpc_unsigned_long_long_int) );
	sctk_common_datatype_set_name_helper( MPC_UNSIGNED_LONG_LONG_INT, "MPI_UNSIGNED_LONG_LONG_INT" );
	tmp = MPC_UNSIGNED_LONG_LONG_INT;
	PMPC_Type_commit( &tmp );
	
	/* MPC_COMPLEX32  (SCTK_DERIVED_DATATYPE_BASE + 12 */
	mpc_longdouble_longdouble foo_9;
	disp = ((char *)&foo_9.b - (char *)&foo_9.a);
	__init_a_composed_common_types( MPC_COMPLEX32, disp, MPC_LONG_DOUBLE, MPC_LONG_DOUBLE , sizeof(mpc_longdouble_longdouble) );
	sctk_common_datatype_set_name_helper( MPC_COMPLEX32, "MPI_COMPLEX32" );
	tmp = MPC_COMPLEX32;
	PMPC_Type_commit( &tmp );
}


void release_composed_common_types()
{
	MPC_Datatype tmp;
	tmp = MPC_FLOAT_INT;
	PMPC_Type_free( &tmp );
	tmp = MPC_LONG_INT;
	PMPC_Type_free( &tmp );
	tmp = MPC_DOUBLE_INT;
	PMPC_Type_free( &tmp );
	tmp = MPC_SHORT_INT;
	PMPC_Type_free( &tmp );
	tmp = MPC_2INT;
	PMPC_Type_free( &tmp );
	tmp = MPC_2FLOAT;
	PMPC_Type_free( &tmp );
	tmp = MPC_COMPLEX;
	PMPC_Type_free( &tmp );
	tmp = MPC_COMPLEX8;
	PMPC_Type_free( &tmp );
	tmp = MPC_2DOUBLE_PRECISION;
	PMPC_Type_free( &tmp );
	tmp = MPC_COMPLEX16;
	PMPC_Type_free( &tmp );
	tmp = MPC_DOUBLE_COMPLEX;
	PMPC_Type_free( &tmp );
	tmp = MPC_LONG_DOUBLE_INT;
	PMPC_Type_free( &tmp );
	tmp = MPC_UNSIGNED_LONG_LONG_INT;
	PMPC_Type_free( &tmp );
	tmp = MPC_COMPLEX32;
	PMPC_Type_free( &tmp );
}

#define tostring(a) #a
#define SCTK_INIT_TYPE_SIZE(datatype,t) __sctk_common_type_sizes[datatype] = sizeof(t) ; \
					sctk_assert(datatype >=0 ); \
					sctk_assert( sctk_datatype_is_common( datatype ) );\
					sctk_common_datatype_set_name_helper( datatype, #datatype );

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
	SCTK_INIT_TYPE_SIZE (MPC_LONG_LONG, long long);
	SCTK_INIT_TYPE_SIZE (MPC_UNSIGNED_LONG_LONG, unsigned long long);
	SCTK_INIT_TYPE_SIZE (MPC_INTEGER1, sctk_int8_t);
	SCTK_INIT_TYPE_SIZE (MPC_INTEGER2, sctk_int16_t);
	SCTK_INIT_TYPE_SIZE (MPC_INTEGER4, sctk_int32_t);
	SCTK_INIT_TYPE_SIZE (MPC_INTEGER8, sctk_int64_t);
	SCTK_INIT_TYPE_SIZE (MPC_INTEGER16, sctk_int64_t[2] );
	SCTK_INIT_TYPE_SIZE (MPC_REAL4, float);
	SCTK_INIT_TYPE_SIZE (MPC_REAL8, double);
	SCTK_INIT_TYPE_SIZE (MPC_REAL16, long double);
	SCTK_INIT_TYPE_SIZE (MPC_INT8_T, sctk_int8_t );
	SCTK_INIT_TYPE_SIZE (MPC_UINT8_T, sctk_uint8_t );
	SCTK_INIT_TYPE_SIZE (MPC_INT16_T, sctk_int16_t );
	SCTK_INIT_TYPE_SIZE (MPC_UINT16_T, sctk_uint16_t );
	SCTK_INIT_TYPE_SIZE (MPC_INT32_T, sctk_int32_t );
	SCTK_INIT_TYPE_SIZE (MPC_UINT32_T, sctk_uint32_t );
	SCTK_INIT_TYPE_SIZE (MPC_INT64_T, sctk_int64_t );
	SCTK_INIT_TYPE_SIZE (MPC_UINT64_T, sctk_uint64_t );
	SCTK_INIT_TYPE_SIZE (MPC_WCHAR, sctk_wchar_t );
	SCTK_INIT_TYPE_SIZE (MPC_AINT, MPC_Aint );
	SCTK_INIT_TYPE_SIZE (MPC_OFFSET, MPC_Aint );
	SCTK_INIT_TYPE_SIZE (MPC_COUNT, MPC_Count );
	SCTK_INIT_TYPE_SIZE (MPC_LONG_LONG_INT, long long int );

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

void sctk_common_datatype_display( MPC_Datatype datatype )
{
	if( !sctk_datatype_is_common( datatype ) )
	{
		sctk_error("Unknown datatype provided to %s\n", __FUNCTION__ );
		abort();
	}
	
	sctk_error("=============COMMON=================");
	sctk_error("NAME %s", sctk_datype_get_name( datatype ) );
	sctk_error("SIZE %ld",__sctk_common_type_sizes[ datatype ]);
	sctk_error("====================================");
}


/************************************************************************/
/* Contiguous Datatype                                                  */
/************************************************************************/

void sctk_contiguous_datatype_init( sctk_contiguous_datatype_t * type , size_t id_rank , size_t element_size, size_t count, sctk_datatype_t datatype )
{
	type->id_rank = id_rank;
	type->size = element_size * count;
	type->count =  count;
	type->element_size = element_size;
	type->datatype = datatype;
	type->ref_count = 1;
	
	/* Clear context */
	sctk_datatype_context_clear( &type->context );
}

void sctk_contiguous_datatype_release( sctk_contiguous_datatype_t * type )
{
	type->ref_count--;
	
	if( type->ref_count == 0 )
	{
		sctk_datatype_context_free( &type->context );
		/* Counter == 0 then free */
		memset( type, 0 , sizeof( sctk_contiguous_datatype_t ) );
	}
}

void sctk_contiguous_datatype_display( sctk_contiguous_datatype_t * target_type )
{
	sctk_error("=============CONTIGUOUS==================");
	sctk_error("ID_RANK %ld", target_type->id_rank );
	sctk_error("REF_COUNT %ld", target_type->ref_count );
	sctk_error("SIZE %ld", target_type->size );
	sctk_error("ELEM SIZE %ld", target_type->element_size );
	sctk_error("COUNT %ld", target_type->count );
	sctk_error("DTYPE %d", target_type->datatype );
	
	
	int ni, na, nd, c;

	sctk_datatype_fill_envelope( &target_type->context , &ni, &na , &nd , &c );
	sctk_error("COMBINER : %s[%d]", sctk_datype_combiner(c), c );
	
	int i;
	
	printf("INT : [");
	for( i = 0 ; i < ni ; i++ )
	{
		printf("[%d] %d , ", i, target_type->context.array_of_integers[i] );
	}
	printf("]\n");
	
	printf("ADD : [");
	for( i = 0 ; i < na ; i++ )
	{
		printf("[%d] %ld , ", i, target_type->context.array_of_addresses[i] );
	}
	printf("]\n");
	
	printf("TYP : [");
	for( i = 0 ; i < nd ; i++ )
	{
		printf("[%d] %d , ", i, target_type->context.array_of_types[i] );
	}
	printf("]\n");
	sctk_error("==============================================");
}

/************************************************************************/
/* Derived Datatype                                                     */
/************************************************************************/

void sctk_derived_datatype_init( sctk_derived_datatype_t * type ,
				 MPC_Datatype id,
				 unsigned long count,
                                 mpc_pack_absolute_indexes_t * begins,
                                 mpc_pack_absolute_indexes_t * ends,
                                 sctk_datatype_t * datatypes,
                                 mpc_pack_absolute_indexes_t lb, 
				 int is_lb,
				 mpc_pack_absolute_indexes_t ub,
				 int is_ub )
{
	sctk_nodebug( "Derived create ID %d", id);
	/* Set the integer id */
	type->id = id;
	
	/* We now allocate the offset pairs */
	type->begins = (mpc_pack_absolute_indexes_t *) sctk_malloc (count * sizeof(mpc_pack_absolute_indexes_t));
	type->ends = (mpc_pack_absolute_indexes_t *) sctk_malloc (count * sizeof(mpc_pack_absolute_indexes_t));
	
	type->opt_begins = type->begins;
	type->opt_ends = type->ends;

	type->datatypes = (sctk_datatype_t *) sctk_malloc (count * sizeof(sctk_datatype_t));

	/*EXPAT*/
	if( !type->begins || !type->ends || !type->datatypes )
	{
		sctk_fatal("Failled to allocate derived type content" );
	}

	/* And we fill them from the parameters */

	memcpy (type->begins, begins, count * sizeof (mpc_pack_absolute_indexes_t));
	memcpy (type->ends, ends, count * sizeof (mpc_pack_absolute_indexes_t));
	memcpy (type->datatypes, datatypes, count * sizeof (sctk_datatype_t));

	/* Fill the rest of the structure */
	type->size = 0;
	type->count = count;
	/* At the beginning before optimization opt_count == count */
	type->opt_count = count;
	type->ref_count = 1;
	
	/* Here we compute the total size of the type
		* by summing sections */
	unsigned long j;
	
	for (j = 0; j < count; j++)
	{
		sctk_nodebug("( %d / %d ) => B : %d  E : %d ",j, count - 1 , type->begins[j], type->ends[j]);
		type->size += type->ends[j] - type->begins[j] + 1;
	}
	
	sctk_nodebug("TYPE SIZE : %d", type->size );
	
	/* Set lower and upper bound parameters */
	type->ub = ub;
	type->lb = lb;
	type->is_ub = is_ub;
	type->is_lb = is_lb;
	
	/* Clear context */
	sctk_datatype_context_clear( &type->context );

}


int sctk_derived_datatype_release( sctk_derived_datatype_t * type )
{
	sctk_nodebug( "Derived %d free REF : %d", type->id, type->ref_count );
	
	
	/* Here we decrement the refcounter */
	type->ref_count--;
	
	if ( type->ref_count == 0 )
	{

		/* First call free on each embedded derived type
			* but we must do this only once per type, therefore
			* we accumulate counter for each type and only
			* call on those which are non-zero */
		short is_datatype_present[ MPC_TYPE_COUNT ];
		memset( is_datatype_present, 0 , sizeof( short ) * MPC_TYPE_COUNT );
		
		
		/* We now have to decrement the refcounter 
		 * First we try to get the layout and if we fail we
		 * abort as it means that this datatype has
		 * no layout and was not handled in set_context */

		/* Try to rely on the datype layout */
		int i;
		size_t count;
		struct Datatype_layout *layout = sctk_datatype_layout( &type->context, &count );

		if( layout )
		{
			int to_free[MPC_TYPE_COUNT];
			
			memset( to_free, 0, sizeof( int ) * MPC_TYPE_COUNT );
			
			
			for(i = 0; i < count; i++)
			{
				to_free[layout[i].type] = 1;
			}

			if( count )
			{
				if( type->context.internal_type != layout[0].type )
				{
					to_free[type->context.internal_type] = 1;
				}
			}

			sctk_free(layout);
			
			/* Now free each type only once */
			for(i = 0; i < MPC_TYPE_COUNT; i++)
			{
				int not_released_yet = 0;
				PMPC_Type_is_allocated ( i, & not_released_yet );
		
				if( to_free[i] 
				&&  not_released_yet
				&&  !sctk_datatype_is_common(i)
				&&  !sctk_datatype_is_boundary(i) )
				{
					MPC_Datatype tmp = i;
					PMPC_Type_free( &tmp );
				}
			}
		}
		else
		{
			sctk_fatal("We found a derived datatype %d with no layout", type->id); 
		}
		
		
		/* Counter == 0 then free */
		if( type->opt_begins != type->begins )
			sctk_free (type->opt_begins);
		
		if( type->opt_ends != type->ends )
			sctk_free (type->opt_ends);
		
		sctk_free (type->begins);
		sctk_free (type->ends);
		
		
		sctk_free (type->datatypes);
		
		sctk_datatype_context_free( &type->context );
		
		memset( type, 0 , sizeof( sctk_derived_datatype_t ) );
		
		sctk_free (type);
	
		return 1;
	}
	
	return 0;
}


void sctk_derived_datatype_true_extent( sctk_derived_datatype_t * type , mpc_pack_absolute_indexes_t * true_lb, mpc_pack_absolute_indexes_t * true_ub)
{
	mpc_pack_absolute_indexes_t min_index, max_index;
	int min_set = 0, max_set = 0;
	
	int i;
	
	for( i = 0 ; i < type->count ; i++ )
	{
		if( !min_set || ( type->begins[i] < min_index ) )
			min_index = type->begins[i];
		
		if( !max_set || ( max_index < type->ends[i] ) )
			max_index = type->ends[i];
		
	}
	
	*true_lb = min_index;
	*true_ub = max_index;
}


int sctk_derived_datatype_optimize( sctk_derived_datatype_t * target_type )
{
	/* Extract cout */
	MPC_Aint count = target_type->count;
	
	
	/* Do we have at least two blocks */
	if( count <= 1 )
		return MPC_SUCCESS;
	
	/* Extract the layout in cells */
	struct Derived_datatype_cell * cells = sctk_malloc( sizeof( struct Derived_datatype_cell ) * count);
	
	assume( cells != NULL );
	
	MPC_Aint i, j;
	
	for( i = 0; i < count ; i++ )
	{
		cells[i].begin = target_type->begins[i];
		cells[i].end = target_type->ends[i];
		cells[i].type = target_type->datatypes[i];
		cells[i].ignore = 0;
	}
	
	MPC_Aint new_count = count;
	
	for( i = 0; i < count ; i++ )
	{
		if( cells[i].ignore )
			continue;

		for( j = (i + 1) ; j < count ; j++ )
		{
			sctk_nodebug("[%d]{%d,%d} <=> [%d]{%d,%d} (%d == %d)[%d]", i, cells[i].begin , cells[i].end, j, cells[j].begin, cells[j].end,  cells[i].end +1 , cells[j].begin, (cells[i].end +1) == cells[j].begin );
			if((cells[i].end + 1 ) == cells[j].begin)
			{
				/* If cells are contiguous we merge with
				 * the previous cell */
				cells[j].ignore = 1;
				cells[i].end = cells[j].end;
				new_count--;
			}
			else
			{
				break;
			}
			
		}
		
	}
	
	assume( new_count != 0 );
	
	if( count != new_count )
	{
		sctk_info("Datatype Optimizer : merged %.4g percents of copies %s", (count  - new_count) * 100.0 / count , (new_count == 1 )?"[Type is now Contiguous]":"" );

		target_type->opt_begins = sctk_malloc( sizeof( mpc_pack_absolute_indexes_t ) * new_count );
		target_type->opt_ends = sctk_malloc( sizeof( mpc_pack_absolute_indexes_t ) * new_count );
		
		assume( target_type->opt_begins != NULL );
		assume( target_type->opt_ends != NULL );
	}
	
	/* Copy back the content */
	
	int cnt = 0;
	for( i = 0; i < count ; i++ )
	{
		if( cells[i].ignore )
			continue;
		
		sctk_nodebug("{%d - %d}",  cells[i].begin, cells[i].end);
		target_type->opt_begins[cnt] = cells[i].begin;
		target_type->opt_ends[cnt] = cells[i].end;
		cnt++;
	}
	
	assume( cnt == new_count );
	
	target_type->opt_count = new_count;
	
	sctk_free(cells);
	
	return MPC_SUCCESS;
}


void sctk_derived_datatype_display( sctk_derived_datatype_t * target_type )
{
	sctk_error("============DERIVED===================");
	sctk_error("TYPE %d", target_type->id );
	sctk_error("SIZE %ld", target_type->size );
	sctk_error("REF_COUNT %ld", target_type->ref_count );
	sctk_error("COUNT %ld", target_type->count );
	sctk_error("OPT_COUNT %ld", target_type->opt_count );
	
	
	int ni, na, nd, c;

	sctk_datatype_fill_envelope( &target_type->context , &ni, &na , &nd , &c );
	sctk_error("COMBINER : %s[%d]", sctk_datype_combiner(c), c );
	
	int i;
	
	printf("INT : [");
	for( i = 0 ; i < ni ; i++ )
	{
		printf("[%d] %d , ", i, target_type->context.array_of_integers[i] );
	}
	printf("]\n");
	
	printf("ADD : [");
	for( i = 0 ; i < na ; i++ )
	{
		printf("[%d] %ld , ", i, target_type->context.array_of_addresses[i] );
	}
	printf("]\n");
	
	printf("TYP : [");
	for( i = 0 ; i < nd ; i++ )
	{
		printf("[%d] %d , ", i, target_type->context.array_of_types[i] );
	}
	printf("]\n");
	sctk_error("==============================================");
}


/************************************************************************/
/* Datatype  Array                                                      */
/************************************************************************/

void Datatype_Array_init( struct Datatype_Array * da )
{
	int i;
	

	for (i = 0; i < SCTK_USER_DATA_TYPES_MAX; i++)
	{
		da->derived_user_types[i] = NULL;
		memset( &da->contiguous_user_types[i] , 0 , sizeof( sctk_contiguous_datatype_t) );
	}
	
	da->datatype_lock = 0; 
}


int Datatype_is_allocated( struct Datatype_Array * da, MPC_Datatype datatype )
{
	sctk_contiguous_datatype_t * cont = NULL;
	sctk_derived_datatype_t * deriv = NULL;

	switch( sctk_datatype_kind( datatype ) )
	{
		case MPC_DATATYPES_COMMON:
			return 0;
		case MPC_DATATYPES_CONTIGUOUS :
			cont = Datatype_Array_get_contiguous_datatype( da, datatype );
			return SCTK_CONTIGUOUS_DATATYPE_IS_IN_USE( cont );
		case MPC_DATATYPES_DERIVED:
			deriv = Datatype_Array_get_derived_datatype( da, datatype );
			return (deriv!=NULL)?1:0;
		case MPC_DATATYPES_UNKNOWN:
			return 0;
	}

	return 0;
}


void Datatype_Array_release( struct Datatype_Array * da )
{
	int i, j;
	int did_free = 0;

	/* Handle derived datatypes */
	
	sctk_derived_datatype_t * deriv = NULL;
	
	/* Now we can free all datatypes */
	for( i = 0 ; i < MPC_TYPE_COUNT ; i++ )
	{
		int to_release = 0;
		PMPC_Type_is_allocated ( i, & to_release );
		
		if( to_release && !sctk_datatype_is_common(i) )
		{
			sctk_warning("Freeing unfreed datatype [%d] did you call MPI_Type_free on all your MPI_Datatypes ?", i );
			MPC_Datatype tmp = i;
			PMPC_Type_free( &tmp );
			did_free = 1;
		}
	}
}

sctk_contiguous_datatype_t *  Datatype_Array_get_contiguous_datatype( struct Datatype_Array * da ,  MPC_Datatype datatype)
{
	assume( sctk_datatype_is_contiguous( datatype ) );

	/* Return the pointed sctk_contiguous_datatype_t */ 
	return &(da->contiguous_user_types[ MPC_TYPE_MAP_TO_CONTIGUOUS( datatype ) ]);	
}


sctk_derived_datatype_t * Datatype_Array_get_derived_datatype( struct Datatype_Array * da  ,  MPC_Datatype datatype)
{
	assume( sctk_datatype_is_derived( datatype ) );
	
	return da->derived_user_types[ MPC_TYPE_MAP_TO_DERIVED(datatype) ];
}

void Datatype_Array_set_derived_datatype( struct Datatype_Array * da ,  MPC_Datatype datatype, sctk_derived_datatype_t * value )
{
	assume( sctk_datatype_is_derived( datatype ) );
	
	da->derived_user_types[ MPC_TYPE_MAP_TO_DERIVED(datatype) ] = value;
}

/************************************************************************/
/* Datatype  Naming                                                     */
/************************************************************************/


/** \brief Static variable used to store type names */
struct Datatype_name_cell * datatype_names = NULL;
/** \brief Lock protecting \ref datatype_names */
sctk_spinlock_t datatype_names_lock = SCTK_SPINLOCK_INITIALIZER;

static inline struct Datatype_name_cell * sctk_datype_get_name_cell( MPC_Datatype datatype )
{
	struct Datatype_name_cell *cell;
	
	HASH_FIND_INT(datatype_names, &datatype, cell);
	
	return cell;
}



int sctk_datype_set_name( MPC_Datatype datatype, char * name )
{
	/* First locate a previous cell */
	sctk_spinlock_lock(&datatype_names_lock);
	struct Datatype_name_cell * cell = sctk_datype_get_name_cell( datatype );
	
	if( cell )
	{
		/* If present free it */
		HASH_DEL(datatype_names, cell);
		sctk_free( cell );
	}

	/* Create a new cell */
	struct Datatype_name_cell * new_cell = sctk_malloc( sizeof( struct Datatype_name_cell ) );
	assume( new_cell != NULL );
	snprintf(new_cell->name, MPC_MAX_OBJECT_NAME, "%s" , name);
	new_cell->datatype = datatype;
	
	/* Save it */
	
	HASH_ADD_INT( datatype_names, datatype, new_cell );
	sctk_spinlock_unlock(&datatype_names_lock);
	
	return 0;
}


char * sctk_datype_get_name( MPC_Datatype datatype )
{
	sctk_spinlock_lock(&datatype_names_lock);
	struct Datatype_name_cell *cell = sctk_datype_get_name_cell( datatype );

	if( !cell )
	{
		sctk_spinlock_unlock(&datatype_names_lock);
		return NULL;
	}
	
	sctk_spinlock_unlock(&datatype_names_lock);	
	return cell->name;
}

void sctk_datype_name_release()
{
	struct Datatype_name_cell *current, *tmp;	

	HASH_ITER(hh, datatype_names, current, tmp) {
		HASH_DEL(datatype_names, current); 
		sctk_free(current);
	}
}

/************************************************************************/
/* Datatype  Context                                                    */
/************************************************************************/

static int Datatype_context_check_envelope( struct Datatype_context * ref, struct Datatype_context * candidate )
{
	int num_integers = 0;
	int num_addresses = 0;
	int num_datatypes = 0;
	int dummy_combiner = MPC_COMBINER_UNKNOWN;
	
	/* Retrieve envelope */
	sctk_datatype_fill_envelope( ref , &num_integers, &num_addresses , &num_datatypes , &dummy_combiner );
	
	/* Now compare each array */
	
	/* Type is generally the shortest */
	int i;
	
	/*
	for( i = 0 ; i < num_datatypes ; i++ )
	{
		if( ref->array_of_types[i] != candidate->array_of_types[i] )
		sctk_error("DT %d == %d", ref->array_of_types[i] ,candidate->array_of_types[i] );
	}
	

	for( i = 0 ; i < num_integers ; i++ )
	{
		sctk_error("INT %d == %d", ref->array_of_integers[i] ,candidate->array_of_integers[i] );
	}

	for( i = 0 ; i < num_addresses ; i++ )
	{
		sctk_error("AD %d == %d", ref->array_of_addresses[i] ,candidate->array_of_addresses[i] );
	}
	*/

	
	for( i = 0 ; i < num_datatypes ; i++ )
	{
		if( ref->array_of_types[i] != candidate->array_of_types[i] )
		{
			return 0;
		}
	}
	
	/* Now integers */
	
	for( i = 0 ; i < num_integers ; i++ )
	{
		if( ref->array_of_integers[i] != candidate->array_of_integers[i] )
			return 0;
	}
	
	/* And addresses */
	
	for( i = 0 ; i < num_addresses ; i++ )
	{
		if( ref->array_of_addresses[i] != candidate->array_of_addresses[i] )
			return 0;
	}
	
	//sctk_error("TYPE MATCH");
	
	/* Here equality has been  verified */
	return 1;
}


int Datatype_context_match( struct Datatype_External_context * eref, struct Datatype_context * candidate )
{
	if( !eref || !candidate )
		return 0;
	
	/* No need to fill if at least combiner are not the same */
	if( eref->combiner != candidate->combiner )
		return 0;
	struct Datatype_context ref;
	sctk_datatype_context_clear(&ref);
	sctk_datatype_context_set( &ref , eref );
	
	/* Now check if combiners are equal */
	int ret =  Datatype_context_check_envelope( &ref, candidate );

	/* Release the temporary context */
	sctk_datatype_context_free( &ref );
	
	return ret;
}


void sctk_datatype_context_clear( struct Datatype_context * ctx )
{
	memset( ctx, 0, sizeof( struct Datatype_context ) );
	ctx->internal_type = MPC_DATATYPE_NULL;
}

void sctk_datatype_external_context_clear( struct Datatype_External_context * ctx )
{
	memset( ctx, 0, sizeof( struct Datatype_External_context ) );	
}

/*
 * Some allocation helpers
 */

static inline int * please_allocate_an_array_of_integers( int count )
{
	if( count == 0 )
		return NULL;

	int * ret = sctk_malloc( sizeof( int ) * count );
	assume( ret != NULL );
	return ret;
}

static inline MPC_Aint * please_allocate_an_array_of_addresses( int count )
{
	if( count == 0 )
		return NULL;

	MPC_Aint * ret = sctk_malloc( sizeof( MPC_Aint ) * count );
	assume( ret != NULL );
	return ret;
}

static inline MPC_Datatype * please_allocate_an_array_of_datatypes( int count )
{
	if( count == 0 )
		return NULL;

	MPC_Datatype * ret = sctk_malloc( sizeof( MPC_Datatype ) * count );
	assume( ret != NULL );
	return ret;
}

#define CHECK_OVERFLOW( cnt , limit ) do{ assume( cnt < limit ); } while(0)

void sctk_datatype_context_set( struct Datatype_context * ctx , struct Datatype_External_context * dctx  )
{
	/* Do we have a context and a type context */
	assume( ctx != NULL );
	assume( dctx != NULL );
	/* Is the type context initalized */
	assume( dctx->combiner != MPC_COMBINER_UNKNOWN );
	
	/* First we check if there is no previously allocated array
	 * as it might happen as some datatype constructors
	 * are built on top of other datatypes */
	sctk_datatype_context_free( ctx );
	
	/* Save the combiner which is always needed */
	sctk_nodebug("Setting combiner to %d\n", dctx->combiner );
	ctx->combiner = dctx->combiner;
	

	/* Save size computing values before calling get envelope */
	ctx->count = dctx->count;
	ctx->ndims = dctx->ndims;
	
	/* Allocate the arrays according to the envelope */
	int n_int = 0;
	int n_addr = 0;
	int n_type = 0;
	int dummy_combiner;
	
	/* The context is not yet fully initialized but we can
	 * already switch the data-type as we have just set it */
	sctk_datatype_fill_envelope( ctx, &n_int, &n_addr, &n_type, &dummy_combiner);

	
	/* We call all allocs as if count == 0 it returns NULL */
	ctx->array_of_integers = please_allocate_an_array_of_integers( n_int );
	ctx->array_of_addresses = please_allocate_an_array_of_addresses( n_addr );
	ctx->array_of_types = please_allocate_an_array_of_datatypes( n_type );
	
	int i = 0;
	int cnt = 0;

	/* Retrieve type dependent information */
	switch( ctx->combiner )
	{
		case MPC_COMBINER_NAMED:
			sctk_fatal("ERROR : You are setting a context on a common data-type");
		break;
		case MPC_COMBINER_DUP:
			ctx->array_of_types[0] = dctx->oldtype;
		break;
		case MPC_COMBINER_CONTIGUOUS:
			ctx->array_of_integers[0] = ctx->count;
			ctx->array_of_types[0] = dctx->oldtype;
			break;
		case MPC_COMBINER_VECTOR:
			ctx->array_of_integers[0] = ctx->count;
			ctx->array_of_integers[1] = dctx->blocklength;
			ctx->array_of_integers[2] = dctx->stride;
			ctx->array_of_types[0] = dctx->oldtype;
		break;
		case MPC_COMBINER_HVECTOR:
			ctx->array_of_integers[0] = ctx->count;
			ctx->array_of_integers[1] = dctx->blocklength;
			ctx->array_of_addresses[0] = dctx->stride_addr;
			ctx->array_of_types[0] = dctx->oldtype;
			/* Here we save the datatype as it might be
			 * removed from the context when data-types are
			 * created on top of each other */
			ctx->internal_type =  dctx->oldtype;
		break;
		case MPC_COMBINER_INDEXED:
			ctx->array_of_integers[0] = ctx->count;
			
			cnt = 0;
			for( i = 1 ; i <= ctx->count ; i++ )
			{
				CHECK_OVERFLOW( i , n_int );
				ctx->array_of_integers[i] = dctx->array_of_blocklenght[cnt];
				cnt++;
			}
			
			cnt = 0;
			for( i = (ctx->count + 1) ; i <= (ctx->count * 2) ; i++ )
			{
				CHECK_OVERFLOW( i , n_int );
				ctx->array_of_integers[i] = dctx->array_of_displacements[cnt];
				cnt++;
			}
			
			ctx->array_of_types[0] = dctx->oldtype;
		break;
		case MPC_COMBINER_HINDEXED:
			ctx->array_of_integers[0] = ctx->count;
			
			cnt = 0;
			for( i = 1 ; i <= ctx->count ; i++ )
			{
				CHECK_OVERFLOW( i , n_int );
				ctx->array_of_integers[i] = dctx->array_of_blocklenght[cnt];
				cnt++;
			}
			
			cnt = 0;
			for( i = 0 ; i < ctx->count ; i++ )
			{
				CHECK_OVERFLOW( i , n_addr );
				ctx->array_of_addresses[i] = dctx->array_of_displacements_addr[cnt];
				cnt++;
			}
			
			ctx->array_of_types[0] = dctx->oldtype;
			/* Here we save the datatype as it might be
			 * removed from the context when data-types are
			 * created on top of each other */
			ctx->internal_type =  dctx->oldtype;
		break;
		case MPC_COMBINER_INDEXED_BLOCK:
			ctx->array_of_integers[0] = ctx->count;
			ctx->array_of_integers[1] = dctx->blocklength;
			
			cnt = 0;
			for( i = 2 ; i <= ( ctx->count + 1 ) ; i++ )
			{
				CHECK_OVERFLOW( i , n_int );
				ctx->array_of_integers[i] = dctx->array_of_displacements[cnt];
				cnt++;
			}
			
			ctx->array_of_types[0] = dctx->oldtype;
		break;
		case MPC_COMBINER_HINDEXED_BLOCK:
			ctx->array_of_integers[0] = ctx->count;
			ctx->array_of_integers[1] = dctx->blocklength;
			
			cnt = 0;
			for( i = 0 ; i < ctx->count ; i++ )
			{
				CHECK_OVERFLOW( i , n_addr );
				ctx->array_of_addresses[i] = dctx->array_of_displacements_addr[cnt];
				cnt++;
			}
			
			ctx->array_of_types[0] = dctx->oldtype;
		break;
		case MPC_COMBINER_STRUCT:
			ctx->array_of_integers[0] = ctx->count;

			cnt = 0;
			for( i = 1 ; i <= ctx->count ; i++ )
			{
				CHECK_OVERFLOW( i , n_int );
				ctx->array_of_integers[i] = dctx->array_of_blocklenght[cnt];
				cnt++;
			}

			cnt = 0;
			for( i = 0 ; i < ctx->count ; i++ )
			{
				CHECK_OVERFLOW( i , n_addr );
				ctx->array_of_addresses[i] = dctx->array_of_displacements_addr[cnt];
				cnt++;
			}

			cnt = 0;
			for( i = 0 ; i < ctx->count ; i++ )
			{
				CHECK_OVERFLOW( i , n_type );
				ctx->array_of_types[i] = dctx->array_of_types[cnt];
				cnt++;
			}
			
			/* Here we save the datatype as it might be
			 * removed from the context when data-types are
			 * created on top of each other.
			 * 
			 * What we do here is VERY ugly ! the only
			 * type which are built on top of struct are
			 * darray an subarray and they are built as follows:
			 * 
			 * LB CONTENT UB
			 * 
			 * Here we just store a reference to the content in
			 * order to free it later */
			if( n_type == 3 && dctx->array_of_types[0] == MPC_LB && dctx->array_of_types[2] == MPC_UB)
				ctx->internal_type =  dctx->array_of_types[1];
		break;
		case MPC_COMBINER_SUBARRAY:
			ctx->array_of_integers[0] = ctx->ndims;
			
			cnt = 0;
			for( i = 1 ; i <= ctx->ndims ; i++ )
			{
				CHECK_OVERFLOW( i , n_int );
				ctx->array_of_integers[i] = dctx->array_of_sizes[cnt];
				cnt++;
			}
			
			cnt = 0;
			for( i = (ctx->ndims + 1) ; i <= (2 * ctx->ndims) ; i++ )
			{
				CHECK_OVERFLOW( i , n_int );
				ctx->array_of_integers[i] = dctx->array_of_subsizes[cnt];
				cnt++;
			}
			
			cnt = 0;
			for( i = (2 * ctx->ndims + 1) ; i <= (3 * ctx->ndims) ; i++ )
			{
				CHECK_OVERFLOW( i , n_int );
				ctx->array_of_integers[i] = dctx->array_of_starts[cnt];
				cnt++;
			}
			
			ctx->array_of_integers[3 * ctx->ndims + 1] = dctx->order;
			ctx->array_of_types[0] = dctx->oldtype;
		break;
		case MPC_COMBINER_DARRAY:
			ctx->array_of_integers[0] = dctx->size;
			ctx->array_of_integers[1] = dctx->rank;
			ctx->array_of_integers[2] = dctx->ndims;
		
			cnt = 0;
			for( i = 3 ; i <= (ctx->ndims + 2) ; i++ )
			{
				CHECK_OVERFLOW( i , n_int );
				ctx->array_of_integers[i] = dctx->array_of_gsizes[cnt];
				cnt++;
			}
		
			cnt = 0;
			for( i = (ctx->ndims + 3) ; i <= (2 * ctx->ndims + 2) ; i++ )
			{
				CHECK_OVERFLOW( i , n_int );
				ctx->array_of_integers[i] = dctx->array_of_distribs[cnt];
				cnt++;
			}
		
			cnt = 0;
			for( i = (2 * ctx->ndims + 3) ; i <= (3 * ctx->ndims + 2) ; i++ )
			{
				CHECK_OVERFLOW( i , n_int );
				ctx->array_of_integers[i] = dctx->array_of_dargs[cnt];
				cnt++;
			}
		
			cnt = 0;
			for( i = (3 * ctx->ndims + 3) ; i <= (4 * ctx->ndims + 2) ; i++ )
			{
				CHECK_OVERFLOW( i , n_int );
				ctx->array_of_integers[i] = dctx->array_of_psizes[cnt];
				cnt++;
			}
			
			ctx->array_of_integers[4 * ctx->ndims + 3] = dctx->order;
			ctx->array_of_types[0] = dctx->oldtype;
		break;
		case MPC_COMBINER_F90_REAL:
		case MPC_COMBINER_F90_COMPLEX:
			ctx->array_of_integers[0] = dctx->p;
			ctx->array_of_integers[1] = dctx->r;
		break;
		case MPC_COMBINER_F90_INTEGER:
			ctx->array_of_integers[1] = dctx->r;
		break;
		case MPC_COMBINER_RESIZED:
			ctx->array_of_addresses[0] = dctx->lb;
			ctx->array_of_addresses[1] = dctx->extent;
			ctx->array_of_types[0] = dctx->oldtype;
		break;
		case MPC_COMBINER_HINDEXED_INTEGER:
		case MPC_COMBINER_STRUCT_INTEGER:
		case MPC_COMBINER_HVECTOR_INTEGER:
		case MPC_COMBINER_COUNT__:
		case MPC_COMBINER_UNKNOWN:
			not_reachable();
	}

	
	/* Now we increment the embedded type refcounter only once per allocated datatype
	* to do so we walk the datatype array while incrementing a counter at the
	* target type offset, later on we just have to refcount the which are non-zero
	*  */
	int is_datatype_present[ MPC_TYPE_COUNT ];
	memset( is_datatype_present, 0 , sizeof( int ) * MPC_TYPE_COUNT );

	/* Accumulate present datatypes */
	int j;
	for( j = 0 ; j < n_type ; j++ )
	{
		/* As UB and LB are negative it would break
		 * everyting in the array */
		if( sctk_datatype_is_boundary( ctx->array_of_types[j]) )
			continue;

		is_datatype_present[ ctx->array_of_types[j] ] = 1;
	}

	/* Increment the refcounters of present datatypes */
	for( j = 0 ; j < MPC_TYPE_COUNT ; j++ )
	{
		if( is_datatype_present[ j ] )
		{
			PMPC_Type_use( j );
		}
	}

}


void sctk_datatype_context_free( struct Datatype_context * ctx )
{
	sctk_free( ctx->array_of_integers );
	sctk_free( ctx->array_of_addresses );
	sctk_free( ctx->array_of_types );
}




int sctk_datatype_fill_envelope( struct Datatype_context * ctx , int * num_integers, int * num_addresses , int * num_datatypes , int * combiner )
{
	if( !ctx )
		return 1;
	
	*combiner = ctx->combiner;
	
	
	switch( ctx->combiner )
	{
		case MPC_COMBINER_NAMED:
			/* Calling MPI get contents
			 * is invalid for this combiner */
			*num_integers = 0;
			*num_addresses = 0;
			*num_datatypes = 0;
		break;
		case MPC_COMBINER_DUP:
			*num_integers = 0;
			*num_addresses = 0;
			*num_datatypes = 1;
		break;
		case MPC_COMBINER_CONTIGUOUS:
			*num_integers = 1;
			*num_addresses = 0;
			*num_datatypes = 1;
		break;
		case MPC_COMBINER_VECTOR:
			*num_integers = 3;
			*num_addresses = 0;
			*num_datatypes = 1;
		break;
		case MPC_COMBINER_HVECTOR:
			*num_integers = 2;
			*num_addresses = 1;
			*num_datatypes = 1;
		break;
		case MPC_COMBINER_INDEXED:
			*num_integers = 2 * ctx->count + 1;
			*num_addresses = 0;
			*num_datatypes = 1;
		break;
		case MPC_COMBINER_HINDEXED:
			*num_integers = ctx->count + 1;
			*num_addresses = ctx->count;
			*num_datatypes = 1;
		break;
		case MPC_COMBINER_INDEXED_BLOCK:
			*num_integers = ctx->count + 2;
			*num_addresses = 0;
			*num_datatypes = 1;
		break;
		case MPC_COMBINER_HINDEXED_BLOCK:
			*num_integers = 2;
			*num_addresses = ctx->count;
			*num_datatypes = 1;
		break;
		case MPC_COMBINER_STRUCT:
			*num_integers = ctx->count +1;
			*num_addresses = ctx->count;
			*num_datatypes = ctx->count;
		break;
		case MPC_COMBINER_SUBARRAY:
			*num_integers = 3 * ctx->ndims + 2;
			*num_addresses = 0;
			*num_datatypes = 1;
		break;
		case MPC_COMBINER_DARRAY:
			*num_integers = 4 * ctx->ndims + 4;
			*num_addresses = 0;
			*num_datatypes = 1;
		break;
		case MPC_COMBINER_F90_REAL:
		case MPC_COMBINER_F90_COMPLEX:
			*num_integers = 2;
			*num_addresses = 0;
			*num_datatypes = 0;
		break;
		case MPC_COMBINER_F90_INTEGER:
			*num_integers = 1;
			*num_addresses = 0;
			*num_datatypes = 0;
		break;
		case MPC_COMBINER_RESIZED:
			*num_integers = 0;
			*num_addresses = 2;
			*num_datatypes = 1;
		break;
		
		case MPC_COMBINER_HINDEXED_INTEGER:
		case MPC_COMBINER_STRUCT_INTEGER:
		case MPC_COMBINER_HVECTOR_INTEGER:
		case MPC_COMBINER_COUNT__:
		case MPC_COMBINER_UNKNOWN:
			return 1;
	}
	
	
	return 0;
}

/************************************************************************/
/* Datatype  Layout                                                     */
/************************************************************************/

static inline struct Datatype_layout * please_allocate_layout( int count )
{
	struct Datatype_layout * ret = sctk_malloc( sizeof( struct Datatype_layout ) * count );
	assume( ret != NULL );
	return ret;
}

static inline int Datatype_layout_fill( struct Datatype_layout * l, MPC_Datatype datatype )
{
	assume( l != NULL );
	l->type = datatype;
	size_t size;
	PMPC_Type_size (datatype, &size);
	l->size = (MPC_Aint) size;
	
	return MPC_SUCCESS;
}

struct Datatype_layout * sctk_datatype_layout( struct Datatype_context * ctx, size_t * ly_count )
{
	struct Datatype_layout *ret = NULL;
	
	size_t count = 0;
	size_t ndims = 0;
	int i, cnt, j;
	*ly_count = 0;
	int is_allocated = 0;
	
	switch( ctx->combiner )
	{
		case MPC_COMBINER_CONTIGUOUS:
		case MPC_COMBINER_RESIZED:
		case MPC_COMBINER_DUP:
		case MPC_COMBINER_VECTOR:
		case MPC_COMBINER_HVECTOR:
		case MPC_COMBINER_DARRAY:
		case MPC_COMBINER_SUBARRAY:
		case MPC_COMBINER_INDEXED_BLOCK:
		case MPC_COMBINER_HINDEXED_BLOCK:
		case MPC_COMBINER_INDEXED:
		case MPC_COMBINER_HINDEXED:
			/* Here no surprises the size is always the same */
			ret = please_allocate_layout( 1 );
			*ly_count = 1;
			
			PMPC_Type_is_allocated (ctx->array_of_types[0], &is_allocated );
					
			if( !is_allocated )
			{
				ret[0].size = 0;
				ret[0].type = MPC_DATATYPE_NULL;
			}
			else
			{
				Datatype_layout_fill( &ret[0] , ctx->array_of_types[0] );
			}
		break;
		case MPC_COMBINER_STRUCT:
			/* We have to handle the case of structs where each element can have a size 
			   and also empty blocklength for indexed types */
			count = ctx->array_of_integers[0];
			
			/* Compute the number of blocks */
			int number_of_blocks = 0;
			
			for( i = 1 ; i <= count ; i++ )
			{
				number_of_blocks += ctx->array_of_integers[i] + 1;
			}

			sctk_nodebug("Num block : %d", number_of_blocks);
			
			/* Allocate blocks */
			ret = please_allocate_layout( number_of_blocks );
			

			
			cnt = 0;
			for( i = 0 ; i < count ; i++ )
			{
				sctk_nodebug("CTX : BL : %d   T : %d", ctx->array_of_integers[i + 1], ctx->array_of_types[i]);
				
				/* Here we don't consider empty blocks as elements */
				if( ctx->array_of_integers[i + 1] == 0 )
				{
					/* We ignore blocks of length 0 */
					continue;
				}
	
				/* And we copy all the individual blocks */
				for( j = 0 ; j < ctx->array_of_integers[i + 1] ; j++ )
				{
					sctk_nodebug("CUR : %d", cnt);
					
					/* Here we avoid reporting types which are not allocated
					 * in the layout as sometimes user only partially free
					 * datatypes but composed datatypes are looking from
					 * them in their layout when being freed and of course
					 * cannot find them. This is here just to solve random
					 * free order problems where the refcounting cannot be used */
					PMPC_Type_is_allocated (ctx->array_of_types[i], &is_allocated );
					
					if( !is_allocated )
						continue;
					
					Datatype_layout_fill( &ret[cnt] , ctx->array_of_types[i] );
					cnt++;
				}
			}
			
			*ly_count = cnt;
		break;

		case MPC_COMBINER_NAMED:
		case MPC_COMBINER_F90_REAL:
		case MPC_COMBINER_F90_COMPLEX:
		case MPC_COMBINER_F90_INTEGER:
		case MPC_COMBINER_HINDEXED_INTEGER:
		case MPC_COMBINER_STRUCT_INTEGER:
		case MPC_COMBINER_HVECTOR_INTEGER:
		case MPC_COMBINER_COUNT__:
		case MPC_COMBINER_UNKNOWN:
			return NULL;
	}
	
	return ret;
}
