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
#include "mpc_info.h"

#include "sctk_debug.h"
#include "sctk_alloc.h"
#include "string.h"
#include "mpc.h"

/***************
* MPC_Info_key *
***************/

/* Init an release */

struct MPC_Info_key * MPC_Info_key_init( char * key, char * value )
{
	/* Allocate and fill */
	struct MPC_Info_key * ret = sctk_malloc( sizeof( struct MPC_Info_key ) );
	
	if( !ret )
	{
		perror("sctk_alloc");
		return NULL;
	}
	
	ret->prev = NULL;	
	ret->next = NULL;	
	
	ret->key = strdup( key );
	ret->value = strdup( value );
	
	return ret;
}


void MPC_Info_key_release( struct MPC_Info_key * tofree )
{
	/* We are at the end of the recursivity */
	if( !tofree )
		return;
	
	/* Free content */
	free( tofree->key );
	tofree->key = NULL;
	
	free( tofree->value );
	tofree->value= NULL;
	
	/* Break the chain */
	tofree->prev = NULL;
	tofree->next = NULL;
	
	/* When going up free the entry itself */
	sctk_free( tofree );
}

void MPC_Info_key_release_recursive( struct MPC_Info_key * tofree )
{
	/* We are at the end of the recursivity */
	if( !tofree )
		return;
	
	/* Go to next */
	MPC_Info_key_release_recursive( tofree->next );
	
	/* When going up free the entry itself */
	MPC_Info_key_release( tofree );
}

/* Getters and setters */

struct MPC_Info_key * MPC_Info_key_get( struct MPC_Info_key * head, char * key )
{
	/* Prepare to look for the key */
	struct MPC_Info_key * current = head;
	
	while( current )
	{
		/* Is it the right key ? */
		if( !strcmp( current->key ,  key ) )
			return current;
		
		/* Go to next */
		current = current->next;	
	}
	
	/* Not found ;( */
	return NULL;
}

/* Methods */

struct MPC_Info_key * MPC_Info_key_set( struct MPC_Info_key * head, char * key, char * value )
{
	struct MPC_Info_key * previous = MPC_Info_key_get( head, key );
	
	if( previous )
	{
		/* Update the content KEY is unchanged as it mathes by definition of MPC_Info_key_get */
		free( previous->value );
		previous->value = strdup( value );
	}
	else
	{
		struct MPC_Info_key * new_entry = MPC_Info_key_init( key, value );
		
		/* Are we the first element ? */
		if( head == NULL )
			return new_entry;
		
		/* Here we have to push in order by definition of the MPI standard */
		struct MPC_Info_key * last = head;
		
		while( last->next )
			last = last->next;
		
		/* Now last is the last block proceed with the actual push */
		last->next = new_entry;
		
		if( !last->next )
		{
			/* Failled to allocate */
			return head;
		}
		
		/* Link the last to the previous */
		last->next->prev = last;
		
	}
	
	return head;
}

struct MPC_Info_key * MPC_Info_key_get_nth( struct MPC_Info_key * head, int n )
{
	int cnt = 0;
	
	/* Prepare to look for the key */
	struct MPC_Info_key * current = head;
	
	while( current )
	{
		/* Is it the nth key ? */
		if( cnt == n )
			return current;
		
		/* Go to next */
		cnt++;
		current = current->next;	
	}	
	
	return NULL;
}


int MPC_Info_key_count( struct MPC_Info_key * head )
{
	int cnt = 0;
	
	/* Prepare to look for the key */
	struct MPC_Info_key * current = head;
	
	while( current )
	{
		/* Go to next */
		cnt++;
		current = current->next;	
	}	
	
	return cnt;
}


struct MPC_Info_key * MPC_Info_key_pop( struct MPC_Info_key * topop )
{
	if( !topop )
		return NULL;
	
	struct MPC_Info_key * next = NULL;
	struct MPC_Info_key * prev = NULL;

	/* Extract next and prev */
	
	if( topop->next )
	{
		next = topop->next;
	}
	
	if( topop->prev )
	{
		next = topop->prev;
	}
	
	/* Update links */
	
	if( next )
	{
		next->prev = prev;
	}
	else
	{
		if( prev )
			prev->next = NULL;
	}
	
	if( prev )
	{
		prev->next = next;
	}
	else
	{
		if( next )
			next->prev = NULL;
	}
	
	return topop;
}


struct MPC_Info_key * MPC_Info_key_delete( struct MPC_Info_key * head, char * key, int * flag )
{
	/* Can we find the key to delete ? */
	struct MPC_Info_key *topop = MPC_Info_key_get( head,  key );
	
	/* Not found */
	if( !topop )
	{
		*flag = 0;
		return head;
	}
	
	/* Found proceed with popping */
	MPC_Info_key_pop( topop );
	*flag = 1;

	struct MPC_Info_key *ret;
	
	if( topop == head )
		ret = topop->next;
	else
		ret = head;

	MPC_Info_key_release( topop );	
	
	return ret;
}

/****************
* MPC_Info_cell *
****************/

/* Init an release */
struct MPC_Info_cell * MPC_Info_cell_init( int id )
{
	struct MPC_Info_cell * ret = sctk_malloc( sizeof( struct MPC_Info_cell ) );
	
	if( !ret )
	{
		perror("sctk_alloc");
		return NULL;
	}
	
	MPC_Comm_rank( MPC_COMM_WORLD, &ret->rank );
	ret->id = id;
	
	ret->keys = NULL;
	
	return ret;
}

void MPC_Info_cell_release( struct MPC_Info_cell * cell )
{
	cell->rank = -1;
	cell->id = 0;

	MPC_Info_key_release_recursive( cell->keys );
}


/* Getters and setters */

int MPC_Info_cell_get( struct MPC_Info_cell * cell , char * key , char * dest, int maxlen, int * flag )
{
	if( !flag )
	{
		sctk_error("Flag cannot be NULL in %s", __FUNCTION__);
	}
	
	
	struct MPC_Info_key * entry = MPC_Info_key_get( cell->keys, key );
	
	if( !entry )
	{
		*flag = 0;
		dest[0] = '\0';
		return 1;
	}
	else
	{
		*flag = 1;
		snprintf( dest, maxlen, "%s", entry->value );  
	}
	
	return 0;
}

int MPC_Info_cell_set( struct MPC_Info_cell * cell , char * key, char * value, int overwrite )
{
	if( !overwrite )
	{
		/* First check if already present */
		struct MPC_Info_key * previous = MPC_Info_key_get( cell->keys, key );
		
		if( previous )
		{
			/* We do not want to overwrite */
			return 1;
		}
	}
	
	/* Otherwise directly set / create the new entry */
	cell->keys = MPC_Info_key_set(  cell->keys, key, value );
}

/* Methods */

int MPC_Info_cell_delete( struct MPC_Info_cell * cell , char * key )
{
	int did_delete = 0;
	cell->keys =  MPC_Info_key_delete( cell->keys, key , &did_delete);
	
	/* 1 is the error case */
	return !did_delete;
}
