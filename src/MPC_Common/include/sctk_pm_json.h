/* ############################ MALP License ############################## */
/* # Fri Jan 18 14:00:00 CET 2013                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
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
/* #   - BESNARD Jean-Baptiste jean-baptiste.besnard@cea.fr               # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef JSON_STRUCT_H
#define JSON_STRUCT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <mpc_common_types.h>
#include <pthread.h>


/*
	TYPES
*/

typedef enum json_type_e
{
	JSON_NULL,
	JSON_BOOL,
	JSON_STRING,
	JSON_INT,
	JSON_REAL,
	JSON_OBJECT,
	JSON_ARRAY,
	JSON_SOMETHING_ELSE
} json_type;

/*
	MAIN Json type definition
*/

typedef struct json_t_s
{
	json_type type;
	size_t refcounter;

	pthread_spinlock_t lock;
	volatile char locked;
}json_t;

static inline void json_lock( json_t *json )
{
	if( !json )
		return;

	pthread_spin_lock( &json->lock );
	json->locked = 1;
}

static inline void json_unlock( json_t *json )
{
	if( !json )
		return;

	pthread_spin_unlock( &json->lock );
	json->locked = 0;
}

static inline int json_locked( json_t *json )
{
	return json->locked;
}

void json_incref( json_t *json );
void json_decref( json_t *json );

/* INTERNALS ------------------------ */
json_t * __json_t_init( json_type type );
void __json_t_release( json_t *json );
/* ---------------------------------- */

static inline void json_incref_lf( json_t *json )
{
	if( !json )
		return;

	json->refcounter++;
}


static inline int json_decref_lf( json_t *json )
{
	if( !json )
		return 0;

	json->refcounter--;

	if( json->refcounter == 0 )
	{
		__json_t_release( json );
		return 0;
	}

	return 1;
}



/*
	BASIC Types
*/

typedef struct json_null_s
{
	json_t ___;
} json_null_t;

json_t * json_null();
void json_null_destroy(json_t * json);

typedef struct json_bool_s
{
	json_t ___;
	char value;
} json_bool_t;

json_t * json_bool( int thruth );

/* INTERNALS ------------------------ */
void json_bool_destroy(json_t * json);
/* ---------------------------------- */

/*
	C Types
*/

typedef struct json_string_s
{
	json_t ___;
	char *value;
} json_string_t;

json_t * json_string( const char * string );
json_t * json_string_l( const char *string , int len );

/* INTERNALS ------------------------ */
void json_string_destroy(json_t * json);
/* ---------------------------------- */

typedef struct json_int_s
{
	json_t ___;
	int64_t value;
} json_int_t;

json_t * json_int( int64_t value );

/* INTERNALS ------------------------ */
void json_int_destroy(json_t * json);
/* ---------------------------------- */

typedef struct json_real_s
{
	json_t ___;
	double value;
} json_real_t;

json_t * json_real( double value );

/* INTERNALS ------------------------ */
void json_real_destroy(json_t * json);
/* ---------------------------------- */

/*
	CONTAINERS Types
*/

struct ObjectHT_entry
{
	char *key;
	json_t * elem;

	struct ObjectHT_entry * prev;
};

struct ObjectHT
{
	struct ObjectHT_entry ** entries;
	size_t size;
	int pow2_size;
};


void ObjectHT_init( struct ObjectHT *ht, int pow2_size );
void ObjectHT_release( struct ObjectHT *ht );

void ObjectHT_set( struct ObjectHT *ht, const char *key, json_t * elem );
struct ObjectHT_entry * ObjectHT_get( struct ObjectHT *ht, const char *key );
void ObjectHT_delete( struct ObjectHT *ht, const char *key );


typedef struct json_object_s
{
	json_t ___;
	struct ObjectHT ht;
} json_object_t;

json_t * json_object();

json_t * json_object_set( json_t * json, const char *key, json_t * elem );
json_t * json_object_get( json_t * json, const char *key );
json_t * json_object_delete( json_t * json, const char *key );


/* INTERNALS ------------------------ */
void json_object_destroy(json_t * json);
/* ---------------------------------- */

typedef struct json_array_s
{
	json_t ___;

	json_t **content;
	size_t size;
} json_array_t;

json_t * json_array();

json_t * json_array_push(json_t *json, json_t * elem);
json_t * json_array_push_at(json_t *json, unsigned int offset, json_t * elem);
json_t * json_array_get( json_t *json, unsigned int offset );
json_t * json_array_set( json_t *json, unsigned int offset , json_t * elem);
json_t * json_array_del( json_t *json, unsigned int offset );

/* INTERNALS ------------------------ */
void json_array_destroy(json_t * json);
int json_array_guarantee( json_array_t  *a, unsigned int offset, int count );
/* ---------------------------------- */


/*
	json_t MUTATORS
*/

#define json_to_null( a ) ( (json_null_t *)( a ) )
#define json_to_bool( a ) ( (json_bool_t *)( a ) )
#define json_to_string( a ) ( (json_string_t *)( a ) )
#define json_to_int( a ) ( (json_int_t *)( a ) )
#define json_to_real( a ) ( (json_real_t *)( a ) )
#define json_to_object( a ) ( (json_object_t *)( a ) )
#define json_to_array( a ) ( (json_array_t *)( a ) )

/*
	OBJECT ITERATOR
*/


typedef struct json_object_iterator_s
{
	struct ObjectHT *ht;
	size_t current_bucket;
	struct ObjectHT_entry *current_elem;
}json_object_iterator;

static inline void json_object_iterator_init( json_object_iterator *it, json_t * json )
{
	if( json->type != JSON_OBJECT )
	{
		it->ht = NULL;
		it->current_bucket = -1;
		it->current_elem = NULL;
		return;
	}

	json_object_t *o = json_to_object( json );

	it->ht = &o->ht;
	it->current_bucket = 0;
	it->current_elem = NULL;


}

static inline int json_object_iterator_next( json_object_iterator *it )
{
	if( !it->ht )
		return 0;

	if( !it->current_elem )
	{
		it->current_elem = it->ht->entries[ it->current_bucket ];
	}
	else
	{
		it->current_elem = it->current_elem->prev;
	}

	if( !it->current_elem )
	{
		it->current_bucket++;

		if( it->ht->size <= it->current_bucket )
		{
			return 0;
		}
		else
		{
			return json_object_iterator_next( it );
		}

	}
	else
	{
		return 1;
	}

}

static inline char * json_object_iterator_key( json_object_iterator *it )
{

	if( it->current_elem )
	{
		return it->current_elem->key;
	}

	return NULL;
}

static inline json_t * json_object_iterator_elem( json_object_iterator *it )
{
	if( it->current_elem )
	{
		return it->current_elem->elem;
	}

	return NULL;
}


/*
	Dumping
*/

typedef enum
{
	JSON_COMPACT = 1,
	JSON_PRETTY = 2,
	JSON_NO_LOCK = 4
}json_format;


char * json_dump( json_t * json, json_format mode );
void json_dump_f(FILE * f, json_t * json, json_format mode );

static inline void json_print( json_t * json, json_format mode )
{
	json_dump_f( stdout , json, mode );
}

/*
  PARSING
*/

json_t * _json_parse( char ** buff );
json_t * json_parse( char * json );

#ifdef __cplusplus
}
#endif

#endif /* JSON_STRUCT_H */
