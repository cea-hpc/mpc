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
#ifndef MPC_DATATYPES_H
#define MPC_DATATYPES_H

#include <uthash.h>
#include <stdlib.h>


#include <mpc_mpi_comm_lib.h>
#include <mpc_common_asm.h>
#include <mpc_common_debug.h>

#include "mpc_lowcomm.h" /* mpc_lowcomm_datatype_t */
#include "mpc_common_datastructure.h"
#include "mpc_common_spinlock.h"
#include "mpc_thread.h"

#include "mpc_lowcomm_datatypes.h"

/************************************************************************/
/* Datatype Init and Release                                            */
/************************************************************************/

/** \brief Entry Point in the datatype code
 * Called from sctk_thread.c
 */
void _mpc_dt_init();

/** \brief Exit Point in the datatype code
 * Called from sctk_thread.c
 */
void _mpc_dt_release();

/************************************************************************/
/* Datatype  Attribute Handling                                         */
/************************************************************************/

/** \brief Create a new data-type keyval
 *  \param copy Copy func
 *  \param delete Delete func
 *  \param type_keyval (OUT) ID of the newly create keyval
 *  \param extra_state Extra pointer to be stored in the keyval
 *  \return MPI_SUCCESS if ok
 */
int _mpc_dt_keyval_create( MPC_Type_copy_attr_function *copy,
						   MPC_Type_delete_attr_function *delete,
						   int *type_keyval, void *extra_state );

/** \brief Delete a keyval
 *  \param type_keyval Delete a keyval
 *  \return MPI_SUCCESS if ok
 */
int _mpc_dt_keyval_free( int *type_keyval );

/** \brief This is where datatype attrs are stored (in each dt)
 */
struct __mpc_dt_attr_store
{
	struct mpc_common_hashtable attrs; /**< Just an ATTR ht */
};

/************************************************************************/
/* Datatype  Context                                                    */
/************************************************************************/

/** \brief Retrieve a combiner name
 *  \param combiner Target combiner
 *  \return a static char describing the combiner
 */
char *_mpc_dt_get_combiner_name( MPC_Type_combiner combiner );

/** \brief This datastructure is used to store the definition
 * of a datatype in order to reproduce it using MPI_Get_envelope
 * and MPI_Get_contents */
struct _mpc_dt_footprint
{
	/* Internal ref-counting handling */

	mpc_lowcomm_datatype_t internal_type; /**< This is the internal type when types are built on top of each other happens for hvector and hindexed */

	/* MPI_get_envelope */
	MPC_Type_combiner combiner; /**< Combiner used to build the datatype */
	int count;					/**< Number of item (as given in the data-type call) */
	int ndims;					/**< Number of dimensions (as given in the data-type call) */

	/* These three arrays are those returned by MPI_Type_get_contents
	 * they are defined in the standard as a pack of the
	 * parameters provided uppon type creation  */
	int *array_of_integers;			   /** An array of integers */
	size_t *array_of_addresses;		   /* An array of addresses */
	mpc_lowcomm_datatype_t *array_of_types; /** An array of types */
};

/** \brief This structure is used to pass the parameters to \ref _mpc_dt_context_set
 *
 *  We use a structure as we don't want to polute the calling context
 *  with all the variants comming from the shape of the callers.
 *  Here the calling type function just set what's needed and
 *  leave the rest NULL. To be sure that errors will be easy to check
 *  the called must call \ref _mpc_dt_context_clear to
 *  allow further initialization checks.
 *
 *  We took the names directly from the standard in order
 *  to make things easier. See standard p. 120 -> 123
 *  here in 3.0
 *
 */
struct _mpc_dt_context
{
	MPC_Type_combiner combiner;
	int count;
	int size;
	int rank;
	int ndims;
	int order;
	int p;
	int r;
	mpc_lowcomm_datatype_t oldtype;
	int blocklength;
	int stride;
	size_t stride_addr;
	size_t lb;
	size_t extent;
	const int *array_of_gsizes;
	const int *array_of_distribs;
	const int *array_of_dargs;
	const int *array_of_psizes;
	const int *array_of_blocklenght;
	const int *array_of_sizes;
	const int *array_of_subsizes;
	const int *array_of_starts;
	const int *array_of_displacements;
	const size_t *array_of_displacements_addr;
	const mpc_lowcomm_datatype_t *array_of_types;
};

/** \brief Clears the external context
 *  \param ctx Context to clear
 */
void _mpc_dt_context_clear( struct _mpc_dt_context *ctx );

/** \brief Function used to factorize data-types by checking their equality
 *
 *  \param ref The datatype external ctx to be checked
 *  \param candidate The datatype we check against
 *  \return 1 if datatypes are equal 0 otherwise
 */
int _mpc_dt_footprint_match( struct _mpc_dt_context *eref, struct _mpc_dt_footprint *candidate );

/** \brief Setup the datatype context
 *
 *  \param ctx Context to fill
 *  \param dctx Datatype context
 *
 *  Note that it is this function which actually generate
 *  the data returned by \ref MPI_Type_Get_contents
 *  and store them in the \ref _mpc_dt_footprint structure.
 *  We have no shuch structure for common data-types as it
 *  is not allowed to call \ref MPI_Type_Get_contents on them
 */
void _mpc_dt_context_set( struct _mpc_dt_footprint *ctx, struct _mpc_dt_context *dctx );

/** \brief This call is used to fill the envelope of an MPI type
 *
 *  \param ctx Source context
 *  \param num_integers Number of input integers [OUT]
 *  \param num_adresses Number of input addresses [OUT]
 *  \param num_datatypes Number of input datatypes [OUT]
 *  \param combiner Combiner used to build the datatype [OUT]
 *
 *  This function is directly directed by the standard
 *  which defines the output values page 120-123
 *  of the standard as ni, na, nd.
 *
 */
int _mpc_dt_fill_envelope( struct _mpc_dt_footprint *ctx, int *num_integers, int *num_addresses, int *num_datatypes, int *combiner );

/************************************************************************/
/* Datatype  Layout                                                     */
/************************************************************************/

/** \brief This structure is used to retrieve datatype layout from its context
 *  It is particularly useful in get_elements
 */
struct _mpc_dt_layout
{
	mpc_lowcomm_datatype_t type;
	size_t size;
};

/** \brief Retrieve a type layout from its context
 *  \param ctx Target datatype context
 *  \return NULL on error a list of datatype blocks otherwise
 */
struct _mpc_dt_layout *_mpc_dt_get_layout( struct _mpc_dt_footprint *ctx, size_t *ly_count );

/************************************************************************/
/* Common Datatype                                                      */
/************************************************************************/

/** \brief Returns 1 if datatype is a common datatype
 */
static inline int _mpc_dt_is_common( mpc_lowcomm_datatype_t datatype )
{
	if ( ( 0 <= datatype ) && ( datatype < SCTK_COMMON_DATA_TYPE_COUNT ) )
	{
		return 1;
	}

	return 0;
}

/* This is the internal array holding common type sizes*/
extern size_t *__sctk_common_type_sizes;
/** \brief Get common datatypes sizes
 *
 *  \warning This function should only be called after MPC init
 *  it aborts if not provided with a common datatype
 *
 *  \param datatype target common datatype
 *  \return datatype size
 */
static inline size_t _mpc_dt_common_get_size( mpc_lowcomm_datatype_t datatype )
{
	assert( _mpc_dt_is_common( datatype ) );
	return mpc_lowcomm_datatype_common_get_size(datatype);
}

/** \brief Display debug informations about a common datatype
 *  \param target_type Type to be displayed
 */
void _mpc_dt_common_display( mpc_lowcomm_datatype_t datatype );

/************************************************************************/
/* Contiguous Datatype                                                  */
/************************************************************************/

/** \brief This structure describes continuous datatypes
 *  Such datatypes are continuous packs of other datatypes
 *  This structure is initialized in \ref _mpc_cl_type_hcontiguous
 */
typedef struct
{
	size_t id_rank;					  /**< Identifier of the contiguous type which is also its offset in the contiguous type table*/
	size_t size;					  /**< Total size of the contiguous type */
	size_t element_size;			  /**< Size of an element of type */
	size_t count;					  /**< Number of elements of type "datatype" in the type */
	mpc_lowcomm_datatype_t datatype;		  /**< Type packed within the datatype */
	unsigned int ref_count;			  /**< Flag telling if the datatype slot is free for use */
	struct _mpc_dt_footprint context; /**< Saves the creation context for MPI_get_envelope & MPI_Get_contents */
	struct __mpc_dt_attr_store attrs; /**< ATTR array for this type */
} _mpc_dt_contiguout_t;

/** \brief _mpc_dt_contiguout_t initializer
 *  this function is called from \ref _mpc_cl_type_hcontiguous
 *
 *  \param type Type to be initialized
 *  \param id_rank unique identifier of the  type which is also its offset in the contiguous type array
 *  \param element_size Size of a datatype element
 *  \param count Number of element
 *  \param source original datatype id
 *
 */
void _mpc_dt_contiguous_init( _mpc_dt_contiguout_t *type, size_t id_rank, size_t element_size, size_t count, mpc_lowcomm_datatype_t datatype );

/** \brief Releases a contiguous datatype
 *  \param type This is the datatype to be freed
 *
 *	\warning This call does not free the container it empties the content whent the refcounter reaches 0
 */
void _mpc_dt_contiguous_release( _mpc_dt_contiguout_t *type );

/** \brief Theses macros allow us to manipulate the contiguous datatype refcounter more simply */
#define _MPC_DT_CONTIGUOUS_IS_FREE( datatype_ptr ) ( datatype_ptr->ref_count == 0 )
#define _MPC_DT_CONTIGUOUS_IS_USED( datatype_ptr ) ( datatype_ptr->ref_count )

/** \brief Display debug informations about a contiguous datatype
 *  \param target_type Type to be displayed
 */
void _mpc_dt_contiguous_display( _mpc_dt_contiguout_t *target_type );

/************************************************************************/
/* Derived Datatype                                                     */
/************************************************************************/

/** \brief More general datatype used to describe more complex datatypes
 *
 *  Here a datatype is described as a list of segments which can themselves
 *  gather several types.
 *
 */
typedef struct
{
	mpc_lowcomm_datatype_t id; /**< Integer ID (useful  for debug) */
	/* Context */
	size_t size;			/**< Total size of the datatype */
	unsigned long count;	/**< Number of elements in the datatype */
	unsigned int ref_count; /**< Ref counter to manage freeing */

	/* Content */
	long *begins; /**< Begin offsets */
	long *ends;   /**< End offsets */

	/* Optimized Content */
	unsigned long opt_count;				 /**< Number of blocks with optimization */
	long *opt_begins; /**< Begin offsets with optimization */
	long *opt_ends;   /**< End offsets with optimization */
	mpc_lowcomm_datatype_t *datatypes;			 /**< Datatypes for each block */

	/* Bounds */
	long lb; /**< Lower bound offset  */
	int is_lb;						/**< Does type has a lower bound */
	long ub; /**< Upper bound offset */
	int is_ub;						/**< Does type has an upper bound */
	int is_a_padded_struct;			/**< Was the type padded with UB during construction ? */

	/* Context */
	struct _mpc_dt_footprint context; /**< Saves the creation context for MPI_get_envelope & MPI_Get_contents */

	/* Attrs */
	struct __mpc_dt_attr_store attrs; /**< ATTR array for this type */
} _mpc_dt_derived_t;

/** \brief Initializes a derived datatype
 *
 * This function allocates a derived datatype
 *
 * \param type Datatype to build
 * \param id Integer id of the new datatype (for debug)
 * \param count number of offsets to store
 * \param begins list of starting offsets in the datatype
 * \param ends list of end offsets in the datatype
 * \param datatypes List of datatypes for individual blocks
 * \param lb offset of type lower bound
 * \param is_lb tells if the type has a lowerbound
 * \param ub offset for type upper bound
 * \param is_b tells if the type has an upper bound
 *
 */
void _mpc_dt_derived_init( _mpc_dt_derived_t *type, mpc_lowcomm_datatype_t id,
						   unsigned long count,
						   long *begins,
						   long *ends,
						   mpc_lowcomm_datatype_t *datatypes,
						   long lb, int is_lb,
						   long ub, int is_ub );

/** \brief Releases a derived datatype
 *
 *  \param type Type to be released
 *
 *  \warning This call frees the container when the refcounter reaches 0
 *
 */
int _mpc_dt_derived_release( _mpc_dt_derived_t *type );

/** \brief Get the minimum offset of a derieved datatype (ignoring LB)
 *  \param type Type which true lb is requested
 *  \param true_lb the true lb of the datatype
 *  \param size size true LB to true UB
 */
void _mpc_dt_derived_true_extend( _mpc_dt_derived_t *type, long *true_lb, long *true_ub );

/** \brief Try to optimize a derived datatype (called by \ref PMPC_Commit)
 *  \param target_type Type to be optimized
 */
int _mpc_dt_derived_optimize( _mpc_dt_derived_t *target_type );

/** \brief Display debug informations about a derived datatype
 *  \param target_type Type to be displayed
 */
void _mpc_dt_derived_display( _mpc_dt_derived_t *target_type );

/************************************************************************/
/* Datatype ID range calculations                                       */
/************************************************************************/

/**
 * Datatypes are identified in function of their position in a contiguous
 * array such as :
 *
 * 	Common datatypes ==> [ 0 , SCTK_COMMON_DATA_TYPE_COUNT[
 *  Contiguous datatypes => [ SCTK_COMMON_DATA_TYPE_COUNT, SCTK_USER_DATA_TYPES_MAX[
 *  Derived datatypes => [ SCTK_USER_DATA_TYPES_MAX, SCTK_USER_DATA_TYPES_MAX * 2 [
 *
 *  This layout is useful as we can directly pass around a single int to identify
 *  any datatype, greatly simplifying fortran handling and also avoiding a
 *  lookup each time a datatype is refferenced.
 *
 *  Therefore we need some functions to check wether a datatype is of a
 *  given type (meaning having an id in a given range).
 *
 *  We also rely on macros to translate from the global ID to the id
 *  local to each range in order  to manipulate the actual type structure.
 */

/** \brief List of datatype kinds
 *  Note that as previously described a datatype can directly
 *  be derived from its integer ID. This labelling is provided
 *  for convenience when "switching" between cases using \ref _mpc_dt_get_kind.
 */
typedef enum {
	MPC_DATATYPES_COMMON,	 /**< These are the common datatypes defined in \ref __mpc_common_types_init */
	MPC_DATATYPES_CONTIGUOUS, /**< These are contiguous datatypes of type \ref _mpc_dt_contiguout_t */
	MPC_DATATYPES_DERIVED,	/**< These are derived datatypes of type \ref _mpc_dt_derived_t */
	MPC_DATATYPES_UNKNOWN	 /**< This key is used to detect faulty datatypes IDs */
} mpc_dt_kind_t;

/** \brief Returns 1 if datatype is a contiguous datatype
 */
static inline int _mpc_dt_is_contiguous( mpc_lowcomm_datatype_t datatype )
{
	if ( ( SCTK_COMMON_DATA_TYPE_COUNT <= datatype ) && ( datatype < ( SCTK_COMMON_DATA_TYPE_COUNT + SCTK_USER_DATA_TYPES_MAX ) ) )
	{
		return 1;
	}

	return 0;
}

/** \brief Returns 1 if datatype is a derived datatype
 */
static inline int _mpc_dt_is_derived( mpc_lowcomm_datatype_t data_in )
{
	if ( ( data_in >= SCTK_COMMON_DATA_TYPE_COUNT + SCTK_USER_DATA_TYPES_MAX ) && ( data_in < ( SCTK_COMMON_DATA_TYPE_COUNT + 2 * SCTK_USER_DATA_TYPES_MAX ) ) )
	{
		return 1;
	}

	return 0;
}

/** \brief Returns 1 if datatype is a stuct datatype datatype
 */
static inline int _mpc_dt_is_struct( mpc_lowcomm_datatype_t data_in )
{
	if ( ( data_in >= SCTK_COMMON_DATA_TYPE_COUNT + SCTK_USER_DATA_TYPES_MAX ) && ( data_in < ( SCTK_COMMON_DATA_TYPE_COUNT + SCTK_USER_DATA_TYPES_MAX + MPC_STRUCT_DATATYPE_COUNT ) ) )
	{
		return 1;
	}

	return 0;
}

/** \brief Returns 1 if the datatype is a boundary (UB or LB)
 */
static inline int _mpc_dt_is_boundary( mpc_lowcomm_datatype_t data_in )
{
	if ( ( data_in == MPC_UB ) || ( data_in == MPC_LB ) )
	{
		return 1;
	}

	return 0;
}

/** \brief Returns 1 if the datatype is occupying a contiguous memory region
 */
static inline int _mpc_dt_is_contig_mem( mpc_lowcomm_datatype_t data_in )
{
	/* Note that the derived asumption can be optimized
   * for single segment derived with no LB/UB */
	if ( _mpc_dt_is_derived( data_in ) || _mpc_dt_is_boundary( data_in ) )
	{
		return 0;
	}

	return 1;
}

/** \brief This functions retuns the \ref mpc_dt_kind_t of an mpc_lowcomm_datatype_t
 *
 * 	It is useful to switch between datatypes
 *
 */
static inline mpc_dt_kind_t _mpc_dt_get_kind( mpc_lowcomm_datatype_t datatype )
{
	mpc_dt_kind_t ret = MPC_DATATYPES_UNKNOWN;

	if ( _mpc_dt_is_common( datatype ) || _mpc_dt_is_boundary( datatype ) )
	{
		ret = MPC_DATATYPES_COMMON;
	}
	else if ( _mpc_dt_is_contiguous( datatype ) )
	{
		ret = MPC_DATATYPES_CONTIGUOUS;
	}
	else if ( _mpc_dt_is_derived( datatype ) )
	{
		ret = MPC_DATATYPES_DERIVED;
	}

	return ret;
}

/** \brief Macros to translate from the global type ID to the ones local to each type (Common , Contiguous and derived)
 *
 *  */

/** \brief Takes a global contiguous type and computes its local offset */
#define _MPC_DT_MAP_TO_CONTIGUOUS( type ) ( type - SCTK_COMMON_DATA_TYPE_COUNT )
/** \brief Takes a local contiguous offset and translates it to a local offset */
#define _MPC_DT_MAP_FROM_CONTIGUOUS( type ) ( type + SCTK_COMMON_DATA_TYPE_COUNT )

/** \brief Takes a global derived type and computes its local offset */
#define _MPC_DT_MAP_TO_DERIVED( a ) ( a - SCTK_USER_DATA_TYPES_MAX - SCTK_COMMON_DATA_TYPE_COUNT )
/** \brief Takes a local derived offset and translates it to a local offset */
#define _MPC_DT_MAP_FROM_DERIVED( a ) ( a + SCTK_USER_DATA_TYPES_MAX + SCTK_COMMON_DATA_TYPE_COUNT )

/************************************************************************/
/* Datatype  Array                                                      */
/************************************************************************/

/** \brief This structure gathers contiguous and derived datatypes in the same lockable structure
 * 
 *  This structure is the entry point for user defined datatypes
 *  it is inintialized in \ref mpc_mpi_cl_per_mpi_process_ctx_t
 */
struct _mpc_dt_storage
{
	_mpc_dt_contiguout_t contiguous_user_types[SCTK_USER_DATA_TYPES_MAX]; /**< Contiguous datatype array */
	_mpc_dt_derived_t *derived_user_types[SCTK_USER_DATA_TYPES_MAX];	  /**< Derived datatype array */
	mpc_common_spinlock_t datatype_lock;								  /**< A lock protecting both datatypes types */
};

/** \brief Initializes the datatype array
 *
 *  This sets every datatype to not initialized
 *
 *  \param da A pointer to the datatype array to be initialized
 */
struct _mpc_dt_storage *_mpc_dt_storage_init();

/** \brief Helper funtion to release only allocated datatypes
 *
 *  \param da A pointer to the datatype array
 *  \param datatype The datatype to be freed
 */
int _mpc_dt_storage_type_can_be_released( struct _mpc_dt_storage *da, mpc_lowcomm_datatype_t datatype );

/** \brief Releases the datatype array and types not previously freed
 *  \param da A pointer to the datatype array
 */
void _mpc_dt_storage_release( struct _mpc_dt_storage *da );

/** \brief Returns a pointer to a contiguous datatype
 *  \param da A pointer to the datatype array
 *  \param datatype The datatype ID we want to retrieve
 *
 *  \return Returns the cell of the requested datatype (allocated or not !)
 *
 *  \warning The datatype must be a contiguous datatype note that event unallocated datatypes are returned !
 */
_mpc_dt_contiguout_t *_mpc_dt_storage_get_contiguous_datatype( struct _mpc_dt_storage *da, mpc_lowcomm_datatype_t datatype );

/** \brief Returns a pointer to a derived datatype
 *  \param da A pointer to the datatype array
 *  \param datatype The datatype ID we want to retrieve
 *
 *  \return NULL id not allocated a valid pointer otherwise
 *
 *  \warning The datatype must be a derived datatype
 */
_mpc_dt_derived_t *_mpc_dt_storage_get_derived_datatype( struct _mpc_dt_storage *da, mpc_lowcomm_datatype_t datatype );

/** \brief Sets a pointer to a contiguous datatype in the datatype array
 *  \param da A pointer to the datatype array
 *  \param datatype The datatype ID we want to set
 *  \param value the pointer to the datatype array
 *
 *  \return NULL id not allocated a valid pointer otherwise
 *
 *  \warning The datatype must be a derived datatype
 */
void _mpc_dt_storage_set_derived_datatype( struct _mpc_dt_storage *da, mpc_lowcomm_datatype_t datatype, _mpc_dt_derived_t *value );

/************************************************************************/
/* Datatype  Attribute Getters                                          */
/************************************************************************/

/** \brief Set a Datatype attr in a datatype-store (contained inside DT)
 *  \param da A pointer to the datatype array
 * 	\param type Target datatype
 * 	\param type_keyval Referenced keyval
 *  \param attribute_val Value to be stored
 *  \return MPI_SUCCESS if ok
 */
int _mpc_dt_attr_set( struct _mpc_dt_storage *da, mpc_lowcomm_datatype_t type,
					  int type_keyval, void *attribute_val );

/** \brief Get a Datatype attr in a datatype-store (contained inside DT)
 *  \param da A pointer to the datatype array
 * 	\param type Target datatype
 * 	\param type_keyval Referenced keyval
 *  \param attribute_val (OUT)Value of the attribute
 *  \param flag (OUT)False if no attribute found
 *  \return MPI_SUCCESS if ok
 */
int _mpc_dt_attr_get( struct _mpc_dt_storage *da, mpc_lowcomm_datatype_t type,
					  int type_keyval, void **attribute_val, int *flag );

/** \brief Delete a Datatype attr in a datatype-store (contained inside DT)
 *  \param da A pointer to the datatype array
 * 	\param type Target datatype
 * 	\param type_keyval Referenced keyval
 *  \return MPI_SUCCESS if ok
 */
int _mpc_dt_attr_delete( struct _mpc_dt_storage *da, mpc_lowcomm_datatype_t type,
						 int type_keyval );

/************************************************************************/
/* Datatype  Naming                                                     */
/************************************************************************/

/** \brief Set a name to a given datatype
 *  \param datatype Type which has to be named
 *  \param name Name we want to give
 *  \return 1 on error 0 otherwise
 *
 *  This version is used internally to set common data-type
 *  name we add it as we do not want the MPI_Type_set_name
 *  to allow this behaviour
 *
 */
int _mpc_dt_name_set_nocheck( mpc_lowcomm_datatype_t datatype, char *name );

/** \brief Set a name to a given datatype
 *  \param datatype Type which has to be named
 *  \param name Name we want to give
 *  \return 1 on error 0 otherwise
 *
 */
int _mpc_dt_name_set( mpc_lowcomm_datatype_t datatype, const char *name );

/** \brief Returns the name of a data-type
 *  \param datatype Requested data-type
 *  \return NULL if no name the name otherwise
 */
char *_mpc_dt_name_get( mpc_lowcomm_datatype_t datatype );

#endif /* MPC_DATATYPES_H */
