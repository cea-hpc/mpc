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

#include "sctk_thread.h"
#include "sctk_collective_communications.h" /* sctk_datatype_t */
#include "mpcmp.h" /* mpc_pack_absolute_indexes_t */

/************************************************************************/
/* Common Datatype                                                      */
/************************************************************************/

/** \brief Initilalize common datatype sizes
 * This function is called in \ref sctk_start_func
 */
void sctk_common_datatype_init();

/** \brief Get common datatypes sizes
 *  
 *  \warning This function should only be called after MPC init
 *  it aborts if not provided with a common datatype
 * 
 *  \param datatype target common datatype
 *  \return datatype size
 */
size_t sctk_common_datatype_get_size( MPC_Datatype datatype );  


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
	size_t count; /**< Number of elements of type "datatype" in the type */
	sctk_datatype_t datatype; /**< Type packed within the datatype */
	int used; /**< Flag telling if the datatype slot is free for use */
} sctk_contiguous_datatype_t;

/** \brief sctk_contiguous_datatype_t initializer
 *  \this function is called from \ref PMPC_Type_hcontiguous
 */
void sctk_contiguous_datatype_init( sctk_contiguous_datatype_t * type , size_t id_rank , size_t size, size_t count, sctk_datatype_t datatype );
void sctk_contiguous_datatype_release( sctk_contiguous_datatype_t * type );

/************************************************************************/
/* Derived Datatype                                                     */
/************************************************************************/


typedef struct
{
	size_t size;
	size_t nb_elements;
	mpc_pack_absolute_indexes_t *begins;
	mpc_pack_absolute_indexes_t *ends;
	unsigned long count;
	unsigned long ref_count;
	mpc_pack_absolute_indexes_t lb;
	mpc_pack_absolute_indexes_t ub;
	int is_lb;
	int is_ub;
} sctk_derived_datatype_t;

void sctk_derived_datatype_init( sctk_derived_datatype_t * type ,  unsigned long count,
                                 mpc_pack_absolute_indexes_t * begins,  mpc_pack_absolute_indexes_t * ends,
                                 mpc_pack_absolute_indexes_t lb, int is_lb,
						         mpc_pack_absolute_indexes_t ub, int is_ub);
void sctk_derived_datatype_release( sctk_derived_datatype_t * type );

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
	&& (data_in < SCTK_COMMON_DATA_TYPE_COUNT + 2 * SCTK_USER_DATA_TYPES_MAX))
	{
		return 1;
	}

	return 0;
}

/** \brief This functions retuns the \ref MPC_Datatype_kind of an MPC_Datatype
 * 
 * 	It is useful to switch between datatypes
 * 
 */
 static inline MPC_Datatype_kind sctk_datatype_kind( MPC_Datatype datatype )
 {
	MPC_Datatype_kind ret = MPC_DATATYPES_UNKNOWN;
	
	if ( sctk_datatype_is_common( datatype ) )
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

/** Takes a global contiguous type and computes its local offset */
#define MPC_TYPE_MAP_TO_CONTIGUOUS( type ) ( type - SCTK_COMMON_DATA_TYPE_COUNT)
/** Takes a local contiguous offset and translates it to a local offset */
#define MPC_TYPE_MAP_FROM_CONTIGUOUS( type ) ( type + SCTK_COMMON_DATA_TYPE_COUNT)

/** Takes a global derived type and computes its local offset */
#define MPC_TYPE_MAP_TO_DERIVED( a ) ( a - SCTK_USER_DATA_TYPES_MAX - SCTK_COMMON_DATA_TYPE_COUNT)
/** Takes a local derived offset and translates it to a local offset */
#define MPC_TYPE_MAP_FROM_DERIVED( a ) ( a + SCTK_USER_DATA_TYPES_MAX + SCTK_COMMON_DATA_TYPE_COUNT)








#endif /* MPC_DATATYPES_H */
