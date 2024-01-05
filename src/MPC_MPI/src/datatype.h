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

#include <stdlib.h>
#include <uthash.h>


#include "mpc_common_datastructure.h"
#include "mpc_common_spinlock.h"
#include "mpc_thread.h"
#include <mpc_common_asm.h>
#include <mpc_common_debug.h>
#include <mpc_mpi_comm_lib.h>

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
int _mpc_dt_keyval_create(MPC_Type_copy_attr_function * copy,
                          MPC_Type_delete_attr_function * delete,
                          int *type_keyval, void *extra_state);

/** \brief Delete a keyval
 *  \param type_keyval Delete a keyval
 *  \return MPI_SUCCESS if ok
 */
int _mpc_dt_keyval_free(int *type_keyval);

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
char *_mpc_dt_get_combiner_name(MPC_Type_combiner combiner);

/** \brief This datastructure is used to store the definition
 * of a datatype in order to reproduce it using MPI_Get_envelope
 * and MPI_Get_contents */
struct _mpc_dt_footprint
{
	/* Internal ref-counting handling */

	mpc_lowcomm_datatype_t  internal_type; /**< This is the internal type when types are built on top of each other happens for hvector and hindexed */

	/* MPI_get_envelope */
	MPC_Type_combiner       combiner; /**< Combiner used to build the datatype */
	int                     count;    /**< Number of item (as given in the data-type call) */
	int                     ndims;    /**< Number of dimensions (as given in the data-type call) */

	/* These three arrays are those returned by MPI_Type_get_contents
	 * they are defined in the standard as a pack of the
	 * parameters provided uppon type creation  */
	int *                   array_of_integers;  /** An array of integers */
	size_t *                array_of_addresses; /* An array of addresses */
	mpc_lowcomm_datatype_t *array_of_types;     /** An array of types */
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
	MPC_Type_combiner             combiner;
	int                           count;
	int                           size;
	int                           rank;
	int                           ndims;
	int                           order;
	int                           p;
	int                           r;
	mpc_lowcomm_datatype_t        oldtype;
	int                           blocklength;
	int                           stride;
	size_t                        stride_addr;
	size_t                        lb;
	size_t                        extent;
	const int *                   array_of_gsizes;
	const int *                   array_of_distribs;
	const int *                   array_of_dargs;
	const int *                   array_of_psizes;
	const int *                   array_of_blocklenght;
	const int *                   array_of_sizes;
	const int *                   array_of_subsizes;
	const int *                   array_of_starts;
	const int *                   array_of_displacements;
	const ssize_t *               array_of_displacements_addr;
	const mpc_lowcomm_datatype_t *array_of_types;
};

/** \brief Clears the external context
 *  \param ctx Context to clear
 */
void _mpc_dt_context_clear(struct _mpc_dt_context *ctx);

bool _mpc_dt_footprint_check_envelope(struct _mpc_dt_footprint *ref, struct _mpc_dt_footprint *candidate);

/** \brief Function used to factorize data-types by checking their equality
 *
 *  \param ref The datatype external ctx to be checked
 *  \param candidate The datatype we check against
 *  \return true if datatypes are equal false otherwise
 */
bool _mpc_dt_footprint_match(struct _mpc_dt_context *eref, struct _mpc_dt_footprint *candidate);

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
void _mpc_dt_context_set(struct _mpc_dt_footprint *ctx, struct _mpc_dt_context *dctx);

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
int _mpc_dt_fill_envelope(struct _mpc_dt_footprint *ctx, int *num_integers, int *num_addresses, int *num_datatypes, int *combiner);

/************************************************************************/
/* Datatype  Layout                                                     */
/************************************************************************/

/** \brief This structure is used to retrieve datatype layout from its context
 *  It is particularly useful in get_elements
 */
struct _mpc_dt_layout
{
	mpc_lowcomm_datatype_t type;
	size_t                 size;
};

/** \brief Retrieve a type layout from its context
 *  \param ctx Target datatype context
 *  \return NULL on error a list of datatype blocks otherwise
 */
struct _mpc_dt_layout *_mpc_dt_get_layout(struct _mpc_dt_footprint *ctx, size_t *ly_count);


/************************************************************************/
/* Contiguous Datatype                                                  */
/************************************************************************/

/** \brief Creates a contiguous datatype
 *
 * As all the datatypes kind are represented by the same structure,
 * this function is only a shortcut for the often used contiguous
 * datatypes. It calls the general initializers (\ref _mpc_dt_general_init)
 * with parameters of a contiguous one.
 *
 * \param type          Datatype handle on the new type to initialize
 * \param id_rank       ID of the user datatype
 * \param element_size  Size of one element of the contiguous datatype
 * \param count         Number of elements in the datatype
 * \param datatype      Datatype embedded in the new datatype
 */
void _mpc_dt_contiguous_create(mpc_lowcomm_datatype_t *type, const size_t id_rank,
                               const size_t element_size, const size_t count,
                               mpc_lowcomm_datatype_t datatype);

/************************************************************************/
/* General Datatype                                                     */
/************************************************************************/

/** \brief Theses macros allow us to manipulate the datatype refcounter more simply */
#define _MPC_DT_USER_IS_FREE(datatype_ptr)    ((datatype_ptr)->ref_count == 0)
#define _MPC_DT_USER_IS_USED(datatype_ptr)    ((datatype_ptr)->ref_count)

/** \brief Create a general datatype
 *
 * This function allocates and fill a general datatype structure
 *
 * \param id        Integer id of the new datatype (for debug)
 * \param count     Number of offsets to store
 * \param begins    List of starting offsets in the datatype
 * \param ends      List of end offsets in the datatype
 * \param datatypes List of datatypes for individual blocks
 * \param lb        Offset of type lower bound
 * \param is_lb     Tells if the type has a lowerbound
 * \param ub        Offset for type upper bound
 * \param is_b      Tells if the type has an upper bound
 *
 * \return The datatype created on success
 *         MPC_DATATYPE_NULL otherwise
 *
 */
mpc_lowcomm_datatype_t _mpc_dt_general_create(const unsigned int id,
                                              const unsigned long count,
                                              const long *const begins,
                                              const long *const ends,
                                              const mpc_lowcomm_datatype_t *const datatypes,
                                              const long lb, const bool is_lb,
                                              const long ub, const bool is_ub);

/** \brief Frees a datatype
 *
 * This function frees all the fields of a general datatype structure
 * If \ref enable_refcounting is true then the embedded datatypes are release.
 *
 * \warning This function doesn't lock the datatype storage array.
 * Make sure it is locked before calling it
 *
 * \param type_p             Pointer to the datatype to free
 * The value pointed by type_p is set to MPC_DATATYPE_NULL after freeing the structure.
 * \param enable_refcounting Should the function release the embedded datatypes
 *
 * \return MPC_LOWCOMM_SUCCESS on succes
 *         the appropriate error code otherwise
 */
int _mpc_dt_general_free(_mpc_lowcomm_general_datatype_t **type_p, const bool enable_refcounting);

/** \brief Releases a general datatype
 *
 * This function locks the datatype array. Please make sure it isn't already
 * locked before calling it.
 *
 *  \param type Type to be released
 *  \return MPC_LOWCOMM_SUCCESS on success
 *          the appropriate error code otherwise
 *
 *  \warning This call frees the container when the refcounter reaches 0
 *
 */
int _mpc_dt_general_release(mpc_lowcomm_datatype_t *type);

/** \brief Commits a datatype in the task-specific datatype array
 *
 *  \warning This function holds a lock when performing the commit
 *
 *  \param datatype_p A pointer on the datatype to commit
 *                    At exit it contained a commited datatype
 *                    or MPC_DATATYPE_NULL on error
 */
int _mpc_dt_general_on_slot(mpc_lowcomm_datatype_t *datatype_p);

/** \brief Retrieves a user defined datatype from its id
 *
 * \param idx ID of the datatype to retrieve
 * \return The datatype on success,
 *         MPI_DATATYPE_NULL otherwise
 */
mpc_lowcomm_datatype_t _mpc_dt_on_slot_get(const size_t idx);

/** \brief Get the minimum offset of a datatype (ignoring LB)
 *  \param type     Type which true lb is requested
 *  \param true_lb  True lb of the datatype
 *  \param true_ub  True ub of the datatype
 */
void _mpc_dt_general_true_extend(const _mpc_lowcomm_general_datatype_t *type, long *true_lb, long *true_ub);

/** \brief Try to optimize a general datatype (called by \ref PMPC_Commit)
 *
 *  \param target_type Type to be optimized
 */
int _mpc_dt_general_optimize(_mpc_lowcomm_general_datatype_t *target_type);

/** \brief Display debug informations about a general datatype
 *
 *  \param target_type Type to be displayed
 */
void _mpc_dt_general_display(mpc_lowcomm_datatype_t target_type);


/************************************************************************/
/* Datatype range calculations                                       */
/************************************************************************/

/**
 * Datatypes are pointers on struct stored in two separated array :
 *
 * 	- Common datatypes
 *  - User Defined datatypes
 *
 *  To check the type of a datatype, we look if its value is in the range of
 *  one of the arrays. It simplifies greatly checking if a pointer is valid or
 *  if a datatype can be released...
 *  The following are funtion to check the kind of datatype and its validness.
 *
 *  Therefore we need some functions to check wether a datatype is of a
 *  given type (meaning having an address in a given range).
 */

/** \brief List of datatype kinds
 *
 *  Note that as previously described a datatype can directly
 *  be identified from its value. This labelling is provided
 *  for convenience when "switching" between cases using \ref _mpc_dt_get_kind.
 *  This is even more relevant as we can't use pointer to switch without casting.
 */
typedef enum
{
	MPC_DATATYPES_UNKNOWN,   /**< This key is used to detect faulty datatypes */
	MPC_DATATYPES_COMMON,    /**< These are the common datatypes defined in \ref __mpc_common_types_init */
	MPC_DATATYPES_USER       /**< These are user defined datatypes of type \ref _mpc_dt_general_t */
} mpc_dt_kind_t;

/** \brief Returns true if the datatype is a boundary (UB or LB)
 *
 * \param data_in Datatype to test
 * \return true if the datatype is a boundary
 *         false otherwise
 */
static inline bool _mpc_dt_is_boundary(mpc_lowcomm_datatype_t data_in)
{
	if( (data_in == MPC_UB) || (data_in == MPC_LB) )
	{
		return true;
	}

	return false;
}

/** \brief Checks whether a datatype is a user defined one
 *
 * \param datatype Datatype to test
 * \return true if it is user defined, false otherwise
 */
bool _mpc_dt_is_user_defined(mpc_lowcomm_datatype_t datatype);

/** \brief Checks whether a datatype is valid or not
 *
 * This means if it has been created
 *
 * \param datatype Datatype to test
 * \return true if datatype is valid, false otherwise
 * */
static inline bool mpc_dt_is_valid(mpc_lowcomm_datatype_t datatype)
{
	return mpc_lowcomm_datatype_is_common(datatype) || _mpc_dt_is_user_defined(datatype);
}

/** \brief Checks whether a datatype has been commited or not
 *
 * This means if it is a common or a user defined one that have been commited
 *
 * \param datatype Datatype to test
 * \return true if datatype is commited, false otherwise
 * */
static inline bool mpc_dt_is_commited(mpc_lowcomm_datatype_t datatype)
{
	if(!mpc_dt_is_valid(datatype) )
	{
		return false;
	}
	if(_mpc_dt_is_user_defined(datatype) && !datatype->is_commited)
	{
		return false;
	}

	return true;
}

/** \brief This functions returns the \ref mpc_dt_kind_t of an mpc_lowcomm_datatype_t
 *
 * 	It is useful to switch between datatypes
 *
 * 	\param datatype Datatype to determine the kind of
 * 	\return Kind of the datatype see \ref mpc_dt_kind_t
 */
static inline mpc_dt_kind_t _mpc_dt_get_kind(mpc_lowcomm_datatype_t datatype)
{
	mpc_dt_kind_t ret = MPC_DATATYPES_UNKNOWN;

	if(mpc_lowcomm_datatype_is_common(datatype) || _mpc_dt_is_boundary(datatype) )
	{
		ret = MPC_DATATYPES_COMMON;
	}
	else if(_mpc_dt_is_user_defined(datatype) )
	{
		ret = MPC_DATATYPES_USER;
	}

	return ret;
}

/** \brief Returns the inner structure of a datatype
 *
 *  \param datatype Datatype to get the structure of
 *  \return A pointer to the _mpc_lowomm_general_datatype_t in the datatype if valid
 *          MPC_LOWCOMM_DATATYPE_NULL otherwise
 */
static inline mpc_lowcomm_datatype_t _mpc_dt_get_datatype(mpc_lowcomm_datatype_t datatype)
{
	mpc_lowcomm_datatype_t type = MPC_LOWCOMM_DATATYPE_NULL;

	if(mpc_dt_is_valid(datatype) )
	{
		if(mpc_lowcomm_datatype_is_common_predefined(datatype) )
		{
			type = mpc_lowcomm_datatype_common_get_type_struct(datatype);
		}
		else
		{
			type = datatype;
		}
	}

	return type;
}

/** \brief Returns true if the datatype is occupying a contiguous memory region
 *
 * \param data_in Datatype to test
 * \return true if data_in is contiguous in memory
 *         false otherwise
 */
static inline bool _mpc_dt_is_contig_mem(mpc_lowcomm_datatype_t data_in)
{
	/* Note that the general asumption can be optimized
	 * for single segment general with no LB/UB */
	if(mpc_dt_is_valid(data_in) &&
	   _mpc_dt_get_datatype(data_in)->opt_count == 1)
	{
		return true;
	}

	return false;
}

/************************************************************************/
/* Datatype  Array                                                      */
/************************************************************************/

/** \brief This structure gathers user defined datatypes in the same lockable structure
 *
 *  This structure is the entry point for user defined datatypes
 *  it is inintialized in \ref mpc_mpi_cl_per_mpi_process_ctx_t
 */
struct _mpc_dt_storage
{
	_mpc_lowcomm_general_datatype_t general_user_types[SCTK_USER_DATA_TYPES_MAX];     /**< general datatype array */
	mpc_common_spinlock_t           datatype_lock;                                    /**< A lock protecting datatypes */
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
 *  \param da       A pointer to the datatype array
 *  \param datatype The datatype to be freed
 */
bool _mpc_dt_storage_type_can_be_released(mpc_lowcomm_datatype_t datatype);

/** \brief Releases the datatype array and types not previously freed
 *  \param da A pointer to the datatype array
 */
void _mpc_dt_storage_release(struct _mpc_dt_storage *da);

/** \brief Returns a pointer to a general datatype
 *
 *  \param da       A pointer to the datatype array
 *  \param datatype The datatype ID we want to retrieve
 *
 *  \return NULL id not allocated a valid pointer otherwise
 *
 *  \warning The datatype must be a general datatype
 */
_mpc_lowcomm_general_datatype_t *_mpc_dt_storage_get_general_datatype(struct _mpc_dt_storage *da, const size_t datatype_idx);

/** \brief Sets a pointer to a datatype in the datatype array
 *
 *  \param da       A pointer to the datatype array
 *  \param datatype The datatype ID we want to set
 *  \param value    The datatype to insert in the array
 *
 *  \warning The datatype must be a general datatype
 */
void _mpc_dt_storage_set_general_datatype(struct _mpc_dt_storage *da, const size_t datatype_idx, mpc_lowcomm_datatype_t value);

/************************************************************************/
/* Datatype  Attribute Getters                                          */
/************************************************************************/

/** \brief Sets a Datatype attr in a datatype-store (contained inside DT)
 *  \param da A pointer to the datatype array
 * 	\param type Target datatype
 * 	\param type_keyval Referenced keyval
 *  \param attribute_val Value to be stored
 *  \return MPI_SUCCESS if ok
 */
int _mpc_dt_attr_set(struct _mpc_dt_storage *da, mpc_lowcomm_datatype_t type,
                     const int type_keyval, void *attribute_val);

/** \brief Gets a Datatype attr in a datatype-store (contained inside DT)
 *  \param da A pointer to the datatype array
 * 	\param type Target datatype
 * 	\param type_keyval Referenced keyval
 *  \param attribute_val (OUT)Value of the attribute
 *  \param flag (OUT)False if no attribute found
 *  \return MPI_SUCCESS if ok
 */
int _mpc_dt_attr_get(struct _mpc_dt_storage *da, mpc_lowcomm_datatype_t type,
                     const int type_keyval, void **attribute_val, int *flag);

/** \brief Deletes a Datatype attr in a datatype-store (contained inside DT)
 *  \param da A pointer to the datatype array
 * 	\param type Target datatype
 * 	\param type_keyval Referenced keyval
 *  \return MPI_SUCCESS if ok
 */
int _mpc_dt_attr_delete(struct _mpc_dt_storage *da, mpc_lowcomm_datatype_t type,
                        const int type_keyval);

/************************************************************************/
/* Datatype  Naming                                                     */
/************************************************************************/

/** \brief Sets a name to a given datatype
 *  \param datatype Type which has to be named
 *  \param name Name we want to give
 *  \return 1 on error 0 otherwise
 *
 *  This version is used internally to set common data-type
 *  name we add it as we do not want the MPI_Type_set_name
 *  to allow this behaviour
 *
 */
int _mpc_dt_name_set_nocheck(mpc_lowcomm_datatype_t datatype, const char *const name);

/** \brief Sets a name to a given datatype
 *  \param datatype Type which has to be named
 *  \param name Name we want to give
 *  \return 1 on error 0 otherwise
 *
 */
int _mpc_dt_name_set(mpc_lowcomm_datatype_t datatype, const char *const name);

/** \brief Returns the name of a data-type
 *  \param datatype Requested data-type
 *  \return NULL if no name the name otherwise
 */
char *_mpc_dt_name_get(mpc_lowcomm_datatype_t datatype);

#endif /* MPC_DATATYPES_H */
