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
#ifndef MPC_INFO_H
#define MPC_INFO_H

#include "uthash.h"

/***************
* MPC_Info_key *
***************/

/**
 * \brief List of keys and values
 *
 *  This is the actual key container reffered to by \ref MPC_Info_cell which is a DL list
 * 	as it has to support deletion.
 */
struct MPC_Info_key
{
	char * key, *value; /**< Key and value of the stored data */
	struct MPC_Info_key * prev; /**< DL list prev links */
	struct MPC_Info_key * next; /**< DL list next links */
};

/* Init an release */

/** \brief Creates a new Key entry
 *
 *  \param key New key
 *  \param value Data associated with this key
 *
 * 	\return A pointer to a newly allocated key object (NULL on error)
 */
struct MPC_Info_key * MPC_Info_key_init( char * key, char * value );

/** \brief Release a single key entry
 *
 * 	\warning Not to use on statically allocated keys
 *
 *  \param tofree Entry to be freed (also frees the container !)
 *
 */
void MPC_Info_key_release( struct MPC_Info_key * tofree );

/** \brief Frees a full list of key by calling \ref MPC_Info_key_release recursivelly
 *
 *  \param tofree HEAD of the list to be freed
 *
 */
void MPC_Info_key_release_recursive( struct MPC_Info_key * tofree );

/* Getters and setters */

/** \brief Get the entry associated with a key
 *
 *  \param head Head of the key list to look in
 *  \param key Key to look for
 *
 * 	\return A pointer to a \ref MPC_Info_key if found NULL otherwise
 *
 */
struct MPC_Info_key * MPC_Info_key_get( struct MPC_Info_key * head, char * key );

/** \brief Set or creates the entry associated with a key
 *
 *  \param head Head of the key list to append to
 *  \param key Key to add / modify
 *  \param value Data to add / update
 *
 * 	\return The new head if it has been updated
 *
 */
struct MPC_Info_key * MPC_Info_key_set( struct MPC_Info_key * head, char * key, char * value );

/** \brief Get the nth entry in the key list
 *
 *  \param head Head of the key list to look in
 *  \param n rank of the entry to look for
 *
 * 	\return A pointer to a \ref MPC_Info_key if found NULL otherwise
 *
 */
struct MPC_Info_key * MPC_Info_key_get_nth( struct MPC_Info_key * head, int n );

/** \brief Get number of keys in the list
 *
 *  \param head Head of the key list to look in
 *
 * 	\return The number of keys in the list
 *
 */
int MPC_Info_key_count( struct MPC_Info_key * head );

/* Methods */

struct MPC_Info_key * MPC_Info_key_pop( struct MPC_Info_key * topop );
struct MPC_Info_key * MPC_Info_key_delete( struct MPC_Info_key * head, char * key , int * flag );

/****************
* MPC_Info_cell *
****************/

/**
 * \brief This is the actual structure shadowed by \ref MPI_Info and MPC_Info
 *
 *  This structure is a simple key value data-store as defined in the MPI standard
 */
struct MPC_Info_cell
{
	int rank; /**< Rank of the process which created the info just to make sure it is not passed arround */
	int id;  /**< Identifier of the info in order to allow lookup by int */

	struct MPC_Info_key * keys; /**< Here is the actual payload which is a list of keys */
	UT_hash_handle hh; /**< This dummy data structure is required by UTHash is order to make this data structure hashable */
};


/* Init an release */

/** \brief Initializes an empty info cell
 * 	\param id Unique id for this info cell as provided by \ref MPC_Info_factory
 * */
struct MPC_Info_cell * MPC_Info_cell_init( int id );

/** \brief Releases an info cell and all its keys
 *  \param cell pointer to the cell to be released
 *  \return a pointer to a newly initalized cell
 *
 * 	\warning Be carefull the cell is not freed (only its content)
 */
void MPC_Info_cell_release( struct MPC_Info_cell * cell );

/* Getters and setters */

/** \brief Get a key in the cell and store it in the buff
 *
 *   \param cell Cell where to look for the key
 *   \param key Unique Key of what to look for
 *   \param dest Buffer where the content will be stored
 *   \param maxlen Length of the buffer where to store data
 *   \param flag set to "true" if the  key was found
 *
 * 	 \return 1 on error 0 otherwise
 *
 *   \warning Note that if dest is too small (as defined by maxlen) buffer is truncated
 */
int MPC_Info_cell_get( struct MPC_Info_cell * cell , char * key , char * dest, int maxlen, int * flag );

/** \brief Set a value in the cell
 *
 * 	\param cell Cell where to store the data
 *  \param key Unique key of where to store the data
 *  \param value Buffer where the string to store is located (NULL terminated !)
 *  \param overwrite defines if the data must be overwriten or not if already present
 *
 *  \warning The maximum length of the value parameter is set by the MPC_MAX_INFO_VAL value
 *
 * 	 \return 1 on error 0 otherwise
 *
 */
int MPC_Info_cell_set( struct MPC_Info_cell * cell , char * key, char * value, int overwrite );

/* Methods */

/** \brief Delete a value in the cell
 *
 * 	\param cell Cell where to store the data
 *  \param key Unique key to be deleted
 *
 * 	 \return 1 on error 0 otherwise
 *
 */
int MPC_Info_cell_delete( struct MPC_Info_cell * cell , char * key );



/*******************
* MPC_Info_factory *
*******************/

/**
 * \brief Matches an integer ID with an actual \ref MPC_Info_cell
 *
 *  This structure is in charge of matching the
 *  opaque int (aka MPI_Info or MPC_Info) identifier
 *  usign this trick we do not have to do any conversion when
 *  going back and forth the fortran interface
 *
 *  This factory is placed in the \ref mpc_mpi_cl_per_mpi_process_ctx_s structure
 *
 * */
struct MPC_Info_factory
{
	int current_id; /**< Id which is incremented each time a new MPI_Info object is created */
	struct MPC_Info_cell * infos; /**< This is a UTHash hash table used to match IDs with \ref MPC_Info_cell */
};

/* Init and Release */
/** \brief Initialises the MPC_Info factory
 *  \param fact A pointer to the factory
 */
void MPC_Info_factory_init( struct MPC_Info_factory * fact );

/** \brief releases the MPC_Info factory
 *  \param fact A pointer to the factory
 *
 *  \warning After this call every MPI_Info are released.
 *
 */
void MPC_Info_factory_release( struct MPC_Info_factory * fact );


/* Methods */

/** \brief Creates a new MPC_Info_cell associated with an unique ID
 *
 *  \param fact A pointer to the factory
 *
 *  \return The id of the newly created factory
 *
 */
int MPC_Info_factory_create( struct MPC_Info_factory * fact );

/** \brief Deletes and MPI_Info from the factory
 *
 *  \param fact A pointer to the factory
 *  \param id The unique ID to delete
 *
 *  \return 0 on success ,  1 on error
 */
int MPC_Info_factory_delete(  struct MPC_Info_factory * fact, int id );

/** \brief Translates an MPI_Info/MPC_Info id to a pointer to an \ref MPC_Info_cell
 *
 * 	\param fact A pointer to the factory
 *  \param id The Unique Id to lookup
 *
 * 	\return A pointer to the MPC_Info_cell matching the ID , NULL if not found
 */
struct MPC_Info_cell * MPC_Info_factory_resolve(   struct MPC_Info_factory * fact , int id );



#endif /* MPC_INFO_H */
