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
#include "datatype.h"

#include <string.h>
#include <mpc_common_debug.h>

#include "mpc_reduction.h"
#include "comm_lib.h"
#include "errh.h"
#include "mpc_common_types.h"
#include "mpc_common_types.h"
#include "uthash.h"

#include <sctk_alloc.h>

/************************************************************************/
/* GLOBALS                                                              */
/************************************************************************/

/** \brief Macro to obtain the total number of datatypes */
#define MPC_TYPE_COUNT ( SCTK_COMMON_DATA_TYPE_COUNT + 2 * SCTK_USER_DATA_TYPES_MAX )


static const char *const type_combiner_names[MPC_COMBINER_COUNT__] =
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
		"MPC_COMBINER_HVECTOR_INTEGER"};

char *_mpc_dt_get_combiner_name( MPC_Type_combiner combiner )
{
	if ( combiner < MPC_COMBINER_COUNT__ )
	{
		return (char *) type_combiner_names[combiner];
	}

	return (char *) type_combiner_names[MPC_COMBINER_UNKNOWN];
}

/************************************************************************/
/* Datatype Init and Release                                            */
/************************************************************************/

static volatile int __mpc_dt_initialized = 0;

static inline void __mpc_common_types_init(void);
static inline void __mpc_composed_common_types_init();

void _mpc_dt_init()
{
	static mpc_common_spinlock_t init_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

	mpc_common_spinlock_lock(&init_lock);

	/* Common types are shared */
	if ( !__mpc_dt_initialized )
	{
		__mpc_dt_initialized = 1;
		mpc_lowcomm_datatype_init_common();	
	}

	mpc_common_spinlock_unlock(&init_lock);

	/* Initialize composed datatypes */
	__mpc_composed_common_types_init();

}

static inline void __mpc_dt_name_clear();

void _mpc_dt_release()
{
	static mpc_common_spinlock_t clear_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

	mpc_common_spinlock_lock(&clear_lock);

	if ( !__mpc_dt_initialized )
	{
		mpc_common_spinlock_unlock(&clear_lock);
		return;
	}

	__mpc_dt_initialized = 0;

	__mpc_dt_name_clear();

	mpc_common_spinlock_unlock(&clear_lock);

}

/************************************************************************/
/* Datatype  Attribute Handling                                         */
/************************************************************************/

/** \brief This structure defines a Datatype Keyval */
struct __mpc_dt_keyval
{
	MPC_Type_copy_attr_function *copy;	 /**< Keyval copy func */
	MPC_Type_delete_attr_function *delete; /**< Keyval delete func */
void *extra_state;			   /**< An extra state to be stored */
	int free_cell;			   /**< Initally 0, 1 is the keyval is then freed */
};

/** Here we store Type Keyvals as a linear array */
static struct __mpc_dt_keyval *__keyval_array = NULL;
/** This is the current array size */
static unsigned int __keyval_array_size = 0;
/** This is the current ID offset */
static unsigned int __keyval_array_offset = 0;
/** This is the Keyval array lock */
mpc_common_spinlock_t __keyval_array_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

/** \brief This function retrieves a keyval from an id
 * 	\param type_keyval ID of keyval to retrieve (offset in a static table)
 *  \return Pointer to keyval (or NULL if keyval freed or invalid)
 */
static inline struct __mpc_dt_keyval *__mpc_dt_keyval_get( unsigned int type_keyval )
{
	if ( __keyval_array_size <= type_keyval )
		return NULL;

	mpc_common_spinlock_lock( &__keyval_array_lock );
	struct __mpc_dt_keyval *cell = &__keyval_array[type_keyval];
	mpc_common_spinlock_unlock( &__keyval_array_lock );

	if ( cell->free_cell )
	{
		/* Already freed */
		return NULL;
	}

	return cell;
}

/** \brief This function deletes a keyval from an id
 * 	\param type_keyval ID of keyval to delete (offset in a static table)
 *  \return MPI_SUCCESS if ok
 */
static inline int __mpc_dt_keyval_delete( unsigned int type_keyval )
{
	struct __mpc_dt_keyval *key = __mpc_dt_keyval_get( type_keyval );

	if ( !key )
	{
		return MPC_ERR_ARG;
	}

	key->free_cell = 1;

	return MPC_LOWCOMM_SUCCESS;
}

/** \brief This function create a new keyval
 *  \param type_keyval (OUT) Id of the new keyval
 *  \return Pointer to a newly created keyval entry
 */
static inline struct __mpc_dt_keyval *__mpc_dt_keyval_new( int *type_keyval )
{
	struct __mpc_dt_keyval *ret = NULL;
	int need_realloc = 0;

	mpc_common_spinlock_lock( &__keyval_array_lock );

	if ( __keyval_array_size == 0 )
	{
		/* No keyval array yet */
		__keyval_array_size = 4;
		need_realloc = 1;
	}

	int new_id = -1;

	unsigned int i;

	/* Try to do some recycling */
	for ( i = 0; i < __keyval_array_offset; i++ )
	{
		if ( __keyval_array[i].free_cell )
		{
			new_id = i;
			break;
		}
	}

	/* We create a new entry */
	if ( new_id < 0 )
	{
		new_id = __keyval_array_offset;
		__keyval_array_offset++;
	}

	if ( __keyval_array_size <= __keyval_array_offset )
	{
		/* We are at the end of the array */
		__keyval_array_size *= 2;
		need_realloc = 1;
	}

	if ( need_realloc )
	{
		/* We are here if we had to realloc */
		__keyval_array = sctk_realloc(
			__keyval_array, sizeof( struct __mpc_dt_keyval ) * __keyval_array_size );

		if ( !__keyval_array )
		{
			perror( "realloc" );
			abort();
		}
	}

	/* Now that we booked a slot export its ID and prepare to return the entry */
	*type_keyval = new_id;
	ret = &__keyval_array[new_id];

	mpc_common_spinlock_unlock( &__keyval_array_lock );

	return ret;
}

/** \brief Trigger delete handler (if present) on a given data-type
 *  \param type_keyval Target KEYVAL
 *  \param type Source data-type
 */
static inline void __mpc_dt_keyval_hit_delete( int type_keyval, void *attribute_val,
						mpc_lowcomm_datatype_t type )
{
	struct __mpc_dt_keyval *kv = __mpc_dt_keyval_get( type_keyval );

	if ( !kv )
		return;

	if ( kv->delete == MPC_TYPE_NULL_DELETE_FN )
		return;

	( kv->delete )( type, type_keyval, attribute_val, kv->extra_state );
}

/** \brief Trigger copy handler (if present) on a given data-type
 *  \param type_keyval Target KEYVAL
 *  \param oldtype old data-type
 *  \param attribute_val_in old data-type value
 *  \param attribute_val_out new data-type value
 */
static inline void __mpc_dt_keyval_hit_copy( int type_keyval, mpc_lowcomm_datatype_t oldtype,
					void **attribute_val_in, void **attribute_val_out,
					int *flag )
{
	struct __mpc_dt_keyval *kv = __mpc_dt_keyval_get( type_keyval );

	if ( !kv )
		return;

	*flag = 0;

	if ( kv->delete == MPC_TYPE_NULL_COPY_FN )
		return;

	if ( kv->delete == MPC_TYPE_NULL_DUP_FN )
	{
		*flag = 1;
		*attribute_val_out = *attribute_val_in;
		return;
	}

	( kv->copy )( oldtype, type_keyval, kv->extra_state, attribute_val_in,
				  attribute_val_out, flag );
}

/** \brief Create a new data-type keyval
 *  \param copy Copy func
 *  \param delete Delete func
 *  \param type_keyval (OUT) ID of the newly create keyval
 *  \param extra_state Extra pointer to be stored in the keyval
 *  \return MPI_SUCCESS if ok
 */
int _mpc_dt_keyval_create( MPC_Type_copy_attr_function *copy,
						   MPC_Type_delete_attr_function *delete,
						   int *type_keyval, void *extra_state )
{
	/* Create a new slot */
	struct __mpc_dt_keyval *new = __mpc_dt_keyval_new( type_keyval );

	if ( !new )
		return MPC_ERR_INTERN;

	/* Fill content */
	new->copy = copy;
	new->delete = delete;
	new->extra_state = extra_state;

	return MPC_LOWCOMM_SUCCESS;
}

/** \brief Delete a keyval
 *  \param type_keyval Delete a keyval
 *  \return MPI_SUCCESS if ok
 */
int _mpc_dt_keyval_free( int *type_keyval )
{
	/* Is is already freed ? */
	if ( *type_keyval == MPC_KEYVAL_INVALID )
		return MPC_ERR_ARG;

	/* Delete in the internal container */
	int ret = __mpc_dt_keyval_delete( *type_keyval );

	/* Clear key handler */
	if ( ret == MPC_LOWCOMM_SUCCESS )
		*type_keyval = MPC_KEYVAL_INVALID;

	return ret;
}

/** \brief This defines a datatype ATTR storing the attribute val
 *          to be manipulalted by \ref __mpc_dt_attr_store
 */
struct __mpc_dt_attr
{
	void *attribute_val; /**< Value of the attribute */
	int type_keyval;	 /**< Referecne keyval */
};


/** \brief Create a new attr entry (alloc)
 *  \param type_keyval Associated keyval
 *  \param attribute_val Attribute to be stored
 *  \param Return a pointer to a newly allocated Attr (NULL on error)
 */
static inline struct __mpc_dt_attr *__mpc_dt_attr_new( int type_keyval, void *attribute_val )
{
	struct __mpc_dt_attr *new = sctk_malloc( sizeof( struct __mpc_dt_attr ) );

	if ( !new )
	{
		perror( "malloc" );
		return NULL;
	}

	new->type_keyval = type_keyval;
	new->attribute_val = attribute_val;

	return new;
}

/** \brief Initialize a data-type store
 *  \param store Pointer to the target store
 *  \return 0 on success
 */
static inline int __mpc_dt_attr_store_init( struct __mpc_dt_attr_store *store )
{
	mpc_common_hashtable_init( &store->attrs, 1 );
	return 0;
}

/** \brief Release a data-type store
 *  \param store Pointer to the target store
 *  \param container_type Datatype being released
 *  \return 0 on success
 */
static inline int __mpc_dt_attr_store_release( struct __mpc_dt_attr_store *store,
						mpc_lowcomm_datatype_t container_type )
{
	void *pattr = NULL;

	/* Call delete handlers for this type */
	MPC_HT_ITER( &store->attrs, pattr )

	if ( pattr )
	{
		struct __mpc_dt_attr *attr = (struct __mpc_dt_attr *) pattr;
		__mpc_dt_keyval_hit_delete( attr->type_keyval, attr->attribute_val,
									container_type );
		sctk_free( attr );
	}

	MPC_HT_ITER_END(&store->attrs)

	mpc_common_hashtable_release( &store->attrs );

	return 0;
}

/************************************************************************/
/* Common Datatype                                                      */
/************************************************************************/

/* We need this funtions as MPI_* types are macro replaced by MPC_* ones
 * and the standard wants MPI_* so we replace ... */
static inline void __mpc_common_dt_set_name( mpc_lowcomm_datatype_t datatype, char *name )
{
	char *tmp = strdup( name );
	tmp[2] = 'I';
	_mpc_dt_name_set( datatype, tmp );
	free( tmp );
}

static inline void ___mpc_init_composed_common_type( mpc_lowcomm_datatype_t target_type,
													 size_t disp,
													 mpc_lowcomm_datatype_t type_a,
													 mpc_lowcomm_datatype_t type_b,
													 size_t struct_size )
{
	/* Compute data-type sizes */
	size_t sa, sb;

	_mpc_cl_type_size( type_a, &sa );
	_mpc_cl_type_size( type_b, &sb );

	/* Allocate type context */
	long *begins = sctk_malloc( sizeof( long ) * 2 );
	long *ends = sctk_malloc( sizeof( long ) * 2 );
	mpc_lowcomm_datatype_t *types = sctk_malloc( sizeof( mpc_lowcomm_datatype_t ) * 2 );

	assume( begins != NULL );
	assume( ends != NULL );
	assume( types != NULL );

	/* Fill type context according to collected data */
	begins[0] = 0;
	begins[1] = disp;

	/* YES datatypes bounds are inclusive ! */
	ends[0] = sa - 1;
	ends[1] = disp + sb - 1;

	types[0] = type_a;
	types[1] = type_b;

	int *blocklengths = sctk_malloc( 2 * sizeof( int ) );
	size_t *displacements = sctk_malloc( 2 * sizeof( void * ) );

	assume( blocklengths != NULL );
	assume( displacements != NULL );

	blocklengths[0] = 1;
	blocklengths[1] = 1;

	displacements[0] = begins[0];
	displacements[1] = begins[1];

	/* Create the derived data-type */
	_mpc_cl_derived_datatype_on_slot( target_type, begins, ends, types, 2, 0, 0, struct_size, 1 );

	/* Fill its context */
	struct _mpc_dt_context dtctx;
	_mpc_dt_context_clear( &dtctx );
	dtctx.combiner = MPC_COMBINER_STRUCT;
	dtctx.count = 2;
	dtctx.array_of_blocklenght = blocklengths;
	dtctx.array_of_displacements_addr = displacements;
	dtctx.array_of_types = types;
	_mpc_cl_type_ctx_set( target_type, &dtctx );

	/* We do this to handle the padding as we
	 * want to guarantee the matching with
	 * an used defined struct */
	_mpc_cl_type_flag_padded( target_type );
}

/** \brief Initialize Common data-types based on derived ones
 *  This is called at mpi tasc context creation
 */
static inline void __mpc_composed_common_types_init()
{
	size_t disp;
	mpc_lowcomm_datatype_t tmp;
	/* MPC_FLOAT_INT (SCTK_DERIVED_DATATYPE_BASE */
	mpc_float_int foo_0;
	disp = ( (char *) &foo_0.b - (char *) &foo_0.a );
	___mpc_init_composed_common_type( MPC_FLOAT_INT, disp, MPC_LOWCOMM_FLOAT, MPC_LOWCOMM_INT, sizeof( mpc_float_int ) );
	__mpc_common_dt_set_name( MPC_FLOAT_INT, "MPI_FLOAT_INT" );
	tmp = MPC_FLOAT_INT;
	_mpc_cl_type_commit( &tmp );

	/* MPC_LONG_INT (SCTK_DERIVED_DATATYPE_BASE + 1 */
	mpc_long_int foo_1;
	disp = ( (char *) &foo_1.b - (char *) &foo_1.a );
	___mpc_init_composed_common_type( MPC_LONG_INT, disp, MPC_LOWCOMM_LONG, MPC_LOWCOMM_INT,
									  sizeof( mpc_long_int ) );
	__mpc_common_dt_set_name( MPC_LONG_INT, "MPI_LONG_INT" );
	tmp = MPC_LONG_INT;
	_mpc_cl_type_commit( &tmp );

	/* MPC_DOUBLE_INT  (SCTK_DERIVED_DATATYPE_BASE + 2 */
	mpc_double_int foo_2;
	disp = ( (char *) &foo_2.b - (char *) &foo_2.a );
	___mpc_init_composed_common_type( MPC_DOUBLE_INT, disp, MPC_LOWCOMM_DOUBLE,
									  MPC_LOWCOMM_INT, sizeof( mpc_double_int ) );
	__mpc_common_dt_set_name( MPC_DOUBLE_INT, "MPI_DOUBLE_INT" );
	tmp = MPC_DOUBLE_INT;
	_mpc_cl_type_commit( &tmp );

	/* MPC_SHORT_INT  (SCTK_DERIVED_DATATYPE_BASE + 3 */
	mpc_short_int foo_3;
	disp = ( (char *) &foo_3.b - (char *) &foo_3.a );
	___mpc_init_composed_common_type( MPC_SHORT_INT, disp, MPC_LOWCOMM_SHORT, MPC_LOWCOMM_INT,
									  sizeof( mpc_short_int ) );
	__mpc_common_dt_set_name( MPC_SHORT_INT, "MPI_SHORT_INT" );
	tmp = MPC_SHORT_INT;
	_mpc_cl_type_commit( &tmp );

	/* MPC_2INT  (SCTK_DERIVED_DATATYPE_BASE + 4 */
	mpc_int_int foo_4;
	disp = ( (char *) &foo_4.b - (char *) &foo_4.a );
	___mpc_init_composed_common_type( MPC_2INT, disp, MPC_LOWCOMM_INT, MPC_LOWCOMM_INT,
									  sizeof( mpc_int_int ) );
	__mpc_common_dt_set_name( MPC_2INT, "MPI_2INT" );
	tmp = MPC_2INT;
	_mpc_cl_type_commit( &tmp );

	/* MPC_2FLOAT  (SCTK_DERIVED_DATATYPE_BASE + 5 */
	mpc_float_float foo_5;
	disp = ( (char *) &foo_5.b - (char *) &foo_5.a );
	___mpc_init_composed_common_type( MPC_2FLOAT, disp, MPC_LOWCOMM_FLOAT, MPC_LOWCOMM_FLOAT,
									  sizeof( mpc_float_float ) );
	__mpc_common_dt_set_name( MPC_2FLOAT, "MPI_2FLOAT" );
	tmp = MPC_2FLOAT;
	_mpc_cl_type_commit( &tmp );

	/* MPC_COMPLEX  (SCTK_DERIVED_DATATYPE_BASE + 6 */
	___mpc_init_composed_common_type( MPC_COMPLEX, disp, MPC_LOWCOMM_FLOAT, MPC_LOWCOMM_FLOAT,
									  sizeof( mpc_float_float ) );
	__mpc_common_dt_set_name( MPC_COMPLEX, "MPI_COMPLEX" );
	tmp = MPC_COMPLEX;
	_mpc_cl_type_commit( &tmp );

	/* MPC_COMPLEX8  (SCTK_DERIVED_DATATYPE_BASE + 10 */
	___mpc_init_composed_common_type( MPC_COMPLEX8, disp, MPC_LOWCOMM_FLOAT, MPC_LOWCOMM_FLOAT,
									  sizeof( mpc_float_float ) );
	__mpc_common_dt_set_name( MPC_COMPLEX8, "MPI_COMPLEX8" );
	tmp = MPC_COMPLEX8;
	_mpc_cl_type_commit( &tmp );

	/* MPC_2DOUBLE_PRECISION  (SCTK_DERIVED_DATATYPE_BASE + 7 */
	mpc_double_double foo_6;
	disp = ( (char *) &foo_6.b - (char *) &foo_6.a );
	___mpc_init_composed_common_type( MPC_2DOUBLE_PRECISION, disp, MPC_LOWCOMM_DOUBLE,
									  MPC_LOWCOMM_DOUBLE, sizeof( mpc_double_double ) );
	__mpc_common_dt_set_name( MPC_2DOUBLE_PRECISION,
							  "MPI_2DOUBLE_PRECISION" );
	tmp = MPC_2DOUBLE_PRECISION;
	_mpc_cl_type_commit( &tmp );

	/* MPC_COMPLEX16  (SCTK_DERIVED_DATATYPE_BASE + 11 */
	___mpc_init_composed_common_type( MPC_COMPLEX16, disp, MPC_LOWCOMM_DOUBLE,
									  MPC_LOWCOMM_DOUBLE, sizeof( mpc_double_double ) );
	__mpc_common_dt_set_name( MPC_COMPLEX16, "MPI_COMPLEX16" );
	tmp = MPC_COMPLEX16;
	_mpc_cl_type_commit( &tmp );

	/* MPC_DOUBLE_COMPLEX  (SCTK_DERIVED_DATATYPE_BASE + 12 */
	___mpc_init_composed_common_type( MPC_DOUBLE_COMPLEX, disp, MPC_LOWCOMM_DOUBLE,
									  MPC_LOWCOMM_DOUBLE, sizeof( mpc_double_double ) );
	__mpc_common_dt_set_name( MPC_DOUBLE_COMPLEX,
							  "MPI_DOUBLE_COMPLEX" );
	tmp = MPC_DOUBLE_COMPLEX;
	_mpc_cl_type_commit( &tmp );

	/* MPC_LONG_DOUBLE_INT  (SCTK_DERIVED_DATATYPE_BASE + 8 */
	mpc_long_double_int foo_7;
	disp = ( (char *) &foo_7.b - (char *) &foo_7.a );
	___mpc_init_composed_common_type( MPC_LONG_DOUBLE_INT, disp,
									  MPC_LOWCOMM_LONG_DOUBLE, MPC_LOWCOMM_INT,
									  sizeof( mpc_long_double_int ) );
	__mpc_common_dt_set_name( MPC_LONG_DOUBLE_INT,
							  "MPI_LONG_DOUBLE_INT" );
	tmp = MPC_LONG_DOUBLE_INT;
	_mpc_cl_type_commit( &tmp );

	/* MPC_2INTEGER  (SCTK_DERIVED_DATATYPE_BASE + 13 */
	mpc_integer_integer foo_13;
	disp = ( (char *) &foo_13.b - (char *) &foo_13.a );
	___mpc_init_composed_common_type( MPC_2INTEGER, disp, MPC_LOWCOMM_INTEGER,
									  MPC_LOWCOMM_INTEGER,
									  sizeof( mpc_integer_integer ) );
	__mpc_common_dt_set_name( MPC_2INTEGER, "MPI_2INTEGER" );
	tmp = MPC_2INTEGER;
	_mpc_cl_type_commit( &tmp );

	/* MPC_2REAL  (SCTK_DERIVED_DATATYPE_BASE + 14 */
	mpc_real_real foo_14;
	disp = ( (char *) &foo_14.b - (char *) &foo_14.a );
	___mpc_init_composed_common_type( MPC_2REAL, disp, MPC_LOWCOMM_REAL, MPC_LOWCOMM_REAL,
									  sizeof( mpc_real_real ) );
	__mpc_common_dt_set_name( MPC_2REAL, "MPI_2REAL" );
	tmp = MPC_2REAL;
	_mpc_cl_type_commit( &tmp );

	/* MPC_COMPLEX32  (SCTK_DERIVED_DATATYPE_BASE + 12 */
	mpc_longdouble_longdouble foo_9;
	disp = ( (char *) &foo_9.b - (char *) &foo_9.a );
	___mpc_init_composed_common_type( MPC_COMPLEX32, disp, MPC_LOWCOMM_LONG_DOUBLE,
									  MPC_LOWCOMM_LONG_DOUBLE,
									  sizeof( mpc_longdouble_longdouble ) );
	__mpc_common_dt_set_name( MPC_COMPLEX32, "MPI_COMPLEX32" );
	tmp = MPC_COMPLEX32;
	_mpc_cl_type_commit( &tmp );
}

void _mpc_dt_common_display( mpc_lowcomm_datatype_t datatype )
{
	if ( !mpc_lowcomm_datatype_is_common( datatype ) )
	{
		mpc_common_debug_error( "Unknown datatype provided to %s\n", __FUNCTION__ );
		abort();
	}

	mpc_common_debug_error( "=============COMMON=================" );
	mpc_common_debug_error( "NAME %s", _mpc_dt_name_get( datatype ) );
	mpc_common_debug_error( "SIZE %ld", mpc_lowcomm_datatype_common_get_size(datatype) );
	mpc_common_debug_error( "====================================" );
}

/************************************************************************/
/* Contiguous Datatype                                                  */
/************************************************************************/

static inline void __mpc_dt_footprint_clear( struct _mpc_dt_footprint *ctx );

void _mpc_dt_contiguous_init( _mpc_dt_contiguout_t *type, size_t id_rank, size_t element_size, size_t count, mpc_lowcomm_datatype_t datatype )
{
	type->id_rank = id_rank;
	type->size = element_size * count;
	type->count = count;
	type->element_size = element_size;
	type->datatype = datatype;
	type->ref_count = 1;

	/* Clear context */
	__mpc_dt_footprint_clear( &type->context );

	_mpc_mpi_handle_new_from_id( datatype, _MPC_MPI_HANDLE_DATATYPE );

	/* Attrs */
	__mpc_dt_attr_store_init( &type->attrs );
}

static inline void __mpc_context_free( struct _mpc_dt_footprint * ctx );

void _mpc_dt_contiguous_release( _mpc_dt_contiguout_t *type )
{
	type->ref_count--;

	if ( type->ref_count == 0 )
	{
		/* Attrs */
		__mpc_dt_attr_store_release( &type->attrs,
									 (mpc_lowcomm_datatype_t) type->datatype );

		__mpc_context_free( &type->context );

		_mpc_mpi_handle_free( type->datatype, _MPC_MPI_HANDLE_DATATYPE );
		/* Counter == 0 then free */
		memset( type, 0, sizeof( _mpc_dt_contiguout_t ) );
	}
}

void _mpc_dt_contiguous_display( _mpc_dt_contiguout_t *target_type )
{
	mpc_common_debug_error( "=============CONTIGUOUS==================" );
	mpc_common_debug_error( "ID_RANK %ld", target_type->id_rank );
	mpc_common_debug_error( "REF_COUNT %ld", target_type->ref_count );
	mpc_common_debug_error( "SIZE %ld", target_type->size );
	mpc_common_debug_error( "ELEM SIZE %ld", target_type->element_size );
	mpc_common_debug_error( "COUNT %ld", target_type->count );
	mpc_common_debug_error( "DTYPE %d", target_type->datatype );

	int ni, na, nd, c;

	_mpc_dt_fill_envelope( &target_type->context, &ni, &na, &nd, &c );
	mpc_common_debug_error( "COMBINER : %s[%d]", _mpc_dt_get_combiner_name( c ), c );

	int i;

	printf( "INT : [" );
	for ( i = 0; i < ni; i++ )
	{
		printf( "[%d] %d , ", i, target_type->context.array_of_integers[i] );
	}
	printf( "]\n" );

	printf( "ADD : [" );
	for ( i = 0; i < na; i++ )
	{
		printf( "[%d] %lu , ", i, (unsigned long)target_type->context.array_of_addresses[i] );
	}
	printf( "]\n" );

	printf( "TYP : [" );
	for ( i = 0; i < nd; i++ )
	{
		printf( "[%d] %d , ", i, target_type->context.array_of_types[i] );
	}
	printf( "]\n" );
	mpc_common_debug_error( "==============================================" );
}

/************************************************************************/
/* Derived Datatype                                                     */
/************************************************************************/

void _mpc_dt_derived_init( _mpc_dt_derived_t *type,
						   mpc_lowcomm_datatype_t id,
						   unsigned long count,
						   long *begins,
						   long *ends,
						   mpc_lowcomm_datatype_t *datatypes,
						   long lb,
						   int is_lb,
						   long ub,
						   int is_ub )
{
	mpc_common_nodebug( "Derived create ID %d", id );
	/* Set the integer id */
	type->id = id;

	/* We now allocate the offset pairs */
	type->begins = (long *) sctk_malloc( count * sizeof( long ) );
	type->ends = (long *) sctk_malloc( count * sizeof( long ) );

	type->opt_begins = type->begins;
	type->opt_ends = type->ends;

	type->datatypes = (mpc_lowcomm_datatype_t *) sctk_malloc( count * sizeof( mpc_lowcomm_datatype_t ) );

	/*EXPAT*/
	if ( !type->begins || !type->ends || !type->datatypes )
	{
		mpc_common_debug_fatal( "Failled to allocate derived type content" );
	}

	/* And we fill them from the parameters */

	memcpy( type->begins, begins, count * sizeof( long ) );
	memcpy( type->ends, ends, count * sizeof( long ) );
	memcpy( type->datatypes, datatypes, count * sizeof( mpc_lowcomm_datatype_t ) );

	/* Fill the rest of the structure */
	type->size = 0;
	type->count = count;
	/* At the beginning before optimization opt_count == count */
	type->opt_count = count;
	type->ref_count = 1;

	/* Here we compute the total size of the type
		* by summing sections */
	unsigned long j;

	for ( j = 0; j < count; j++ )
	{
		mpc_common_nodebug( "( %d / %d ) => B : %d  E : %d D : %d ", j, count - 1, type->begins[j], type->ends[j], type->datatypes[j] );
		type->size += type->ends[j] - type->begins[j] + 1;
	}

	mpc_common_nodebug( "TYPE SIZE : %d", type->size );

	/* Set lower and upper bound parameters */
	type->ub = ub;
	type->lb = lb;
	type->is_ub = is_ub;
	type->is_lb = is_lb;

	/* We assume 0 this value is set in the Struct constructor afterwards */
	type->is_a_padded_struct = 0;

	/* Clear context */
	__mpc_dt_footprint_clear( &type->context );

	_mpc_mpi_handle_new_from_id( id, _MPC_MPI_HANDLE_DATATYPE );
	/* Attrs */
	__mpc_dt_attr_store_init( &type->attrs );
}

int _mpc_dt_derived_release( _mpc_dt_derived_t *type )
{
	mpc_common_nodebug( "Derived %d free REF : %d", type->id, type->ref_count );

	/* Here we decrement the refcounter */
	type->ref_count--;

	if ( type->ref_count == 0 )
	{

		_mpc_mpi_handle_free( type->id, _MPC_MPI_HANDLE_DATATYPE );

		/* Attrs */
		__mpc_dt_attr_store_release( &type->attrs, type->id );

		/* First call free on each embedded derived type
                  * but we must do this only once per type, therefore
                  * we accumulate counter for each type and only
                  * call on those which are non-zero */
		short is_datatype_present[MPC_TYPE_COUNT];
		memset( is_datatype_present, 0, sizeof( short ) * MPC_TYPE_COUNT );

		/* We now have to decrement the refcounter
           * First we try to get the layout and if we fail we
           * abort as it means that this datatype has
           * no layout and was not handled in set_context */

		/* Try to rely on the datype layout */
		unsigned int i;
		size_t count;
		struct _mpc_dt_layout *layout =
			_mpc_dt_get_layout( &type->context, &count );

		if ( layout )
		{
			int to_free[MPC_TYPE_COUNT];

			memset( to_free, 0, sizeof( int ) * MPC_TYPE_COUNT );

			for ( i = 0; i < count; i++ )
			{
				if ( _mpc_dt_is_boundary( layout[i].type ) )
					continue;

				to_free[layout[i].type] = 1;
			}

			if ( count )
			{
				if ( type->context.internal_type != layout[0].type &&
					 type->context.internal_type != MPC_DATATYPE_NULL )
				{
					to_free[type->context.internal_type] = 1;
				}
			}

			sctk_free( layout );

			/* Now free each type only once */
			for ( i = 0; i < MPC_TYPE_COUNT; i++ )
			{
				int not_released_yet = 0;
				_mpc_cl_type_is_allocated( i, &not_released_yet );

				if ( to_free[i] && not_released_yet &&
					 !mpc_lowcomm_datatype_is_common( i ) &&
					 !_mpc_dt_is_boundary( i ) )
				{
					mpc_lowcomm_datatype_t tmp = i;
					_mpc_cl_type_free( &tmp );
				}
			}
		}
		else
		{
			if ( type->context.combiner != MPC_COMBINER_DUMMY )
				mpc_common_debug_fatal( "We found a derived datatype %d with no layout",
							type->id );
		}
		/* Counter == 0 then free */
		if ( type->opt_begins != type->begins )
			sctk_free( type->opt_begins );

		if ( type->opt_ends != type->ends )
			sctk_free( type->opt_ends );

		sctk_free( type->begins );
		sctk_free( type->ends );

		sctk_free( type->datatypes );

		__mpc_context_free( &type->context );

		memset( type, 0, sizeof( _mpc_dt_derived_t ) );

		sctk_free( type );

		return 1;
	}

	return 0;
}

void _mpc_dt_derived_true_extend( _mpc_dt_derived_t *type, long *true_lb, long *true_ub )
{
	long min_index = 0, max_index = 0;
	int min_set = 0, max_set = 0;

	unsigned int i;

	for ( i = 0; i < type->count; i++ )
	{
		if ( !min_set || ( type->begins[i] < min_index ) )
		{
			min_index = type->begins[i];
			min_set = 1;
		}

		if ( !max_set || ( max_index < type->ends[i] ) )
		{
			max_index = type->ends[i];
			max_set = 1;
		}
	}

	*true_lb = min_index;
	*true_ub = max_index;
}


/** \brief This stucture is used to optimize a derived datatype
 *  It is easier to manipulate pairs of begin and ends instead of two arrays
 */
struct __mpc_derived_type_desc
{
	long begin;
	long end;
	mpc_lowcomm_datatype_t type;
	short ignore;
};

int _mpc_dt_derived_optimize( _mpc_dt_derived_t *target_type )
{
	/* Extract cout */
	size_t count = target_type->count;

	/* Do we have at least two blocks */
	if ( count <= 1 )
		return MPC_LOWCOMM_SUCCESS;

	/* Extract the layout in cells */
	struct __mpc_derived_type_desc *cells = sctk_malloc( sizeof( struct __mpc_derived_type_desc ) * count );

	assume( cells != NULL );

	size_t i, j;

	for ( i = 0; i < count; i++ )
	{
		cells[i].begin = target_type->begins[i];
		cells[i].end = target_type->ends[i];
		cells[i].type = target_type->datatypes[i];
		cells[i].ignore = 0;
	}

	size_t new_count = count;

	for ( i = 0; i < count; i++ )
	{
		if ( cells[i].ignore )
			continue;

		for ( j = ( i + 1 ); j < count; j++ )
		{
			mpc_common_nodebug( "[%d]{%d,%d} <=> [%d]{%d,%d} (%d == %d)[%d]", i, cells[i].begin, cells[i].end, j, cells[j].begin, cells[j].end, cells[i].end + 1, cells[j].begin, ( cells[i].end + 1 ) == cells[j].begin );
			if ( ( cells[i].end + 1 ) == cells[j].begin )
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

	if ( count != new_count )
	{
		mpc_common_nodebug( "Datatype Optimizer : merged %.4g percents of copies %s", ( count - new_count ) * 100.0 / count, ( new_count == 1 ) ? "[Type is now Contiguous]" : "" );

		target_type->opt_begins = sctk_malloc( sizeof( long ) * new_count );
		target_type->opt_ends = sctk_malloc( sizeof( long ) * new_count );

		assume( target_type->opt_begins != NULL );
		assume( target_type->opt_ends != NULL );
	}

	/* Copy back the content */

	unsigned int cnt = 0;
	for ( i = 0; i < count; i++ )
	{
		if ( cells[i].ignore )
			continue;

		mpc_common_nodebug( "{%d - %d}", cells[i].begin, cells[i].end );
		target_type->opt_begins[cnt] = cells[i].begin;
		target_type->opt_ends[cnt] = cells[i].end;
		cnt++;
	}

	assume( cnt == new_count );

	target_type->opt_count = new_count;

	sctk_free( cells );

	return MPC_LOWCOMM_SUCCESS;
}

void _mpc_dt_derived_display( _mpc_dt_derived_t *target_type )
{
	mpc_common_debug_error( "============DERIVED===================" );
	mpc_common_debug_error( "TYPE %d", target_type->id );
	mpc_common_debug_error( "NAME %s", _mpc_dt_name_get( target_type->id ) );
	mpc_common_debug_error( "SIZE %ld", target_type->size );
	mpc_common_debug_error( "REF_COUNT %ld", target_type->ref_count );
	mpc_common_debug_error( "COUNT %ld", target_type->count );
	mpc_common_debug_error( "OPT_COUNT %ld", target_type->opt_count );

	int ni, na, nd, c;

	_mpc_dt_fill_envelope( &target_type->context, &ni, &na, &nd, &c );
	mpc_common_debug_error( "COMBINER : %s[%d]", _mpc_dt_get_combiner_name( c ), c );

	int i;

	mpc_common_debug_error( "INT : [" );
	for ( i = 0; i < ni; i++ )
	{
		mpc_common_debug_error( "[%d] %d , ", i,
					target_type->context.array_of_integers[i] );
	}
	mpc_common_debug_error( "]\n" );

	mpc_common_debug_error( "ADD : [" );
	for ( i = 0; i < na; i++ )
	{
		mpc_common_debug_error( "[%d] %ld , ", i,
					target_type->context.array_of_addresses[i] );
	}
	mpc_common_debug_error( "]\n" );

	mpc_common_debug_error( "TYP : [" );
	for ( i = 0; i < nd; i++ )
	{
		mpc_common_debug_error(
			"[%d] %d (%s), ", i, target_type->context.array_of_types[i],
			_mpc_dt_name_get( target_type->context.array_of_types[i] ) );
	}
	mpc_common_debug_error( "]\n" );

	unsigned int j;
	for ( j = 0; j < target_type->opt_count; j++ )
	{
		mpc_common_debug_error( "[%d , %d]\n", target_type->opt_begins[j],
					target_type->opt_ends[j] );
	}

	mpc_common_debug_error( "==============================================" );
}

/************************************************************************/
/* Datatype  Array                                                      */
/************************************************************************/

struct _mpc_dt_storage * _mpc_dt_storage_init()
{

  struct _mpc_dt_storage * da = sctk_malloc(sizeof(struct _mpc_dt_storage));

	int i;

	for (i = 0; i < SCTK_USER_DATA_TYPES_MAX; i++)
	{
		da->derived_user_types[i] = NULL;
		memset( &da->contiguous_user_types[i] , 0 , sizeof( _mpc_dt_contiguout_t) );
	}

	mpc_common_spinlock_init(&da->datatype_lock, 0);

  return da;
}


int _mpc_dt_storage_type_can_be_released( struct _mpc_dt_storage * da, mpc_lowcomm_datatype_t datatype )
{
	_mpc_dt_contiguout_t * cont = NULL;
	_mpc_dt_derived_t * deriv = NULL;

	switch( _mpc_dt_get_kind( datatype ) )
	{
		case MPC_DATATYPES_COMMON:
			return 0;
		case MPC_DATATYPES_CONTIGUOUS :
			cont = _mpc_dt_storage_get_contiguous_datatype( da, datatype );
			return _MPC_DT_CONTIGUOUS_IS_USED( cont );
		case MPC_DATATYPES_DERIVED:
			deriv = _mpc_dt_storage_get_derived_datatype( da, datatype );
			return (deriv!=NULL)?1:0;
		case MPC_DATATYPES_UNKNOWN:
			return 0;
	}

	return 0;
}


void _mpc_dt_storage_release( struct _mpc_dt_storage * da )
{
	int i;

	/* Now we can free all datatypes */
	for( i = 0 ; i < MPC_TYPE_COUNT ; i++ )
	{
		int to_release = 0;
		to_release = _mpc_dt_storage_type_can_be_released(da, i);

		if( to_release && !mpc_lowcomm_datatype_is_common(i) && !_mpc_dt_is_struct(i))
		{
                  mpc_common_debug("Freeing unfreed datatype [%d] did you call "
                             "MPI_Type_free on all your MPI_Datatypes ?",
                             i);
                  mpc_lowcomm_datatype_t tmp = i;
                  _mpc_cl_type_free(&tmp);
                }
  }

  sctk_free(da);
}

_mpc_dt_contiguout_t *  _mpc_dt_storage_get_contiguous_datatype( struct _mpc_dt_storage * da ,  mpc_lowcomm_datatype_t datatype)
{
	assume( _mpc_dt_is_contiguous( datatype ) );

	/* Return the pointed _mpc_dt_contiguout_t */
	return &(da->contiguous_user_types[ _MPC_DT_MAP_TO_CONTIGUOUS( datatype ) ]);
}


_mpc_dt_derived_t * _mpc_dt_storage_get_derived_datatype( struct _mpc_dt_storage * da  ,  mpc_lowcomm_datatype_t datatype)
{
	assume( _mpc_dt_is_derived( datatype ) );

        return da->derived_user_types[_MPC_DT_MAP_TO_DERIVED(datatype)];
}

void _mpc_dt_storage_set_derived_datatype( struct _mpc_dt_storage * da ,  mpc_lowcomm_datatype_t datatype, _mpc_dt_derived_t * value )
{
	assume( _mpc_dt_is_derived( datatype ) );

        da->derived_user_types[_MPC_DT_MAP_TO_DERIVED(datatype)] = value;
}

/************************************************************************/
/* Datatype  Attribute Getters                                          */
/************************************************************************/

static inline struct __mpc_dt_attr_store *
__mpc_dt_get_attr_store( struct _mpc_dt_storage *da, mpc_lowcomm_datatype_t type )
{

	mpc_dt_kind_t kind = _mpc_dt_get_kind( type );

	struct __mpc_dt_attr_store *store = NULL;

	_mpc_dt_contiguout_t *cont = NULL;
	_mpc_dt_derived_t *deriv = NULL;

	switch ( kind )
	{
		case MPC_DATATYPES_CONTIGUOUS:
			cont = _mpc_dt_storage_get_contiguous_datatype( da, type );

			if ( cont )
				store = &cont->attrs;

			break;

		case MPC_DATATYPES_DERIVED:
			deriv = _mpc_dt_storage_get_derived_datatype( da, type );

			if ( deriv )
				store = &deriv->attrs;
			break;
		default:
			return NULL;
	}

	return store;
}

int _mpc_dt_attr_set( struct _mpc_dt_storage *da, mpc_lowcomm_datatype_t type,
					  int type_keyval, void *attribute_val )
{
	struct __mpc_dt_attr_store *store = __mpc_dt_get_attr_store( da, type );

	if ( !store )
		return MPC_ERR_ARG;

	struct __mpc_dt_attr *new = NULL;

	void *pnew = mpc_common_hashtable_get( &store->attrs, type_keyval );

	if ( pnew )
	{
		new = (struct __mpc_dt_attr *) pnew;
		new->attribute_val = attribute_val;
	}
	else
	{
		new = __mpc_dt_attr_new( type_keyval, attribute_val );
		mpc_common_hashtable_set( &store->attrs, type_keyval, new );
	}

	return MPC_LOWCOMM_SUCCESS;
}

int _mpc_dt_attr_get( struct _mpc_dt_storage *da, mpc_lowcomm_datatype_t type,
					  int type_keyval, void **attribute_val, int *flag )
{
	struct __mpc_dt_attr_store *store = __mpc_dt_get_attr_store( da, type );

	if ( !store )
		return MPC_ERR_ARG;

	struct __mpc_dt_attr *ret = NULL;

	void *pret = mpc_common_hashtable_get( &store->attrs, type_keyval );

	if ( pret )
	{
		ret = (struct __mpc_dt_attr *) pret;

		*( attribute_val ) = ret->attribute_val;

		*flag = 1;
	}
	else
	{
		*flag = 0;
	}

	return MPC_LOWCOMM_SUCCESS;
}

int _mpc_dt_attr_delete( struct _mpc_dt_storage *da, mpc_lowcomm_datatype_t type,
						 int type_keyval )
{
	struct __mpc_dt_attr_store *store = __mpc_dt_get_attr_store( da, type );

	if ( !store )
		return MPC_ERR_ARG;

	void *pret = mpc_common_hashtable_get( &store->attrs, type_keyval );

	if ( !pret )
	{
		return MPC_ERR_ARG;
	}

	mpc_common_hashtable_delete( &store->attrs, type_keyval );

	sctk_free( pret );

	return MPC_LOWCOMM_SUCCESS;
}

/************************************************************************/
/* Datatype  Naming                                                     */
/************************************************************************/


/** \brief This struct is used to store the name given to a datatype
 *
 *  We privileged an hash table instead of a static  array to
 *  save memory as type naming is really a secondary feature
 *  (Here we need it for MPICH Datatype tests)
 */
struct __mpc_dt_name_cell
{
	mpc_lowcomm_datatype_t datatype;
	char name[MPC_MAX_OBJECT_NAME]; /**< Name given to the datatype */
	UT_hash_handle hh; /**< This dummy data structure is required by UTHash is order to make this data structure hashable */
};


/** \brief Static variable used to store type names */
struct __mpc_dt_name_cell *datatype_names = NULL;
/** \brief Lock protecting \ref datatype_names */
mpc_common_spinlock_t datatype_names_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

static inline struct __mpc_dt_name_cell *__mpc_dt_get_name_cell( mpc_lowcomm_datatype_t datatype )
{
	struct __mpc_dt_name_cell *cell;

	HASH_FIND_INT( datatype_names, &datatype, cell );

	return cell;
}

int _mpc_dt_name_set( mpc_lowcomm_datatype_t datatype, const char *name )
{
	/* First locate a previous cell */
	mpc_common_spinlock_lock( &datatype_names_lock );

	int ret = mpc_lowcomm_datatype_common_set_name(datatype, name);

	if(ret = MPC_LOWCOMM_SUCCESS){
		mpc_common_spinlock_unlock( &datatype_names_lock );
		return ret;
	}

	struct __mpc_dt_name_cell *cell = __mpc_dt_get_name_cell( datatype );

	if ( cell )
	{
		/* If present free it */
		HASH_DEL( datatype_names, cell );
		sctk_free( cell );
	}

	/* Create a new cell */
	struct __mpc_dt_name_cell *new_cell = sctk_malloc( sizeof( struct __mpc_dt_name_cell ) );
	assume( new_cell != NULL );
	snprintf( new_cell->name, MPC_MAX_OBJECT_NAME, "%s", name );
	new_cell->datatype = datatype;

	/* Save it */

	HASH_ADD_INT( datatype_names, datatype, new_cell );
	mpc_common_spinlock_unlock( &datatype_names_lock );

	return 0;
}

char *_mpc_dt_name_get( mpc_lowcomm_datatype_t datatype )
{
	mpc_common_spinlock_lock( &datatype_names_lock );
	struct __mpc_dt_name_cell *cell = __mpc_dt_get_name_cell( datatype );

	if ( !cell )
	{
		mpc_common_spinlock_unlock( &datatype_names_lock );
		return NULL;
	}

	mpc_common_spinlock_unlock( &datatype_names_lock );
	return cell->name;
}

/** \brief Release Data-types names
 */
static inline void __mpc_dt_name_clear()
{
	struct __mpc_dt_name_cell *current, *tmp;

	HASH_ITER( hh, datatype_names, current, tmp )
	{
		HASH_DEL( datatype_names, current );
		sctk_free( current );
	}

}

/************************************************************************/
/* Datatype  Context                                                    */
/************************************************************************/

static int _mpc_dt_footprint_check_envelope( struct _mpc_dt_footprint *ref, struct _mpc_dt_footprint *candidate )
{
	int num_integers = 0;
	int num_addresses = 0;
	int num_datatypes = 0;
	int dummy_combiner = MPC_COMBINER_UNKNOWN;

	/* Retrieve envelope */
	_mpc_dt_fill_envelope( ref, &num_integers, &num_addresses, &num_datatypes, &dummy_combiner );

	/* Now compare each array */

	/* Type is generally the shortest */
	int i;

	/*
	for( i = 0 ; i < num_datatypes ; i++ )
	{
		if( ref->array_of_types[i] != candidate->array_of_types[i] )
		mpc_common_debug_error("DT %d == %d", ref->array_of_types[i] ,candidate->array_of_types[i] );
	}


	for( i = 0 ; i < num_integers ; i++ )
	{
		mpc_common_debug_error("INT %d == %d", ref->array_of_integers[i] ,candidate->array_of_integers[i] );
	}

	for( i = 0 ; i < num_addresses ; i++ )
	{
		mpc_common_debug_error("AD %d == %d", ref->array_of_addresses[i] ,candidate->array_of_addresses[i] );
	}
	*/

	for ( i = 0; i < num_datatypes; i++ )
	{
		if ( ref->array_of_types[i] != candidate->array_of_types[i] )
		{
			return 0;
		}
	}

	/* Now integers */

	for ( i = 0; i < num_integers; i++ )
	{
		if ( ref->array_of_integers[i] != candidate->array_of_integers[i] )
			return 0;
	}

	/* And addresses */

	for ( i = 0; i < num_addresses; i++ )
	{
		if ( ref->array_of_addresses[i] != candidate->array_of_addresses[i] )
			return 0;
	}

	//mpc_common_debug_error("TYPE MATCH");

	/* Here equality has been  verified */
	return 1;
}

static void __mpc_context_set_refcount( struct _mpc_dt_footprint *ctx, struct _mpc_dt_context *dctx, int enable_refcounting );

int _mpc_dt_footprint_match( struct _mpc_dt_context *eref, struct _mpc_dt_footprint *candidate )
{
	if ( !eref || !candidate )
		return 0;

	/* No need to fill if at least combiner are not the same */
	if ( eref->combiner != candidate->combiner )
		return 0;
	struct _mpc_dt_footprint ref;
	__mpc_dt_footprint_clear( &ref );

	__mpc_context_set_refcount( &ref, eref, 0 );

	/* Now check if combiners are equal */
	int ret = _mpc_dt_footprint_check_envelope( &ref, candidate );

	/* Release the temporary context */
	__mpc_context_free( &ref );

	return ret;
}

static inline void __mpc_dt_footprint_clear( struct _mpc_dt_footprint *ctx )
{
	memset( ctx, 0, sizeof( struct _mpc_dt_footprint ) );
	ctx->internal_type = MPC_DATATYPE_NULL;
}

void _mpc_dt_context_clear( struct _mpc_dt_context *ctx )
{
	memset( ctx, 0, sizeof( struct _mpc_dt_context ) );
}

/*
 * Some allocation helpers
 */

static inline int *__alloc_int_array( int count )
{
	if ( count == 0 )
		return NULL;

	int *ret = sctk_malloc( sizeof( int ) * count );
	assume( ret != NULL );
	return ret;
}

static inline size_t *__alloc_addr_array( int count )
{
	if ( count == 0 )
		return NULL;

	size_t *ret = sctk_malloc( sizeof( size_t ) * count );
	assume( ret != NULL );
	return ret;
}

static inline mpc_lowcomm_datatype_t *__alloc_datatype_array( int count )
{
	if ( count == 0 )
		return NULL;

	mpc_lowcomm_datatype_t *ret = sctk_malloc( sizeof( mpc_lowcomm_datatype_t ) * count );
	assume( ret != NULL );
	return ret;
}

#define CHECK_OVERFLOW( cnt, limit ) \
	do                               \
	{                                \
		assume( cnt < limit );       \
	} while ( 0 )

void _mpc_dt_context_set( struct _mpc_dt_footprint *ctx, struct _mpc_dt_context *dctx )
{
	__mpc_context_set_refcount( ctx, dctx, 1 );
}

static void __mpc_context_set_refcount( struct _mpc_dt_footprint *ctx, struct _mpc_dt_context *dctx, int enable_refcounting )
{
	/* Do we have a context and a type context */
	assume( ctx != NULL );
	assume( dctx != NULL );
	/* Is the type context initalized */

	assume( dctx->combiner != MPC_COMBINER_UNKNOWN );

	/* First we check if there is no previously allocated array
         * as it might happen as some datatype constructors
         * are built on top of other datatypes */
	__mpc_context_free( ctx );

	/* Save the combiner which is always needed */
	mpc_common_nodebug( "Setting combiner to %d\n", dctx->combiner );
	ctx->combiner = dctx->combiner;

	/* This combiner is used for serialized datatypes
         * it is never seen outside of the runtime */
	if ( dctx->combiner == MPC_COMBINER_DUMMY )
	{
		return;
	}

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
	_mpc_dt_fill_envelope( ctx, &n_int, &n_addr, &n_type,
						   &dummy_combiner );

	/* We call all allocs as if count == 0 it returns NULL */
	ctx->array_of_integers = __alloc_int_array( n_int );
	ctx->array_of_addresses = __alloc_addr_array( n_addr );
	ctx->array_of_types = __alloc_datatype_array( n_type );

	int i = 0;
	int cnt = 0;

	/* Retrieve type dependent information */
	switch ( ctx->combiner )
	{
		case MPC_COMBINER_DUMMY:
			mpc_common_debug_fatal( "ERROR : You are setting a context on a dummy data-type" );
			break;
		case MPC_COMBINER_NAMED:
			mpc_common_debug_fatal( "ERROR : You are setting a context on a common data-type" );
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
			ctx->internal_type = dctx->oldtype;
			break;
		case MPC_COMBINER_INDEXED:
			ctx->array_of_integers[0] = ctx->count;

			cnt = 0;
			for ( i = 1; i <= ctx->count; i++ )
			{
				CHECK_OVERFLOW( i, n_int );
				ctx->array_of_integers[i] = dctx->array_of_blocklenght[cnt];
				cnt++;
			}

			cnt = 0;
			for ( i = ( ctx->count + 1 ); i <= ( ctx->count * 2 ); i++ )
			{
				CHECK_OVERFLOW( i, n_int );
				ctx->array_of_integers[i] = dctx->array_of_displacements[cnt];
				cnt++;
			}

			ctx->array_of_types[0] = dctx->oldtype;
			break;
		case MPC_COMBINER_HINDEXED:
			ctx->array_of_integers[0] = ctx->count;

			cnt = 0;
			for ( i = 1; i <= ctx->count; i++ )
			{
				CHECK_OVERFLOW( i, n_int );
				ctx->array_of_integers[i] = dctx->array_of_blocklenght[cnt];
				cnt++;
			}

			cnt = 0;
			for ( i = 0; i < ctx->count; i++ )
			{
				CHECK_OVERFLOW( i, n_addr );
				ctx->array_of_addresses[i] = dctx->array_of_displacements_addr[cnt];
				cnt++;
			}

			ctx->array_of_types[0] = dctx->oldtype;
			/* Here we save the datatype as it might be
           * removed from the context when data-types are
           * created on top of each other */
			ctx->internal_type = dctx->oldtype;
			break;
		case MPC_COMBINER_INDEXED_BLOCK:
			ctx->array_of_integers[0] = ctx->count;
			ctx->array_of_integers[1] = dctx->blocklength;

			cnt = 0;
			for ( i = 2; i <= ( ctx->count + 1 ); i++ )
			{
				CHECK_OVERFLOW( i, n_int );
				ctx->array_of_integers[i] = dctx->array_of_displacements[cnt];
				cnt++;
			}

			ctx->array_of_types[0] = dctx->oldtype;
			break;
		case MPC_COMBINER_HINDEXED_BLOCK:
			ctx->array_of_integers[0] = ctx->count;
			ctx->array_of_integers[1] = dctx->blocklength;

			cnt = 0;
			for ( i = 0; i < ctx->count; i++ )
			{
				CHECK_OVERFLOW( i, n_addr );
				ctx->array_of_addresses[i] = dctx->array_of_displacements_addr[cnt];
				cnt++;
			}

			ctx->array_of_types[0] = dctx->oldtype;
			break;
		case MPC_COMBINER_STRUCT:
			ctx->array_of_integers[0] = ctx->count;

			cnt = 0;
			for ( i = 1; i <= ctx->count; i++ )
			{
				CHECK_OVERFLOW( i, n_int );
				ctx->array_of_integers[i] = dctx->array_of_blocklenght[cnt];
				cnt++;
			}

			cnt = 0;
			for ( i = 0; i < ctx->count; i++ )
			{
				CHECK_OVERFLOW( i, n_addr );
				ctx->array_of_addresses[i] = dctx->array_of_displacements_addr[cnt];
				cnt++;
			}

			cnt = 0;
			for ( i = 0; i < ctx->count; i++ )
			{
				CHECK_OVERFLOW( i, n_type );
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
			if ( n_type == 3 && dctx->array_of_types[0] == MPC_LB &&
				 dctx->array_of_types[2] == MPC_UB )
				ctx->internal_type = dctx->array_of_types[1];
			break;
		case MPC_COMBINER_SUBARRAY:
			ctx->array_of_integers[0] = ctx->ndims;

			cnt = 0;
			for ( i = 1; i <= ctx->ndims; i++ )
			{
				CHECK_OVERFLOW( i, n_int );
				ctx->array_of_integers[i] = dctx->array_of_sizes[cnt];
				cnt++;
			}

			cnt = 0;
			for ( i = ( ctx->ndims + 1 ); i <= ( 2 * ctx->ndims ); i++ )
			{
				CHECK_OVERFLOW( i, n_int );
				ctx->array_of_integers[i] = dctx->array_of_subsizes[cnt];
				cnt++;
			}

			cnt = 0;
			for ( i = ( 2 * ctx->ndims + 1 ); i <= ( 3 * ctx->ndims ); i++ )
			{
				CHECK_OVERFLOW( i, n_int );
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
			for ( i = 3; i <= ( ctx->ndims + 2 ); i++ )
			{
				CHECK_OVERFLOW( i, n_int );
				ctx->array_of_integers[i] = dctx->array_of_gsizes[cnt];
				cnt++;
			}

			cnt = 0;
			for ( i = ( ctx->ndims + 3 ); i <= ( 2 * ctx->ndims + 2 ); i++ )
			{
				CHECK_OVERFLOW( i, n_int );
				ctx->array_of_integers[i] = dctx->array_of_distribs[cnt];
				cnt++;
			}

			cnt = 0;
			for ( i = ( 2 * ctx->ndims + 3 ); i <= ( 3 * ctx->ndims + 2 ); i++ )
			{
				CHECK_OVERFLOW( i, n_int );
				ctx->array_of_integers[i] = dctx->array_of_dargs[cnt];
				cnt++;
			}

			cnt = 0;
			for ( i = ( 3 * ctx->ndims + 3 ); i <= ( 4 * ctx->ndims + 2 ); i++ )
			{
				CHECK_OVERFLOW( i, n_int );
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

	if ( !enable_refcounting )
		return;

	/* Now we increment the embedded type refcounter only once per allocated
        * datatype
        * to do so we walk the datatype array while incrementing a counter at
        * the
        * target type offset, later on we just have to refcount the which are
        * non-zero
        *  */
	int is_datatype_present[MPC_TYPE_COUNT];
	memset( is_datatype_present, 0, sizeof( int ) * MPC_TYPE_COUNT );

	/* Accumulate present datatypes */
	int j;
	for ( j = 0; j < n_type; j++ )
	{
		/* As UB and LB are negative it would break
           * everyting in the array */
		if ( _mpc_dt_is_boundary( ctx->array_of_types[j] ) )
			continue;

		is_datatype_present[ctx->array_of_types[j]] = 1;
	}

	/* Increment the refcounters of present datatypes */
	for ( j = 0; j < MPC_TYPE_COUNT; j++ )
	{
		if ( is_datatype_present[j] )
		{
			_mpc_cl_type_use( j );
		}
	}
}

/** \brief Frees the data stored in a data-type context
 *  \param ctx Context to free
 */
static inline void __mpc_context_free( struct _mpc_dt_footprint *ctx )
{
	sctk_free( ctx->array_of_integers );
	sctk_free( ctx->array_of_addresses );
	sctk_free( ctx->array_of_types );
}

int _mpc_dt_fill_envelope( struct _mpc_dt_footprint *ctx, int *num_integers, int *num_addresses, int *num_datatypes, int *combiner )
{
	if ( !ctx )
		return 1;

	*combiner = ctx->combiner;

	switch ( ctx->combiner )
	{
		case MPC_COMBINER_DUMMY:
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
			*num_integers = ctx->count + 1;
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

static inline struct _mpc_dt_layout *please_allocate_layout( int count )
{
	struct _mpc_dt_layout *ret = sctk_malloc( sizeof( struct _mpc_dt_layout ) * count );
	assume( ret != NULL );
	return ret;
}

static inline int _mpc_dt_layout_fill( struct _mpc_dt_layout *l, mpc_lowcomm_datatype_t datatype )
{
	assume( l != NULL );
	l->type = datatype;
	size_t size;
	_mpc_cl_type_size( datatype, &size );
	l->size = (size_t) size;

	return MPC_LOWCOMM_SUCCESS;
}

struct _mpc_dt_layout *_mpc_dt_get_layout( struct _mpc_dt_footprint *ctx, size_t *ly_count )
{
	struct _mpc_dt_layout *ret = NULL;
	size_t count = 0;

	unsigned int i = 0;
	int cnt = 0, j = 0;
	*ly_count = 0;
	int is_allocated = 0;
	int number_of_blocks = 0;

	switch ( ctx->combiner )
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

			_mpc_cl_type_is_allocated( ctx->array_of_types[0], &is_allocated );

			if ( !is_allocated )
			{
				ret[0].size = 0;
				ret[0].type = MPC_DATATYPE_NULL;
			}
			else
			{
				_mpc_dt_layout_fill( &ret[0], ctx->array_of_types[0] );
			}
			break;
		case MPC_COMBINER_STRUCT:
			/* We have to handle the case of structs where each element can have a size 
			   and also empty blocklength for indexed types */
			count = ctx->array_of_integers[0];

			/* Compute the number of blocks */
			number_of_blocks = 0;

			for ( i = 1; i <= count; i++ )
			{
				number_of_blocks += ctx->array_of_integers[i] + 1;
			}

			mpc_common_nodebug( "Num block : %d", number_of_blocks );

			/* Allocate blocks */
			ret = please_allocate_layout( number_of_blocks );

			for ( i = 0; i < count; i++ )
			{
				mpc_common_nodebug( "CTX : BL : %d   T : %d", ctx->array_of_integers[i + 1], ctx->array_of_types[i] );

				/* Here we don't consider empty blocks as elements */
				if ( ctx->array_of_integers[i + 1] == 0 )
				{
					/* We ignore blocks of length 0 */
					continue;
				}

				/* And we copy all the individual blocks */
				for ( j = 0; j < ctx->array_of_integers[i + 1]; j++ )
				{
					mpc_common_nodebug( "CUR : %d", cnt );

					/* Here we avoid reporting types which are not allocated
					 * in the layout as sometimes user only partially free
					 * datatypes but composed datatypes are looking from
					 * them in their layout when being freed and of course
					 * cannot find them. This is here just to solve random
					 * free order problems where the refcounting cannot be used */
					_mpc_cl_type_is_allocated( ctx->array_of_types[i], &is_allocated );

					if ( !is_allocated )
						continue;

					_mpc_dt_layout_fill( &ret[cnt], ctx->array_of_types[i] );
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
		case MPC_COMBINER_DUMMY:
			return NULL;
	}

	return ret;
}
