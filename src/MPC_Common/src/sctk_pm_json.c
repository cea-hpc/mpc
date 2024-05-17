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

#include <string.h>
#include <errno.h>
#include <limits.h>

#include "sctk_alloc.h"
#include "sctk_pm_json.h"
#include "mpc_common_debug.h"

/*
   MAIN Json type definition
   */



void json_incref( json_t *json )
{
	json_lock( json );

	json_incref_lf( json );

	json_unlock( json );
}


void json_decref( json_t *json )
{
	json_lock( json );

	if( json_decref_lf( json ) )
		json_unlock( json );
}

json_t * __json_t_init( json_type type )
{
	/*
	   COMPUTE SIZE
	   */
	size_t extra_type_size = 0;

	switch( type )
	{
		case JSON_NULL:
			extra_type_size = sizeof( json_null_t );
			break;
		case JSON_BOOL:
			extra_type_size = sizeof( json_bool_t );
			break;
		case JSON_STRING:
			extra_type_size = sizeof( json_string_t );
			break;
		case JSON_INT:
			extra_type_size = sizeof( json_int_t );
			break;
		case JSON_REAL:
			extra_type_size = sizeof( json_real_t );
			break;
		case JSON_OBJECT:
			extra_type_size = sizeof( json_object_t );
			break;
		case JSON_ARRAY:
			extra_type_size = sizeof( json_array_t );
			break;
		case JSON_SOMETHING_ELSE:
			printf("Warning : Unhandled JSON type...\n");
			break;
	}

	if( extra_type_size == 0 )
	{
		/* We put the default HERE */
		return NULL;
	}

	/*
	   ALLOCATE
	   */

	json_t * json = (json_t *)sctk_malloc( sizeof( json_t ) + extra_type_size );

	if (!json )
	{
		perror( "malloc" );
		return NULL;
	}

	/*
	   INIT CONTAINER
	   */

	json->type = type;
	json->refcounter = 1;

	pthread_spin_init( &json->lock, 0 );

	return json;
}


void __json_t_release( json_t *json )
{
	if( !json )
		return;

	/*
	   CLEAN OBJECT
	   */

	switch( json->type )
	{
		case JSON_NULL:
			json_null_destroy(json);
			break;
		case JSON_BOOL:
			json_bool_destroy(json);
			break;
		case JSON_STRING:
			json_string_destroy(json);
			break;
		case JSON_INT:
			json_int_destroy(json);
			break;
		case JSON_REAL:
			json_real_destroy(json);
			break;
		case JSON_OBJECT:
			json_object_destroy(json);
			break;
		case JSON_ARRAY:
			json_array_destroy(json);
			break;
		case JSON_SOMETHING_ELSE:
			printf("Warning : Unhandled JSON type...\n");
			break;
	}


	pthread_spin_destroy( &json->lock );

	free( json );
}


/*
   BASIC Types
   */

json_t * json_null()
{
	json_t * ret = __json_t_init( JSON_NULL );
	return ret;
}

void json_null_destroy(__UNUSED__ json_t * json)
{

}

json_t * json_bool( int thruth )
{
	json_t * ret = __json_t_init( JSON_BOOL );

	/* INIT PACKED */
	json_bool_t * b = json_to_bool( ret );
	b->value = thruth;

	return ret;
}

void json_bool_destroy(json_t * json)
{
	json_bool_t * b = json_to_bool( json );
	b->value = -1;
}

json_t * json_string( const char * string )
{
	json_t * ret = __json_t_init( JSON_STRING );

	/* INIT PACKED */
	json_string_t * s = json_to_string( ret );
	s->value = strdup( string );

	return ret;
}

json_t * json_string_l( const char *string , int len )
{
	json_t * ret = __json_t_init( JSON_STRING );

	/* INIT PACKED */
	json_string_t * s = json_to_string( ret );

	s->value = (char *)sctk_malloc( len + 1 );

	if( ! s->value )
	{
		perror( "malloc" );
		return NULL;
	}

	memcpy( s->value , string, len );
	s->value[ len ] = '\0';

	return ret;
}

void json_string_destroy(json_t * json)
{
	json_string_t * s = json_to_string( json );
	free( s->value );
	s->value = NULL;
}

json_t * json_int( int64_t value )
{
	json_t * ret = __json_t_init( JSON_INT );

	/* INIT PACKED */
	json_int_t * i = json_to_int( ret );
	i->value = value;

	return ret;
}

void json_int_destroy(json_t * json)
{
	json_int_t * i = json_to_int( json );
	i->value = 0;
}

json_t * json_real( double value )
{
	json_t * ret = __json_t_init( JSON_REAL );

	/* INIT PACKED */
	json_real_t * r = json_to_real( ret );
	r->value = value;

	return ret;
}

void json_real_destroy(json_t * json)
{
	json_real_t * r = json_to_real( json );
	r->value = 0.0;
}

/*
   CONTAINERS Types
   */

struct ObjectHT_entry * ObjectHT_entry_new( const char *key, json_t * elem )
{
	if( !strlen( key ) || !elem )
		return NULL;

	struct ObjectHT_entry * ret = (struct ObjectHT_entry *)sctk_malloc( sizeof( struct ObjectHT_entry ) );

	if( !ret )
	{
		perror("malloc");
		return NULL;
	}


	ret->key = strdup( key );

	json_incref_lf( elem );

	ret->elem = elem;
	ret->prev = NULL;

	return ret;
}

void ObjectHT_entry_release( struct ObjectHT_entry * ent )
{
	if( !ent )
		return;

	ObjectHT_entry_release( ent->prev );

	free( ent->key );
	ent->key = NULL;

	json_decref( ent->elem );
	ent->elem = NULL;

	free( ent );
}




struct ObjectHT_entry * ObjectHT_entry_push( struct ObjectHT_entry * prev, const char *key, json_t * elem )
{

	struct ObjectHT_entry *ret = ObjectHT_entry_new( key, elem );

	if( !ret )
		return NULL;

	if( ret )
	{
		ret->prev = prev;
	}

	return ret;
}


void ObjectHT_init( struct ObjectHT *ht, int pow2_size )
{
	int size = 0;

	int i;
	for( i = 0 ; i < pow2_size ; i++ )
		size |= (0x1 << i );

	ht->pow2_size = pow2_size;

	ht->entries = (struct ObjectHT_entry **)calloc( sizeof( struct ObjectHT_entry *) , size );

	if( ! ht->entries )
	{
		perror("malloc");
		abort();
	}

	ht->size = size;
}


void ObjectHT_release( struct ObjectHT *ht )
{
	unsigned int i;

	for( i = 0 ; i < ht->size ; i++ )
	{
		ObjectHT_entry_release( ht->entries[ i ] );
	}


	free( ht->entries );
	ht->entries = NULL;
	ht->size = 0;

}

static inline unsigned int hashString( const char *s , int bit_width )
{
	unsigned int ret = 0;
	int i = 1;

	while( *s )
	{
		ret += *s;
		ret ^= (*s << (((i++)%7) * 4));
		s++;
	}

	int dec = 32 - bit_width;

	ret += ret >> dec;
	ret = (ret << dec) >> dec;

	return ret?ret-1:ret;
}

static inline int same_string( const char *a, const char *b )
{
	do
	{
		if( *a != *b )
			return 0;

		a++;
		b++;
	}while( *a && *b );

	if( *a == *b )
		return 1;
	else
		return 0;

}


struct ObjectHT_entry * ObjectHT_get( struct ObjectHT *ht, const char *key )
{
	if( !key || !ht )
		return NULL;

	unsigned int bucket = hashString( key , ht->pow2_size );

	struct ObjectHT_entry * ent = ht->entries[bucket];

	while( ent )
	{

		if( same_string( ent->key, key ) )
			return ent;

		ent = ent->prev;
	}

	return NULL;
}


void ObjectHT_set( struct ObjectHT *ht, const char *key, json_t * elem )
{
	if( !elem || !ht || !key )
		return;

	struct ObjectHT_entry *ent = ObjectHT_get( ht, key );

	if( ent )
	{
		json_decref( ent->elem );
		json_incref_lf( elem );
		ent->elem = elem;
	}
	else
	{
		unsigned int bucket = hashString( key , ht->pow2_size );
		ht->entries[bucket] = ObjectHT_entry_push( ht->entries[bucket], key, elem );
	}
}

struct ObjectHT_entry * __ObjectHT_delete( struct ObjectHT_entry *ent, const char *key, int did_delete )
{
	if( !ent )
		return NULL;

	if( did_delete )
		return ent;

	/* We could speedup this using a DL list... */
	if( same_string( ent->key, key ) )
	{
		struct ObjectHT_entry * ret = __ObjectHT_delete( ent->prev, key, 1 );

		json_decref( ent->elem );
		free( ent->key );
		free( ent );

		return ret;
	}

	struct ObjectHT_entry * ret =  __ObjectHT_delete( ent->prev, key, 0 );

	if( ret != ent->prev )
	{
		ent->prev = ret;
	}

	return ent;
}



void ObjectHT_delete( struct ObjectHT *ht, const char *key )
{
	unsigned int bucket = hashString( key , ht->pow2_size );
	ht->entries[ bucket ] = __ObjectHT_delete( ht->entries[ bucket ], key, 0 );
}


json_t * json_object()
{
	json_t * ret = __json_t_init( JSON_OBJECT );

	/* INIT PACKED */
	json_object_t * o = json_to_object( ret );

	ObjectHT_init( &o->ht, 13 );

	return ret;
}

void json_object_destroy(json_t * json)
{
	json_object_t * o = json_to_object( json );

	ObjectHT_release( &o->ht );
}

json_t * json_object_set( json_t * json, const char *key, json_t * elem )
{
	json_object_t * o = json_to_object( json );

	json_lock( json );

	ObjectHT_set( &o->ht, key, elem );

	json_unlock( json );

	return elem;
}

json_t * json_object_get( json_t * json, const char *key )
{
	json_object_t * o = json_to_object( json );

	json_t *ret = NULL;

	json_lock( json );

	struct ObjectHT_entry *ent = ObjectHT_get( &o->ht, key );

	ret = ent?ent->elem:NULL;

	if( ret )
		json_incref( ret );

	json_unlock( json );

	return ret;
}

json_t * json_object_delete( json_t * json, const char *key )
{
	json_object_t * o = json_to_object( json );

	json_lock( json );

	ObjectHT_delete( &o->ht, key );

	json_unlock( json );

	return json;
}



/*
   ARRAY
   */


json_t * json_array()
{
	json_t * ret = __json_t_init( JSON_ARRAY );

	/* INIT PACKED */
	json_array_t * a = json_to_array( ret );

	a->content = NULL;
	a->size = 0;

	return ret;
}

int json_array_guarantee( json_array_t  *a, unsigned int offset, int count )
{
	if( count == 0 )
		return 0;

	size_t overflow = 0;

	if( a->size < offset )
		overflow = offset - a->size;

	a->content = (json_t**)realloc( a->content, (a->size + count + overflow) * sizeof( json_t * ) );

	if( !a->content )
	{
		perror("realloc");
		return 1;
	}

	if( overflow )
	{
		/* PAD the array with NULL values */
		unsigned int i;
		for( i = a->size; i < offset; i++ )
		{
			a->content[i] = json_null();
		}
	}


	if( offset < a->size )
	{
		memmove( a->content + offset + count , a->content + offset, (a->size - offset) * sizeof( json_t * ) );
	}

	a->size += count + overflow;

	return 0;
}

json_t * json_array_push_at(json_t *json, unsigned int offset, json_t * elem)
{
	json_incref_lf( elem );

	json_lock( json );

	json_array_t *array = json_to_array( json );

	if( json_array_guarantee( array, offset, 1 ) )
		return NULL;

	array->content[ offset ] = elem;

	json_unlock( json );

	return elem;
}

json_t * json_array_push(json_t *json, json_t * elem)
{
	json_array_t *array = json_to_array( json );
	return  json_array_push_at(json, array->size, elem);
}



json_t * json_array_get( json_t *json, unsigned int offset )
{
	json_t * ret = NULL;

	json_array_t *array = json_to_array( json );

	if( array->size <= offset )
		return ret;

	json_lock( json );
	ret = array->content[ offset ];
	json_incref( ret );
	json_unlock( json );

	return ret;
}

json_t * json_array_set( json_t *json, unsigned int offset , json_t * elem)
{
	json_array_t *array = json_to_array( json );

	if( array->size <= offset )
		return NULL;

	json_incref_lf( elem );

	json_lock( json );

	json_decref( array->content[ offset ] );
	array->content[ offset ] = elem;

	json_unlock( json );

	return elem;
}


json_t * json_array_del( json_t *json, unsigned int offset )
{
	json_array_t *array = json_to_array( json );

	if( array->size <= offset )
		return NULL;

	json_lock( json );

	json_decref( array->content[offset] );
	array->content[offset] = NULL;

	if( array->size == 1 )
	{
		free( array->content );
		array->content = NULL;
		array->size = 0;

		json_unlock( json );
		return json;
	}
	else
	{
		if( offset < (array->size - 1) )
		{
			memmove( &array->content[offset], &array->content[offset + 1], (array->size - offset ) * sizeof( json_t * ) );
		}

		array->content = (json_t**)realloc( array->content, (array->size - 1) * sizeof( json_t * ) );

		if( !array->content )
		{
			perror("realloc");
			abort();
		}
	}

	array->size--;

	json_unlock( json );

	return json;
}


void json_array_destroy(json_t * json)
{
	json_array_t * array = json_to_array( json );

	unsigned int i;

	for( i = 0 ; i < array->size ; i++ )
		json_decref( array->content[i] );

	free( array->content );
	array->content = NULL;

	array->size = 0;

}

/*
   Dumping
   */

struct string_buff
{
	char *s;
	size_t len;
	size_t buff_len;
};

void string_buff_init( struct string_buff * sb )
{
	sb->s = NULL;
	sb->len = 0;
	sb->buff_len = 1024*1024;
}

void string_buff_release( struct string_buff * sb )
{
	free( sb->s );
	string_buff_init( sb );
}


void string_buff_push( struct string_buff * sb, char * s )
{
	if( !s || !sb)
		return;

	int len = strlen( s );

	if( !len )
		return;

	while( sb->buff_len <= sb->len + len + 1 )
	{
		sb->buff_len *= 2;
	}

	sb->s = (char *)realloc( sb->s, sb->buff_len );

	if( !sb->s )
	{
		perror("realloc");
		abort();
	}

	memcpy( sb->s + sb->len , s, len + 1 );

	sb->len += len;
}


char * c_newline( int newline )
{
	if( newline )
		return "\n";
	else
		return "";
}

void stream_indent( char *tmp_buff, struct string_buff *out, int depth )
{
	int i;
	tmp_buff[0] = '\0';

	for( i = 0 ; i < depth && i < 4095 ; i++)
		tmp_buff[i] = '\t';

	tmp_buff[depth] = '\0';
	string_buff_push( out, tmp_buff );
}


void __json_dump( char *tmp_buff, struct string_buff *out, json_t * json, int depth, int indent, int newline, int lock )
{
	if( !json )
		return;

	unsigned int i;
	json_object_iterator it;
	int is_first = 0;
	int cont = 0;
	json_array_t *array = NULL;

	if( indent )
	{
		stream_indent( tmp_buff, out, depth );
	}

	if( lock )
		json_lock( json );


	switch( json->type )
	{
		case JSON_NULL:
			string_buff_push( out, " null " );
			break;
		case JSON_BOOL:

			snprintf( tmp_buff, 4096, " %s ", json_to_bool( json )->value?"true":"false" );
			string_buff_push( out, tmp_buff );
			break;
		case JSON_STRING:
			string_buff_push( out, " \"" );
			snprintf( tmp_buff, 4096, "%s", json_to_string( json )->value );
			string_buff_push( out, tmp_buff );
			string_buff_push( out, "\" " );
			break;
		case JSON_INT:
			snprintf( tmp_buff, 4096, " %ld ", json_to_int( json )->value );
			string_buff_push( out, tmp_buff );
			break;
		case JSON_REAL:
			snprintf( tmp_buff, 4096, " %g ", json_to_real( json )->value );
			string_buff_push( out, tmp_buff );
			break;
		case JSON_OBJECT:
			/* OPEN OBJECT */
			snprintf( tmp_buff, 4096, "{%s", c_newline( newline ) );
			string_buff_push( out, tmp_buff );

			/* DUMP OBJECT */

			/*
			 *  This strange do while loop is made in order to put the comas
			 * at the right places as we do not have a counter but
			 * only an iterator.
			 */



			json_object_iterator_init( &it, json );



			do
			{
				if( is_first != 0 )
				{
					/* WRITE KEY */
					snprintf( tmp_buff, 4096, " \"%s\" : %s", json_object_iterator_key( &it ),  c_newline( newline )  );
					string_buff_push( out, tmp_buff );
					/* WRITE ENTITY */
					__json_dump( tmp_buff, out, json_object_iterator_elem( &it ), depth + 1 , indent,  newline, lock );
					is_first++;
				}
				else
				{
					is_first++;
				}

				cont = json_object_iterator_next( &it );

				/* IF AN ENTRY FOLLOWS */
				if( cont /* NO COMA FOR THE LAST */
						&& is_first != 1 /* NO COMA FOR THE FIRST DUMMY*/)
				{
					snprintf( tmp_buff, 4096, ",%s", c_newline( newline ) );
					string_buff_push( out, tmp_buff );
				}

			}while( cont );


			/* SUGAR NEWLINE AND INDENT */
			snprintf( tmp_buff, 4096, "%s",  c_newline( newline ) );
			string_buff_push( out, tmp_buff );

			if( indent )
			{
				stream_indent( tmp_buff, out, depth );
			}

			/* CLOSE OBJECT */
			snprintf( tmp_buff, 4096, "}");
			string_buff_push( out, tmp_buff );

			break;
		case JSON_ARRAY:
			/* OPEN ARRAY */
			snprintf( tmp_buff, 4096, " [%s", c_newline( newline ) );
			string_buff_push( out, tmp_buff );

			/* DUMP ARRAY */
			array = json_to_array( json );

			for( i = 0 ; i < array->size; i++ )
			{
				/* WRITE ENTITY */
				__json_dump( tmp_buff, out, array->content[i], depth + 1 , indent,  newline, lock );

				/* CLOSE ENTRY */
				if( i != ( array->size - 1 ) )
				{
					snprintf( tmp_buff, 4096, ",%s", c_newline( newline ) );
					string_buff_push( out, tmp_buff );
				}
			}

			/* SUGAR NEWLINE AND INDENT */
			snprintf( tmp_buff, 4096, "%s",  c_newline( newline ) );
			string_buff_push( out, tmp_buff );

			if( indent )
			{
				stream_indent( tmp_buff, out, depth );
			}

			/* CLOSE ARRAY */
			snprintf( tmp_buff, 4096, "]");
			string_buff_push( out, tmp_buff );
			break;
		case JSON_SOMETHING_ELSE:
			printf("Warning : Unhandled JSON type...\n");
			break;
	}

	if( lock )
		json_unlock( json );
}


char * json_dump( json_t * json, json_format mode )
{
	char *tmp_buff = (char *)sctk_malloc( 4096 );

	if( !tmp_buff )
	{
		perror( "malloc" );
		return NULL;
	}

	struct string_buff out;
	string_buff_init( &out );

	int indent;
	int newline;

	if( mode & JSON_COMPACT )
	{
			indent = 0;
			newline = 0;
	}
	else
	{
			indent = 1;
			newline = 1;
	}

	int lock = 1;

	if( mode & JSON_NO_LOCK )
		lock = 0;


	__json_dump( tmp_buff, &out, json, 0, indent, newline, lock );


	free( tmp_buff );

	return out.s;
}


void json_dump_f(FILE * f, json_t * json, json_format mode )
{
	char * dump = json_dump( json, mode );

	fprintf( f, "%s" , dump );

	free( dump );
}


/*
   PARSING
   */

int is_alphanumeric( char * c )
{
	if( !c )
		return 0;


	if( 'a' <= *c && *c <= 'z' )
		return 1;

	if( 'A' <= *c && *c <= 'Z' )
		return 1;

	if( '0' <= *c && *c <= '9' )
		return 1;

	return 0;
}

int is_numeric( char * c )
{
	if( !c )
		return 0;

	if( '0' <= *c && *c <= '9' )
		return 1;

	return 0;
}


int pad_buff( char **buff, char expect )
{

	char *s = *buff;

	while( *s && *s != expect )
	{

		if( *s != ' ' && *s != '\n' && *s != '\t' )
		{
			*buff = s;
			return 1;
		}


		s++;
	}

	*buff = s;

	return 0;
}


char * parse_string(  char ** json_string )
{
	char *s = *json_string;

	/* First quote */
	if( pad_buff( &s, '"' ) )
		return NULL;

	s++;

	char *string_start = s;

	int skip_next_quote = 0;

	/* Second quote */
	while( (*s != '"' || skip_next_quote ) && *s )
	{

		if( *s == '\\' )
			skip_next_quote = 1;

		if( skip_next_quote )
			skip_next_quote = 0;

		s++;
	}

	if( *s != '"' )
		return NULL;

	*s = '\0';

	*json_string = s + 1;

	return string_start;
}


int is_true( char *s )
{
	if( *s == 't' || *s == 'T')
	{
		if( *(s + 1 ) == 'r' || *(s + 1 ) == 'R' )
		{
			if( *(s + 2 ) == 'u' || *(s + 2 ) == 'U' )
			{
				if( *(s + 3 ) == 'e' || *(s + 3 ) == 'E' )
				{
					return 1;
				}
			}
		}
	}

	return 0;
}

int is_false( char *s )
{

	if( *s == 'f' || *s == 'F')
	{
		if( *(s + 1 ) == 'a' || *(s + 1 ) == 'A' )
		{
			if( *(s + 2 ) == 'l' || *(s + 2 ) == 'L' )
			{
				if( *(s + 3 ) == 's' || *(s + 3 ) == 'S' )
				{
					if( *(s + 4 ) == 'e' || *(s + 4 ) == 'E' )
					{
						return 1;
					}
				}
			}
		}
	}

	return 0;
}

int is_null( char *s )
{

	if( *s == 'n' || *s == 'N')
	{
		if( *(s + 1 ) == 'u' || *(s + 1 ) == 'U' )
		{
			if( *(s + 2 ) == 'l' || *(s + 2 ) == 'L' )
			{
				if( *(s + 3 ) == 'l' || *(s + 3 ) == 'L' )
				{
					return 1;
				}
			}
		}
	}

	return 0;
}


int json_infer_type( char * ent )
{
	char *s = ent;

	while( *s )
	{
		if( *s == ',' || *s == '}' || *s == ']'  )
			return JSON_NULL;

		if( is_null( s ) )
			return JSON_NULL;

		if( *s == '"' )
			return JSON_STRING;

		if( *s == '{' )
			return JSON_OBJECT;

		if( *s == '[' )
			return JSON_ARRAY;

		if( is_numeric( s ) )
		{
			while( *s && *s != '}' && *s != ']' && *s != ',' )
			{
				if( *s == 'e' || *s == 'E' || *s == '.' )
					return JSON_REAL;

				s++;
			}

			return JSON_INT;
		}


		if( is_true( s ) || is_false( s ) )
			return JSON_BOOL;

		s++;
	}

	return JSON_SOMETHING_ELSE;
}


json_t * json_parse_int( char ** buff )
{
	char *s = *buff;

	pad_buff( &s, '\0' );

	char * begin = s;

	while( *s && *s != ',' && *s != ']' && *s != '}' )
	{
		s++;
	}

	char end_char = *s;
	*s = '\0';

	char *end = NULL;
	errno = 0;
	long int val = strtoll( begin , &end, 10);

	if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN))
			|| (errno != 0 && val == 0)) {
		perror("strtol");
		return NULL;
	}

	if( end == begin )
		return NULL;

	*s = end_char;

	return json_int( val );
}

json_t * json_parse_real( char ** buff )
{
	char *s = *buff;

	pad_buff( &s, '\0' );

	char * begin = s;

	while( *s && *s != ',' && *s != ']' && *s != '}' )
	{
		s++;
	}

	char end_char = *s;
	*s = '\0';

	char *end = NULL;
	errno = 0;
	long double val = strtold(begin, &end);

	if ((errno == ERANGE ) || (errno != 0 && val == 0.0)) {
		perror("strtold");
		return NULL;
	}

	if( end == begin )
		return NULL;


	*s = end_char;

	return json_real( val );
}


json_t * json_parse_string( char ** buff )
{
	char * string = parse_string( buff );

	if( !string )
		return NULL;

	return json_string( string );
}


json_t * json_parse_bool( char ** json_string )
{
	char *s = *json_string;

	pad_buff( &s, '\0' );

	if( is_true( s ) )
	{
		*json_string += 4;
		return json_bool( 1 );
	}
	else
	{
		if( is_false( s ) )
		{
			*json_string += 5;
			return json_bool( 0 );
		}
	}


	return json_null();
}


json_t * json_parse_array( char ** json_string )
{
	char *s = *json_string;

	if( pad_buff( &s, '[' ) )
	{
		return NULL;
	}

	if( *s != '[' )
	{
		return NULL;
	}


	s++;


	json_t *ret = json_array();

	while( *s && *s != ']' )
	{
		json_t *elem = _json_parse( &s );

		if( !elem )
		{
			goto json_parse_array_error;
		}

		json_lock( elem );

		json_array_push( ret, elem );

		json_decref_lf( elem );

		json_unlock( elem );

		while( *s && *s != ',' && *s != ']' )
			s++;

		if( *s == ']' )
			break;

		if( !(*s) )
			break;

		s++;
	}


	if( pad_buff( &s, ']' ) )
	{
		goto json_parse_array_error;
	}

	if( *s != ']' )
	{
		goto json_parse_array_error;
	}

	*json_string = s + 1;

	return ret;

json_parse_array_error:
	json_decref( ret );
	return NULL;
}



char * json_parse_key (  char ** json_string )
{
	char *key = parse_string( json_string );

	/* SKIP Key separator */

	char * s = *json_string;

	if( pad_buff( &s, ':' ) )
	{
		return NULL;
	}

	if( *s != ':' )
	{
		return NULL;
	}

	*json_string = s + 1;

	return key;
}



json_t * json_parse_object( char ** json_string )
{
	char *s = *json_string;

	if( pad_buff( &s, '{' ) )
	{
		return NULL;
	}

	if( *s != '{' )
		return NULL;

	s++;

	json_t *ret = json_object();

	while( *s != '}' && *s)
	{
		//printf("KEY : " );

		char * key = json_parse_key( &s );

		if( !key )
		{
			goto json_parse_object_error;
		}

		json_t *elem = _json_parse( &s );

		if( !elem )
		{
			//printf("NO ELEM\n");
			goto json_parse_object_error;
		}

		json_lock( elem );

		json_object_set( ret, key, elem );

		json_decref_lf( elem );

		json_unlock( elem );



		while( *s && *s != ',' && *s != '}' )
			s++;

		if( *s == '}' )
			break;

		if( !(*s) )
			break;

		s++;
	}

	if( *s != '}' )
	{
		goto json_parse_object_error;
	}

	*json_string = s + 1;

	return ret;

json_parse_object_error:
	json_decref( ret );
	return NULL;
}




json_t * _json_parse( char ** buff )
{

	if( *buff == NULL )
		return NULL;

	switch( json_infer_type( *buff ) )
	{
		case JSON_NULL :
			//printf("NULL\n");
			return json_null();
		case JSON_BOOL:
			return json_parse_bool( buff );
		case JSON_STRING:
			//printf("STRING : ");
			return json_parse_string( buff );
		case JSON_INT:
			//printf("INT\n");
			return json_parse_int( buff );
		case JSON_REAL:
			//printf("REAL\n");
			return json_parse_real( buff );
		case JSON_OBJECT:
			//printf("OBJECT\n");
			return json_parse_object( buff );
		case JSON_ARRAY:
			//printf("ARRAY\n");
			return json_parse_array( buff );
		case JSON_SOMETHING_ELSE:
			return NULL;
	}

	return NULL;
}

json_t * json_parse( char * json )
{
	char * tmp = strdup( json );

	char *s = tmp;

	json_t *ret =_json_parse( &s );

	free( tmp );

	return ret;
}
