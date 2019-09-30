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

#include "mpcmp.h" /* mpc_pack_absolute_indexes_t */
#include "mpc_common_asm.h"
#include "sctk_collective_communications.h" /* sctk_datatype_t */
#include "sctk_ht.h"
#include "mpc_common_spinlock.h"
#include "sctk_thread.h"
#include <stdlib.h>

/************************************************************************/
/* Datatype Init and Release                                            */
/************************************************************************/
/** \brief Are datatypes initialized ?*/
int sctk_datatype_initialized();

/** \brief Entry Point in the datatype code
 * Called from sctk_thread.c
 */
void sctk_datatype_init();

/** \brief Exit Point in the datatype code
 * Called from sctk_thread.c
 */
void sctk_datatype_release();

/************************************************************************/
/* Datatype  Attribute Handling                                         */
/************************************************************************/

/** \brief This structure defines a Datatype Keyval */
struct Datatype_Keyval {
  MPC_Type_copy_attr_function *copy;     /**< Keyval copy func */
  MPC_Type_delete_attr_function *delete; /**< Keyval delete func */
  void *extra_state;                     /**< An extra state to be stored */
  int free_cell; /**< Initally 0, 1 is the keyval is then freed */
};

/** \brief This function retrieves a keyval from an id
 * 	\param type_keyval ID of keyval to retrieve (offset in a static table)
 *  \return Pointer to keyval (or NULL if keyval freed or invalid)
 */
struct Datatype_Keyval *Datatype_Keyval_get(unsigned int type_keyval);

/** \brief This function deletes a keyval from an id
 * 	\param type_keyval ID of keyval to delete (offset in a static table)
 *  \return MPI_SUCCESS if ok
 */
int Datatype_Keyval_delete(unsigned int type_keyval);

/** \brief This function create a new keyval
 *  \param type_keyval (OUT) Id of the new keyval
 *  \return Pointer to a newly created keyval entry
 */
struct Datatype_Keyval *Datatype_Keyval_new(int *type_keyval);

/** \brief Trigger delete handler (if present) on a given data-type
 *  \param type_keyval Target KEYVAL
 *  \param type Source data-type
 */
void Datatype_Keyval_hit_delete(int type_keyval, void *attribute_val,
                                MPC_Datatype type);

/** \brief Trigger copy handler (if present) on a given data-type
 *  \param type_keyval Target KEYVAL
 *  \param oldtype old data-type
 *  \param attribute_val_in old data-type value
 *  \param attribute_val_out new data-type value
 */
void Datatype_Keyval_hit_copy(int type_keyval, MPC_Datatype oldtype,
                              void **attribute_val_in, void **attribute_val_out,
                              int *flag);

/** \brief Create a new data-type keyval
 *  \param copy Copy func
 *  \param delete Delete func
 *  \param type_keyval (OUT) ID of the newly create keyval
 *  \param extra_state Extra pointer to be stored in the keyval
 *  \return MPI_SUCCESS if ok
 */
int sctk_type_create_keyval(MPC_Type_copy_attr_function *copy,
                            MPC_Type_delete_attr_function *delete,
                            int *type_keyval, void *extra_state);

/** \brief Delete a keyval
 *  \param type_keyval Delete a keyval
 *  \return MPI_SUCCESS if ok
 */
int sctk_type_free_keyval(int *type_keyval);

/** \brief This defines a datatype ATTR storing the attribute val
 *          to be manipulalted by \ref Datatype_Attr_store
 */
struct Datatype_Attr {
  void *attribute_val; /**< Value of the attribute */
  int type_keyval;     /**< Referecne keyval */
};

/** \brief Create a new attr entry (alloc)
 *  \param type_keyval Associated keyval
 *  \param attribute_val Attribute to be stored
 *  \param Return a pointer to a newly allocated Attr (NULL on error)
 */
struct Datatype_Attr *Datatype_Attr_new(int type_keyval, void *attribute_val);

/** \brief This is where datatype attrs are stored (in each dt)
 */
struct Datatype_Attr_store {
  struct MPCHT attrs; /**< Just an ATTR ht */
};

/** \brief Initialize a data-type store
 *  \param store Pointer to the target store
 *  \return 0 on success
 */
int Datatype_Attr_store_init(struct Datatype_Attr_store *store);

/** \brief Release a data-type store
 *  \param store Pointer to the target store
 *  \param container_type Datatype being released
 *  \return 0 on success
 */
int Datatype_Attr_store_release(struct Datatype_Attr_store *store,
                                MPC_Datatype container_type);

/************************************************************************/
/* Datatype  Context                                                    */
/************************************************************************/

/** \brief Retrieve a combiner name
 *  \param combiner Target combiner
 *  \return a static char describing the combiner
 */
char * sctk_datype_combiner(MPC_Type_combiner combiner);

/** \brief This datastructure is used to store the definition
 * of a datatype in order to reproduce it using MPI_Get_envelope
 * and MPI_Get_contents */
struct Datatype_context
{
	/* Internal ref-counting handling */

	MPC_Datatype internal_type; /**< This is the internal type when types are built on top of each other happens for hvector and hindexed */

	/* MPI_get_envelope */
	MPC_Type_combiner combiner; /**< Combiner used to build the datatype */
	int count; /**< Number of item (as given in the data-type call) */
	int ndims; /**< Number of dimensions (as given in the data-type call) */

	/* These three arrays are those returned by MPI_Type_get_contents
	 * they are defined in the standard as a pack of the 
	 * parameters provided uppon type creation  */
	int * array_of_integers; /** An array of integers */
	MPC_Aint * array_of_addresses; /* An array of addresses */
	MPC_Datatype * array_of_types; /** An array of types */
};

/** \brief Clears the context
 *  \param ctx Context to clear
 */
void sctk_datatype_context_clear( struct Datatype_context * ctx );

/** \brief This structure is used to pass the parameters to \ref sctk_datatype_context_set
 *  
 *  We use a structure as we don't want to polute the calling context
 *  with all the variants comming from the shape of the callers.
 *  Here the calling type function just set what's needed and
 *  leave the rest NULL. To be sure that errors will be easy to check
 *  the called must call \ref sctk_datatype_external_context_clear to
 *  allow further initialization checks.
 * 
 *  We took the names directly from the standard in order
 *  to make things easier. See standard p. 120 -> 123
 *  here in 3.0
 * 
 */
struct Datatype_External_context
{
	MPC_Type_combiner combiner;
	int count;
	int size;
	int rank;
	int ndims;
	int order;
	int p;
	int r;
	MPC_Datatype oldtype;
	int blocklength;
	int stride;
	MPC_Aint stride_addr;
	MPC_Aint lb;
	MPC_Aint extent;
	int * array_of_gsizes;
	int * array_of_distribs;
	int * array_of_dargs;
	int * array_of_psizes;
	int * array_of_blocklenght;
	int * array_of_sizes;
	int * array_of_subsizes;
	int * array_of_starts;
	int * array_of_displacements;
	MPC_Aint * array_of_displacements_addr;
	MPC_Datatype * array_of_types;
};

/** \brief Clears the external context
 *  \param ctx Context to clear
 */
void sctk_datatype_external_context_clear( struct Datatype_External_context * ctx );


/** \brief Function used to factorize data-types by checking their equality
 *  
 *  \param ref The datatype external ctx to be checked
 *  \param candidate The datatype we check against
 *  \return 1 if datatypes are equal 0 otherwise
 */
int Datatype_context_match( struct Datatype_External_context * eref, struct Datatype_context * candidate );



/** \brief Setup the datatype context
 *  
 *  \param ctx Context to fill
 *  \param dctx Datatype context
 * 
 *  Note that it is this function which actually generate
 *  the data returned by \ref MPI_Type_Get_contents
 *  and store them in the \ref Datatype_context structure.
 *  We have no shuch structure for common data-types as it
 *  is not allowed to call \ref MPI_Type_Get_contents on them
 */
void sctk_datatype_context_set( struct Datatype_context * ctx , struct Datatype_External_context * dctx );
/* This is the same function but with the possibility of disabling refcounting (to use in type match) */
void __sctk_datatype_context_set( struct Datatype_context * ctx , struct Datatype_External_context * dctx, int enable_refcounting  );

/** \brief Frees the data stored in a data-type context
 *  \param ctx Context to free
 */
void sctk_datatype_context_free( struct Datatype_context * ctx );


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
int sctk_datatype_fill_envelope( struct Datatype_context * ctx , int * num_integers, int * num_addresses , int * num_datatypes , int * combiner );

/************************************************************************/
/* Datatype  Layout                                                     */
/************************************************************************/

/** \brief This structure is used to retrieve datatype layout from its context
 *  It is particularly useful in get_elements
 */
struct Datatype_layout
{
	MPC_Datatype type;
	MPC_Aint size;
};

/** \brief Retrieve a type layout from its context 
 *  \param ctx Target datatype context
 *  \return NULL on error a list of datatype blocks otherwise
 */
struct Datatype_layout * sctk_datatype_layout( struct Datatype_context * ctx, size_t * ly_count );


/************************************************************************/
/* Common Datatype                                                      */
/************************************************************************/

/** \brief Returns 1 if datatype is a common datatype
 */
static inline int sctk_datatype_is_common( MPC_Datatype datatype )
{
	if( (0 <= datatype)
	&&  ( datatype < SCTK_COMMON_DATA_TYPE_COUNT ) )
	{
		return 1;
	}
	
	return 0;
}

/** \brief Initilalize common datatype sizes
 * This function is called in \ref sctk_start_func
 */
void sctk_common_datatype_init();

/* This is the internal array holding common type sizes*/
extern size_t * __sctk_common_type_sizes;
/** \brief Get common datatypes sizes
 *  
 *  \warning This function should only be called after MPC init
 *  it aborts if not provided with a common datatype
 * 
 *  \param datatype target common datatype
 *  \return datatype size
 */
static inline size_t sctk_common_datatype_get_size( MPC_Datatype datatype )
{
  assert(sctk_datatype_is_common(datatype));
  return __sctk_common_type_sizes[datatype];
}


/** \brief Initialize Common data-types based on derived ones
 *  This is called at mpi tasc context creation
 */
void init_composed_common_types();

/** \brief Release Common data-types based on derived ones
 *  This is called at mpi tasc context free
 */
void release_composed_common_types();

/** \brief Display debug informations about a common datatype
 *  \param target_type Type to be displayed
 */
void sctk_common_datatype_display( MPC_Datatype datatype );


/************************************************************************/
/* Contiguous Datatype                                                  */
/************************************************************************/

/** \brief This structure describes continuous datatypes
 *  Such datatypes are continuous packs of other datatypes
 *  This structure is initialized in \ref PMPC_Type_hcontiguous
 */
typedef struct 
{
	size_t id_rank; /**< Identifier of the contiguous type which is also its offset in the contiguous type table*/
	size_t size; /**< Total size of the contiguous type */
	size_t element_size; /**< Size of an element of type */
	size_t count; /**< Number of elements of type "datatype" in the type */
	sctk_datatype_t datatype; /**< Type packed within the datatype */
	unsigned int ref_count; /**< Flag telling if the datatype slot is free for use */
	struct Datatype_context context; /**< Saves the creation context for MPI_get_envelope & MPI_Get_contents */
        struct Datatype_Attr_store attrs; /**< ATTR array for this type */
} sctk_contiguous_datatype_t;

/** \brief sctk_contiguous_datatype_t initializer
 *  this function is called from \ref PMPC_Type_hcontiguous
 *
 *  \param type Type to be initialized
 *  \param id_rank unique identifier of the  type which is also its offset in the contiguous type array
 *  \param element_size Size of a datatype element
 *  \param count Number of element 
 *  \param source original datatype id
 * 
 */
void sctk_contiguous_datatype_init( sctk_contiguous_datatype_t * type , size_t id_rank , size_t element_size, size_t count, sctk_datatype_t datatype );

/** \brief Releases a contiguous datatype
 *  \param type This is the datatype to be freed
 * 
 *	\warning This call does not free the container it empties the content whent the refcounter reaches 0
 */
void sctk_contiguous_datatype_release( sctk_contiguous_datatype_t * type );

/** \brief Theses macros allow us to manipulate the contiguous datatype refcounter more simply */
#define SCTK_CONTIGUOUS_DATATYPE_IS_FREE( datatype_ptr ) (datatype_ptr->ref_count == 0)
#define SCTK_CONTIGUOUS_DATATYPE_IS_IN_USE( datatype_ptr ) (datatype_ptr->ref_count)


/** \brief Display debug informations about a contiguous datatype
 *  \param target_type Type to be displayed
 */
void sctk_contiguous_datatype_display( sctk_contiguous_datatype_t * target_type );




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
	MPC_Datatype id; /**< Integer ID (useful  for debug) */
	/* Context */
	size_t size; /**< Total size of the datatype */
	unsigned long count; /**< Number of elements in the datatype */
	unsigned int  ref_count; /**< Ref counter to manage freeing */

	/* Content */
	mpc_pack_absolute_indexes_t *begins; /**< Begin offsets */
	mpc_pack_absolute_indexes_t *ends; /**< End offsets */
	
	/* Optimized Content */
	unsigned long opt_count; /**< Number of blocks with optimization */
	mpc_pack_absolute_indexes_t *opt_begins; /**< Begin offsets with optimization */
	mpc_pack_absolute_indexes_t *opt_ends; /**< End offsets with optimization */
	sctk_datatype_t * datatypes; /**< Datatypes for each block */
	
	/* Bounds */
	mpc_pack_absolute_indexes_t lb; /**< Lower bound offset  */
	int is_lb; /**< Does type has a lower bound */
	mpc_pack_absolute_indexes_t ub; /**< Upper bound offset */
	int is_ub; /**< Does type has an upper bound */
	int is_a_padded_struct; /**< Was the type padded with UB during construction ? */
	
	/* Context */
	struct Datatype_context context; /**< Saves the creation context for MPI_get_envelope & MPI_Get_contents */

        /* Attrs */
        struct Datatype_Attr_store attrs; /**< ATTR array for this type */
} sctk_derived_datatype_t;


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
void sctk_derived_datatype_init(sctk_derived_datatype_t *type, MPC_Datatype id,
                                unsigned long count,
                                mpc_pack_absolute_indexes_t *begins,
                                mpc_pack_absolute_indexes_t *ends,
                                sctk_datatype_t *datatypes,
                                mpc_pack_absolute_indexes_t lb, int is_lb,
                                mpc_pack_absolute_indexes_t ub, int is_ub);

/** \brief Releases a derived datatype
 * 
 *  \param type Type to be released
 *  
 *  \warning This call frees the container when the refcounter reaches 0
 * 
 */
int sctk_derived_datatype_release( sctk_derived_datatype_t * type );

/** \brief Get the minimum offset of a derieved datatype (ignoring LB)
 *  \param type Type which true lb is requested
 *  \param true_lb the true lb of the datatype
 *  \param size size true LB to true UB
 */
void sctk_derived_datatype_true_extent( sctk_derived_datatype_t * type , mpc_pack_absolute_indexes_t * true_lb, mpc_pack_absolute_indexes_t * true_ub);

/** \brief This stucture is used to optimize a derived datatype
 *  It is easier to manipulate pairs of begin and ends instead of two arrays
 */
struct Derived_datatype_cell
{
	mpc_pack_absolute_indexes_t begin;
	mpc_pack_absolute_indexes_t end;
	MPC_Datatype type;
	short ignore;
};


/** \brief Try to optimize a derived datatype (called by \ref PMPC_Commit)
 *  \param target_type Type to be optimized
 */
int sctk_derived_datatype_optimize( sctk_derived_datatype_t * target_type );

/** \brief Display debug informations about a derived datatype
 *  \param target_type Type to be displayed
 */
void sctk_derived_datatype_display( sctk_derived_datatype_t * target_type );

/** \brief This struct is used to serialize these boundaries all
 * at once instead of having to do it one by one (see two next functions) */
struct inner_lbub {
  mpc_pack_absolute_indexes_t lb; /**< Lower bound offset  */
  int is_lb;                      /**< Does type has a lower bound */
  mpc_pack_absolute_indexes_t ub; /**< Upper bound offset */
  int is_ub;                      /**< Does type has an upper bound */
  int is_a_padded_struct; /**< Was the type padded with UB during construction ?
                             */
};

/** \brief Serialize a derived datatype in a contiguous segment
 *  \param type the derived data-type to be serialized
 *  \param size (OUT) the size of the serialize buffer
 *  \param header_pad offset to allocate for the header (data will be shifted)
 *  \return the allocated buffer of size (size) to be used and freed
 */
void *sctk_derived_datatype_serialize(sctk_datatype_t type, size_t *size,
                                      size_t header_pad);

/** \brief Deserialize a derived datatype from a contiguous segment
 *  \param buff the buffer from a previously serialized datatype
 *  \param size size of the input buffer (as generated during serialize)
 *  \param header_pad offset to skip as being the header
 *  \return a new datatype matching the serialized one (to be freed)
 */
sctk_datatype_t sctk_derived_datatype_deserialize(void *buff, size_t size,
                                                  size_t header_pad);

/** \brief This function gets the basic type constituing a derived type for RMA
 *  \param type Derived type to be checked
 *  \return -1 if types are differing, the type if not
 */
sctk_datatype_t sctk_datatype_get_inner_type(sctk_datatype_t type);

/************************************************************************/
/* Datatype ID range calculations                                       */
/************************************************************************/

/** 
 * Datatypes are identified in function of their position in a contiguous
 * array such as :
 * 
 * 	Common datatypes ==> [ 0 , SCTK_COMMON_DATA_TYPE_COUNT[
 *  Contiguous datatypes => [ SCTK_COMMON_DATA_TYPE_COUNT, SCTK_USER_DATA_TYPES_MAX[
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
 *  for convenience when "switching" between cases using \ref sctk_datatype_kind.
 */
typedef enum
{
	MPC_DATATYPES_COMMON, /**< These are the common datatypes defined in \ref sctk_common_datatype_init */
	MPC_DATATYPES_CONTIGUOUS, /**< These are contiguous datatypes of type \ref sctk_contiguous_datatype_t */
	MPC_DATATYPES_DERIVED, /**< These are derived datatypes of type \ref sctk_derived_datatype_t */
	MPC_DATATYPES_UNKNOWN /**< This key is used to detect faulty datatypes IDs */
} MPC_Datatype_kind;

/** \brief Returns 1 if datatype is a contiguous datatype
 */
static inline int sctk_datatype_is_contiguous( MPC_Datatype datatype )
{
	if( (SCTK_COMMON_DATA_TYPE_COUNT <= datatype)
	&&  ( datatype < ( SCTK_COMMON_DATA_TYPE_COUNT + SCTK_USER_DATA_TYPES_MAX ) ) )
	{
		return 1;
	}
	
	return 0;
}

/** \brief Returns 1 if datatype is a derived datatype
 */
static inline int sctk_datatype_is_derived (MPC_Datatype data_in)
{
	if ((data_in >= SCTK_COMMON_DATA_TYPE_COUNT + SCTK_USER_DATA_TYPES_MAX) 
	&& (data_in < (SCTK_COMMON_DATA_TYPE_COUNT + 2 * SCTK_USER_DATA_TYPES_MAX)))
	{
		return 1;
	}

	return 0;
}

/** \brief Returns 1 if datatype is a stuct datatype datatype
 */
static inline int sctk_datatype_is_struct_datatype (MPC_Datatype data_in)
{
	if ((data_in >= SCTK_COMMON_DATA_TYPE_COUNT + SCTK_USER_DATA_TYPES_MAX) 
	&& (data_in < ( SCTK_COMMON_DATA_TYPE_COUNT + SCTK_USER_DATA_TYPES_MAX + MPC_STRUCT_DATATYPE_COUNT)))
	{
		return 1;
	}

	return 0;
}

/** \brief Returns 1 if the datatype is a boundary (UB or LB)
 */
static inline int sctk_datatype_is_boundary (MPC_Datatype data_in)
{
	if ((data_in == MPC_UB)
	||  (data_in == MPC_LB))
	{
		return 1;
	}

	return 0;
}

/** \brief Returns 1 if the datatype is occupying a contiguous memory region
 */
static inline int sctk_datatype_contig_mem(MPC_Datatype data_in) {
  /* Note that the derived asumption can be optimized
   * for single segment derived with no LB/UB */
  if (sctk_datatype_is_derived(data_in) || sctk_datatype_is_boundary(data_in)) {
    return 0;
  }

  return 1;
}

/** \brief This functions retuns the \ref MPC_Datatype_kind of an MPC_Datatype
 * 
 * 	It is useful to switch between datatypes
 * 
 */
 static inline MPC_Datatype_kind sctk_datatype_kind( MPC_Datatype datatype )
 {
	MPC_Datatype_kind ret = MPC_DATATYPES_UNKNOWN;
	
	if ( sctk_datatype_is_common( datatype ) || sctk_datatype_is_boundary( datatype ) )
	{
		ret = MPC_DATATYPES_COMMON;
	}
	else if( sctk_datatype_is_contiguous( datatype ) )
	{
		ret = MPC_DATATYPES_CONTIGUOUS;
	}
	else if( sctk_datatype_is_derived( datatype ) )
	{
		ret = MPC_DATATYPES_DERIVED;
	}
	
	return ret;
 }


/** \brief Macros to translate from the global type ID to the ones local to each type (Common , Contiguous and derived)
 * 
 *  */

/** \brief Takes a global contiguous type and computes its local offset */
#define MPC_TYPE_MAP_TO_CONTIGUOUS( type ) ( type - SCTK_COMMON_DATA_TYPE_COUNT)
/** \brief Takes a local contiguous offset and translates it to a local offset */
#define MPC_TYPE_MAP_FROM_CONTIGUOUS( type ) ( type + SCTK_COMMON_DATA_TYPE_COUNT)

/** \brief Takes a global derived type and computes its local offset */
#define MPC_TYPE_MAP_TO_DERIVED( a ) ( a - SCTK_USER_DATA_TYPES_MAX - SCTK_COMMON_DATA_TYPE_COUNT)
/** \brief Takes a local derived offset and translates it to a local offset */
#define MPC_TYPE_MAP_FROM_DERIVED( a ) ( a + SCTK_USER_DATA_TYPES_MAX + SCTK_COMMON_DATA_TYPE_COUNT)



/************************************************************************/
/* Datatype  Array                                                      */
/************************************************************************/

/** \brief This structure gathers contiguous and derived datatypes in the same lockable structure
 * 
 *  This structure is the entry point for user defined datatypes
 *  it is inintialized in \ref sctk_task_specific_t
 */
struct Datatype_Array
{
	sctk_contiguous_datatype_t contiguous_user_types[SCTK_USER_DATA_TYPES_MAX]; /**< Contiguous datatype array */
	sctk_derived_datatype_t * derived_user_types[SCTK_USER_DATA_TYPES_MAX];  /**< Derived datatype array */
	mpc_common_spinlock_t datatype_lock; /**< A lock protecting both datatypes types */
};

/** \brief Initializes the datatype array
 *  
 *  This sets every datatype to not initialized
 * 
 *  \param da A pointer to the datatype array to be initialized
 */
struct Datatype_Array * Datatype_Array_init();

/** \brief Helper funtion to release only allocated datatypes 
 * 
 *  \param da A pointer to the datatype array
 *  \param datatype The datatype to be freed
 */
int Datatype_Array_type_can_be_released( struct Datatype_Array * da, MPC_Datatype datatype );

/** \brief Releases the datatype array and types not previously freed
 *  \param da A pointer to the datatype array
 */
void Datatype_Array_release( struct Datatype_Array * da );

/** \brief Locks the datatype array 
 *  \param da A pointer to the datatype array we want to lock
 */
static inline void Datatype_Array_lock(void)
{

}

/** \brief Unlocks the datatype array 
 *  \param da A pointer to the datatype array we want to unlock
 */
static inline void Datatype_Array_unlock(void)
{

}

/** \brief Returns a pointer to a contiguous datatype
 *  \param da A pointer to the datatype array
 *  \param datatype The datatype ID we want to retrieve
 * 
 *  \return Returns the cell of the requested datatype (allocated or not !)
 * 
 *  \warning The datatype must be a contiguous datatype note that event unallocated datatypes are returned !
 */
sctk_contiguous_datatype_t *  Datatype_Array_get_contiguous_datatype( struct Datatype_Array * da ,  MPC_Datatype datatype);

/** \brief Returns a pointer to a derived datatype
 *  \param da A pointer to the datatype array
 *  \param datatype The datatype ID we want to retrieve
 * 
 *  \return NULL id not allocated a valid pointer otherwise
 * 
 *  \warning The datatype must be a derived datatype
 */
sctk_derived_datatype_t * Datatype_Array_get_derived_datatype( struct Datatype_Array * da  ,  MPC_Datatype datatype);

/** \brief Sets a pointer to a contiguous datatype in the datatype array
 *  \param da A pointer to the datatype array
 *  \param datatype The datatype ID we want to set
 *  \param value the pointer to the datatype array
 * 
 *  \return NULL id not allocated a valid pointer otherwise
 * 
 *  \warning The datatype must be a derived datatype
 */
void Datatype_Array_set_derived_datatype( struct Datatype_Array * da ,  MPC_Datatype datatype, sctk_derived_datatype_t * value );

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
int sctk_type_set_attr(struct Datatype_Array *da, MPC_Datatype type,
                       int type_keyval, void *attribute_val);

/** \brief Get a Datatype attr in a datatype-store (contained inside DT)
 *  \param da A pointer to the datatype array
 * 	\param type Target datatype
 * 	\param type_keyval Referenced keyval
 *  \param attribute_val (OUT)Value of the attribute
 *  \param flag (OUT)False if no attribute found
 *  \return MPI_SUCCESS if ok
 */
int sctk_type_get_attr(struct Datatype_Array *da, MPC_Datatype type,
                       int type_keyval, void *attribute_val, int *flag);

/** \brief Delete a Datatype attr in a datatype-store (contained inside DT)
 *  \param da A pointer to the datatype array
 * 	\param type Target datatype
 * 	\param type_keyval Referenced keyval
 *  \return MPI_SUCCESS if ok
 */
int sctk_type_delete_attr(struct Datatype_Array *da, MPC_Datatype type,
                          int type_keyval);

/************************************************************************/
/* Datatype  Naming                                                     */
/************************************************************************/

/** \brief This struct is used to store the name given to a datatype
 * 
 *  We privileged an hash table instead of a static  array to
 *  save memory as type naming is really a secondary feature
 *  (Here we need it for MPICH Datatype tests)
 */
struct Datatype_name_cell
{
	MPC_Datatype datatype;
	char name[MPC_MAX_OBJECT_NAME]; /**< Name given to the datatype */
	UT_hash_handle hh; /**< This dummy data structure is required by UTHash is order to make this data structure hashable */
};


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
int sctk_datype_set_name_nocheck( MPC_Datatype datatype, char * name );

/** \brief Set a name to a given datatype
 *  \param datatype Type which has to be named
 *  \param name Name we want to give
 *  \return 1 on error 0 otherwise
 * 
 */
int sctk_datype_set_name( MPC_Datatype datatype, char * name );


/** \brief Returns the name of a data-type
 *  \param datatype Requested data-type
 *  \return NULL if no name the name otherwise
 */
char * sctk_datype_get_name( MPC_Datatype datatype );


/** \brief Release Data-types names
 */
void sctk_datype_name_release();

#endif /* MPC_DATATYPES_H */
