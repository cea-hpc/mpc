/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
#ifndef MPC_COMMON_INCLUDE_MPC_COMMON_DATASTRUCTURE_H_
#define MPC_COMMON_INCLUDE_MPC_COMMON_DATASTRUCTURE_H_

#include <mpc_common_types.h>
#include <mpc_common_spinlock.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************
 * BIT LEVEL ARRAY *
 *******************/

/**
 * @addtogroup mpc_common_bit_array_
 * @{
 */

/**
 * @brief Masks used to get and set bits.
 *
 * It is binary anded/ored with the buffer to get/set the bit value :
 *  @code
 *  bit = buffer & mask;
 *  buffer = buffer | mask;
 *  @endcode
 * Example in 4 bits :
 * 0001
 * 0010
 * 0100
 * 1000
 */
#define BIT_LEVEL_ARRAY_GET_SET_MASK {1, 2, 4, 8, 16, 32, 64, 128}


/**
 * @brief Masks used to unset bits
 *
 *  It is binary anded to unset bit on the buffer :
 *  @code
 *  buffer = buffer & mask;
 *  @endcode
 * Example in 4 bits :
 * 1110
 * 1101
 * 1011
 * 0111
 */
#define BIT_LEVEL_ARRAY_UNSET_MASK {254, 253, 251, 247, 239, 223, 191, 127}

/**
 * @brief Defines a bit array
 */
struct mpc_common_bit_array
{
	uint8_t *array;		/**< @brief The data */
	uint64_t size;		/**< @brief The number of uint8_t array contains */
	uint64_t real_size; /**< @brief The number of bits the array contains */
};

/**
 * @brief Getter on the array size
 * @param ba the array
 * @return the number of bits ba contains
 */
static inline uint64_t mpc_common_bit_array_size( struct mpc_common_bit_array *ba )
{
	return ba->real_size;
}

/**
 * @brief mpc_common_bit_array initializer
 * @param ba the mpc_common_bit_array to initialize
 * @param size the number of bits wanted fot ba
 */
void mpc_common_bit_array_init( struct mpc_common_bit_array *ba, uint64_t size );

/**
 * @brief mpc_common_bit_array initializer with preallocated buffer
 * @param ba the mpc_common_bit_array to initialize
 * @param size the number of bits wanted fot ba
 * @param buff a preallocated buffer larger than size bits
 * @param buff_size the size of the preallocated buffer
 */
void mpc_common_bit_array_init_buff( struct mpc_common_bit_array *ba, uint64_t size,
							   void *buff, uint64_t buff_size );

/**
 * @brief mpc_common_bit_array deinitializer
 * @param ba the mpc_common_bit_array to deinitialize
 */
void mpc_common_bit_array_release( struct mpc_common_bit_array *ba );

/**
 * @brief Copies a mpc_common_bit_array into another
 * @param dest the destination mpc_common_bit_array
 * @param src the source mpc_common_bit_array
 *
 * @warning Both src and dest must be initialized at the same size before
 * copying.
 */
void mpc_common_bit_array_replicate( struct mpc_common_bit_array *dest,
							   struct mpc_common_bit_array *src );

/**
 * @brief Prints a mpc_common_bit_array on standard output
 * @param ba the mpc_common_bit_array to print
 */
void mpc_common_bit_array_dump( struct mpc_common_bit_array *ba );

/**
 * @brief Sets a bit in a mpc_common_bit_array
 * @param ba the mpc_common_bit_array where to set the bit
 * @param key the index of the bit to set
 * @param value the value to set
 */
static inline void mpc_common_bit_array_set( struct mpc_common_bit_array *ba,
									   uint64_t key, uint8_t value )
{
    static const uint8_t unset_bit_mask[] = BIT_LEVEL_ARRAY_UNSET_MASK;

    static const uint8_t get_set_bit_mask[] = BIT_LEVEL_ARRAY_GET_SET_MASK;

	if ( ba->real_size <= key )
	{
		// printf(" SET Out of bound (%ld)\n", key);
		return;
	}

	uint64_t extern_offset = key >> 3;
	uint8_t local_offset = (uint8_t)(key - extern_offset) * 8;

	uint8_t *target = &ba->array[extern_offset];

	if ( value )
		*target |= get_set_bit_mask[local_offset];
	else
		*target &= unset_bit_mask[local_offset];
}

/**
 * @brief Gets a bit in a mpc_common_bit_array
 * @param ba the mpc_common_bit_array where to get the bit
 * @param key the index of the bit to get
 * @return the value of the bit (3 if out of bound)
 */
static inline uint8_t mpc_common_bit_array_get( struct mpc_common_bit_array *ba,
										  uint64_t key )
{
    static const uint8_t get_set_bit_mask[] = BIT_LEVEL_ARRAY_GET_SET_MASK;

	if ( ba->real_size <= key )
	{
		// printf(" GET Out of bound (%ld) \n", key);
		return 3;
	}

	uint64_t extern_offset = key >> 3;
	uint8_t local_offset = (uint8_t)(key - extern_offset) * 8;

	return ( ba->array[extern_offset] & get_set_bit_mask[local_offset] ) >>
		   local_offset;
}

/**
 * @}
 */

/*****************
 * BUFFERED FIFO *
 *****************/

/**
 * @addtogroup mpc_common_fifo_
 * @{
 */

struct mpc_common_fifo_chunk;

/**
 * @brief This is a struct defining a FIFO
 * It is composed of several mpc_common_fifo_chunk
 */
struct mpc_common_fifo
{
	mpc_common_spinlock_t head_lock;  /**< @brief a lock for concurent access to the head */
	struct mpc_common_fifo_chunk *head; /**< @brief the head chunk (chere elements are pushed */
	mpc_common_spinlock_t tail_lock;  /**< @brief a lock for concurent access to the tail */
	struct mpc_common_fifo_chunk *tail; /**< @brief the tail chunk (from where elements are popped */

	uint64_t chunk_size; /**< @brief the size of the composing chunks */
	size_t elem_size;	/**< @brief the size of each stored element */

	mpc_common_spinlock_t lock; /**< @brief a lock for concurrent access to element_count */
	uint64_t elem_count;		/**< @brief the number of elements currently stored in the FIFO */
};

/**
 * @brief Initializes a mpc_common_fifo
 * @param fifo the FIFO to initialize
 * @param chunk_size the wanted size for the chunks
 * @param elem_size the size of stored elements
 */
void mpc_common_fifo_init( struct mpc_common_fifo *fifo, uint64_t chunk_size, size_t elem_size );

/**
 * @brief releases a FIFO
 * @param fifo the FIFO to release
 * @param free_func
 * @warning free_func parameter is unused.
 * Release of FIFO consists of setting everything to 0 in it.
 */
void mpc_common_fifo_release( struct mpc_common_fifo *fifo );

/**
 * @brief Pushes an element into a FIFO
 * @param fifo the FIFO where to push the element
 * @param elem the element to push
 * @return the internally copied element
 */
void *mpc_common_fifo_push( struct mpc_common_fifo *fifo, void *elem );

/**
 * @brief Pops an element from a FIFO
 * @param fifo the FIFO from where to pop the element
 * @param dest where to copy the popped element
 * @return the popped element, NULL if the FIFO is empty
 */
void *mpc_common_fifo_pop( struct mpc_common_fifo *fifo, void *dest );

/**
 * @brief Thread-safely gets the number of elements stored in the FIFO
 * @param fifo the FIFO where to count elements
 * @return the number of elements stored in the FIFO
 */
uint64_t mpc_common_fifo_count( struct mpc_common_fifo *fifo );

/**
 * @brief Apply a function to each element of the FIFO
 * @param fifo the FIFO where to process elements
 * @param func the function to apply to all elements
 * @param arg arbitrary pointer passed to function calls
 * @return number of elements processed
 */
int mpc_common_fifo_process( struct mpc_common_fifo *fifo, int ( *func )( void *elem, void *arg ), void *arg );

/**
 * @}
 */

/********************
 * MPC'S HASH TABLE *
 ********************/

/**
 * @addtogroup _mpc_ht_cell_
 * @{
 */

/**
 * @brief This defines an HT cell to be used internally
 *
 */
struct _mpc_ht_cell
{
	char use_flag;              /**< If the cell is in use */
	uint64_t key;               /**< The key for this cell */
	void * data;                /**< Data inside the cell */
	struct _mpc_ht_cell * next;   /**< Pointer to next cell */
};

/**
 * @}
 */

/**
 * @addtogroup mpc_common_hashtable_
 * @{
 */

/**
 * @brief The definition of an MPC hash_table
 *
 */
struct mpc_common_hashtable
{
	struct _mpc_ht_cell *cells;       /**< Array of entries to be indexed */
	mpc_common_rwlock_t *rwlocks;    /**< Array of rwlocks for each cell */
	uint64_t table_size;            /**< Stores the Table size */
};

/**
 * @brief Initialize an MPC hash table
 *
 * @param ht table to be initialized
 * @param size number of cells in the HT
 */
void mpc_common_hashtable_init( struct mpc_common_hashtable *ht, uint64_t size );

/**
 * @brief Release a hash table
 *
 * @param ht table to be released
 */
void mpc_common_hashtable_release( struct mpc_common_hashtable *ht );

/**
 * @brief Get an element in the hash-table
 *
 * @param ht table to be queried
 * @param key key to look for
 * @return void* queried element or NULL if not found
 */
void *mpc_common_hashtable_get( struct mpc_common_hashtable *ht, uint64_t key );

/**
 * @brief Get an element in the HT or create it
 *
 * @param ht hash-table to look or create into
 * @param key the key to look of or assign to
 * @param create_entry a function pointer to a func returning the newly created elem (if needed)
 * @param did_create a flag defining if a new object was created
 * @return void* a pointer to the existing or new elemement
 */
void *mpc_common_hashtable_get_or_create( struct mpc_common_hashtable *ht, uint64_t key, void *( create_entry )( uint64_t key ), int *did_create );

/**
 * @brief Set a value at key (can replace)
 *
 * @param ht hash-table to insert a value into
 * @param key the key to use
 * @param data the pointer to be stored
 */
void mpc_common_hashtable_set( struct mpc_common_hashtable *ht, uint64_t key, void *data );

/**
 * @brief Delete a key in the HT
 *
 * @param ht the hash-table to delete from
 * @param key the key to be deleted
 */
void mpc_common_hashtable_delete( struct mpc_common_hashtable *ht, uint64_t key );

/**
 * @brief Check if the hash-table is empty
 *
 * @param ht hash-table to be checked
 * @return int true if empty
 */
int mpc_common_hashtable_empty( struct mpc_common_hashtable *ht );

/* Fine grained locking */

/**
 * @brief Lock a cell for reading
 *
 * @param ht the hash-table
 * @param key the key to be locked (locks the cell)
 */
void mpc_common_hashtable_lock_cell_read( struct mpc_common_hashtable *ht, uint64_t key );

/**
 * @brief Unlock a cell for reading
 *
 * @param ht the hash-table
 * @param key the key to be unlocked (unlocks the cell)
 */
void mpc_common_hashtable_unlock_cell_read( struct mpc_common_hashtable *ht, uint64_t key );

/**
 * @brief Lock a cell for writing
 *
 * @param ht the hash-table
 * @param key the key to be locked (locks the cell)
 */
void mpc_common_hashtable_lock_cell_write( struct mpc_common_hashtable *ht, uint64_t key );

/**
 * @brief Unlock a cell for writing
 *
 * @param ht the hash-table
 * @param key the key to be unlocked (unlocks the cell)
 */
void mpc_common_hashtable_unlock_cell_write( struct mpc_common_hashtable *ht, uint64_t key );

/**
 * @brief Start a Hash-table walk filling the var void* pointer for each elem
 * @warning This construct must be combined with a \ref MPC_HT_ITER_END construct
 *
 */
#define MPC_HT_ITER( ht, var )                             \
	unsigned int ____i;                                    \
	for ( ____i = 0; ____i < ( ht )->table_size; ____i++ ) \
	{                                                      \
		struct _mpc_ht_cell *____c = &( ht )->cells[____i];  \
		if ( ____c->use_flag == 0 )                        \
			continue;                                      \
                                                           \
		while ( ____c )                                    \
		{                                                  \
			var = ____c->data;

/** End an HT walk block */
#define MPC_HT_ITER_END  \
	____c = ____c->next; \
	}                    \
	}
/**
 * @}
 */


/*******************
 * BASE64 ENCODING *
 *******************/

/**
 * base64_encode - Base64 encode
 * @src: Data to be encoded
 * @len: Length of the data to be encoded
 * @out_len: Pointer to output length variable, or %NULL if not used
 * Returns: Allocated buffer of out_len bytes of encoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer. Returned buffer is
 * nul terminated to make it easier to use as a C string. The nul terminator is
 * not included in out_len.
 */
unsigned char * mpc_common_datastructure_base64_encode(const unsigned char *src, size_t len,
			      size_t *out_len);

/**
 * base64_decode - Base64 decode
 * @src: Data to be decoded
 * @len: Length of the data to be decoded
 * @out_len: Pointer to output length variable
 * Returns: Allocated buffer of out_len bytes of decoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer.
 */
unsigned char * mpc_common_datastructure_base64_decode(const unsigned char *src, size_t len,
			      size_t *out_len);


#ifdef __cplusplus
}
#endif

#endif /* MPC_COMMON_INCLUDE_MPC_COMMON_DATASTRUCTURE_H_ */