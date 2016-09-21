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
#ifndef MPIT_INTERNAL_H
#define MPIT_INTERNAL_H

#include "mpc_mpi.h"
#include "sctk_spinlock.h"
#include "sctk_ht.h"

/************************************************************************/
/* INTERNAL MPI_T Init and Finalize                                     */
/************************************************************************/

int mpc_MPI_T_init_thread( int required, int * provided );

int mpc_MPI_T_finalize( void );

/************************************************************************/
/* INTERNAL MPI_T Enum                                                  */
/************************************************************************/

/** This is the internal MPI_T_enum data-structure */
struct MPC_T_enum
{
	/** The enum infos */
	char * name; /**<< Name of the enum */
	int number_of_entries; /**<< Number of entries in the enum */
	/** Content of the enum */
	char ** entry_names; /**<< Name for each member */
	int * entry_values; /**<< Content for each value */
};

/** Initialize an MPI_T internal enum
 * @arg en A pointer to the internal enum
 * @arg name The name of the enum
 * @arg num_entries The number of entries in the enum 
 * @arg names An array of names matching the enum size
 * @arg valies An array of values matching the enum size
 * @return Non zero on error
 */
int MPC_T_enum_init( struct MPC_T_enum * en, char * name, int num_entries, char *names[], int *values);


/** Free an internal MPI_T enum
 * @arg en The enum to be freed
 * @return Non zero on error
 */
int MPC_T_enum_release( struct MPC_T_enum * en);

/** Get a pointer to a value in an internal enum
 * @arg en Enum to look into
 * @arg index Index to peek from
 * @return Pointer to value (NULL if out of bound )
 */
static inline int *  MPC_T_enum_get_pvalue( struct MPC_T_enum *en, int index )
{
	if( !en )
	{
		return NULL;
	}

	if( index < en->number_of_entries )
	{
		return &en->entry_values[index];
	}

	return NULL;
}

/** Get a value name in an internal enum
 * @arg en Enum to look into
 * @arg index Index to peek from
 * @return Pointer to name (NULL if out of bound )
 */
static inline char *  MPC_T_enum_get_name( struct MPC_T_enum *en, int index )
{
	if( !en )
	{
		return NULL;
	}

	if( index < en->number_of_entries )
	{
		return en->entry_names[index];
	}

	return NULL;
}

/** MPI_T_enum internal interface */

int mpc_MPI_T_enum_get_info( MPI_T_enum enumtype, int * num, char * name, int * name_len );

int mpc_MPI_T_enum_get_item( MPI_T_enum enumtype, int index, int * value, char * name, int * name_len );

/************************************************************************/
/* INTERNAL MPI_T Control variables CVARS                               */
/************************************************************************/

/** This is CVAR meta-data */

struct MPC_T_cvar
{
	int index; /**<< Index is offset in the CVAR array */
	char * name; /**<< Name of the CVAR */
	MPC_T_verbosity verbosity; /**<< Verbosity of this CVAR */
	MPI_Datatype datatype; /**<< Datatype of the CVAR */
	struct MPC_T_enum enumtype; /**<< Enumtype of the CVAR */
	char * desc; /**<< Description of the CVAR */
	MPC_T_binding bind; /**<< Binding MPI obj of the CVAR */
	MPC_T_cvar_scope scope; /**<< Scope of the CVAR */
};

/** These are the known CVAR types */

typedef enum
{
	/** INCLUDE HERE */
	MPC_T_CVAR_COUNT /**<< This is the number of static CVARS */
}MPC_T_cvar_t;

/** This is where the CVARS (meta) are being stored */

struct MPC_T_cvars_array
{
	struct MPC_T_cvar st_vars[MPC_T_CVAR_COUNT]; /**<< An array of static CVARS */
	int dyn_cvar_count;	/**<< The number of dynamic CVARS */
	struct MPC_T_cvar * dyn_cvars; /*<< The array of dynamic CVARS */
	sctk_spinlock_t lock; /**<< A lock to rule them all */
};

/** Initialize the CVAR array storage (done once)
 * @return Non-zero on error
 */
int MPI_T_cvars_array_init();

/** Release the CVAR array storage (done once)
 * @return Non-zero on error
 */
int MPI_T_cvars_array_release();

/** Get a CVAR either from the dynamic array or the static one
 * @note Use this function to store static CVARS
 * @arg slot Where to take it from
 * @return NULL if not found, the pointer otherwise
 */
struct MPC_T_cvar * MPI_T_cvars_array_get( MPC_T_cvar_t slot );

/** Get a CVAR from the dynamic array
 * @arg cvar_index The index of the new CVAR
 * @return NULL if error, the pointer otherwise
 */
struct MPC_T_cvar * MPI_T_cvars_array_new( int * cvar_index );


/** This is how to manipulate CVARS */

int MPC_T_cvar_register( char * name,
		MPC_T_verbosity verbosity, 
		MPI_Datatype datatype,
		MPI_T_enum enumtype ,
		char * desc, 
		MPC_T_binding bind, 
		MPC_T_cvar_scope  scope );


int MPC_T_cvar_register_on_slot( int event_key,
		char * name,
		MPC_T_verbosity verbosity, 
		MPI_Datatype datatype,
		MPI_T_enum enumtype ,
		char * desc, 
		MPC_T_binding bind, 
		MPC_T_cvar_scope scope );


/** MPI CVAR Interface */

int mpc_MPI_T_cvar_get_num( int *num_cvar );

int mpc_MPI_T_cvar_get_info( int cvar_index,
		char * name,
		int * name_len,
		int * verbosity, 
		MPI_Datatype *datatype,
		MPI_T_enum * enumtype ,
		char * desc, 
		int * desc_len, 
		int * bind, 
		int * scope );

int mpc_MPI_T_cvar_get_index( const char * name, int * cvar_index );

/************************************************************************/
/* INTERNAL MPI_T Control variables CVARS HANDLES                       */
/************************************************************************/

int mpc_MPI_T_cvar_handle_alloc( int cvar_index , void * obj_handle, MPI_T_cvar_handle *handle, int * count );

int mpc_MPI_T_cvar_handle_free( MPI_T_cvar_handle * handle );

/************************************************************************/
/* INTERNAL MPI_T Control variables CVARS ACCESS                        */
/************************************************************************/

int mpc_MPI_T_cvar_read( MPI_T_cvar_handle handle, void * buff );

int mpc_MPI_T_cvar_write( MPI_T_cvar_handle handle, const void * buff );

/************************************************************************/
/* INTERNAL MPI_T Performance variables PVARS                           */
/************************************************************************/

/** This is the PVAR meta-data */

struct MPC_T_pvar
{
	int pvar_index;
	char *name;
	MPC_T_verbosity verbosity;
	MPC_T_pvar_class pvar_class;
	MPI_Datatype datatype;
	MPI_T_enum enumtype;
	char * desc;
	MPC_T_binding bind;
	int  readonly;
	int continuous;
	int atomic;
};

/** This is the PVAR storage */

typedef enum
{
	/** INCLUDE HERE */
	MPC_T_PVAR_COUNT
}MPC_T_pvar_t;

struct MPC_T_pvars_array
{
	struct MPC_T_pvar st_pvar[MPC_T_PVAR_COUNT];

	int dyn_pvar_count;
	struct MPC_T_pvar * dyn_pvars;

	sctk_spinlock_t lock;
};


/** Initialize the PVAR array storage (done once)
 * @return Non-zero on error
 */
int MPI_T_pvars_array_init();

/** Release the CVAR array storage (done once)
 * @return Non-zero on error
 */
int MPI_T_pvars_array_release();

/** Get a PVAR either from the dynamic array or the static one
 * @note Use this function to store static PVARS
 * @arg slot Where to take it from
 * @return NULL if not found, the pointer otherwise
 */
struct MPC_T_pvar * MPI_T_pvars_array_get( int slot );

/** Get a PVAR from the dynamic array
 * @arg pvar_index The index of the new PVAR
 * @return NULL if error, the pointer otherwise
 */
struct MPC_T_pvar * MPI_T_pvars_array_new( int * pvar_index );

/** This is how to manipulate PVARS */

int mpc_MPI_T_pvars_array_register_on_slot(    int pvar_index,
		char *name,
		MPC_T_verbosity verbosity,
		MPC_T_pvar_class var_class,
		MPI_Datatype datatype,
		MPI_T_enum enumtype,
		char * desc,
		MPC_T_binding bind,
		int readonly,
		int continuous,
		int atomic );

int mpc_MPI_T_pvars_array_register( char *name,
		MPC_T_verbosity verbosity,
		MPC_T_pvar_class var_class,
		MPI_Datatype datatype,
		MPI_T_enum enumtype,
		char * desc,
		MPC_T_binding bind,
		int readonly,
		int continuous,
		int atomic );


/** This is the PVAR MPI interface */

int mpc_MPI_T_pvar_get_num( int * num_pvar );

int mpc_MPI_T_pvar_get_info(    int pvar_index,
		char *name,
		int * name_len,
		int * verbosity,
		int * var_class,
		MPI_Datatype * datatype,
		MPI_T_enum * enumtype,
		char * desc,
		int * desc_len,
		int * bind,
		int * readonly,
		int * continuous,
		int * atomic );


int mpc_MPI_T_pvar_get_index( char * name , int * pvar_class, int * pvar_index );

/************************************************************************/
/* INTERNAL MPI_T Performance variables PVARS SESSIONS                  */
/************************************************************************/

int mpc_MPI_T_pvar_session_create( MPI_T_pvar_session * session ); 

int mpc_MPI_T_pvar_session_free( MPI_T_pvar_session * session ); 

/************************************************************************/
/* INTERNAL MPI_T Performance variables PVARS Handle Allocation         */
/************************************************************************/

int mpc_MPI_T_pvar_handle_alloc( MPI_T_pvar_session session,
		int pvar_index,
		void * obj_handle,
		MPI_T_pvar_handle * handle, 
		int * count );


int mpc_MPI_T_pvar_handle_free( MPI_T_pvar_handle * handle );

/************************************************************************/
/* INTERNAL MPI_T Performance variables PVARS Start and Stop            */
/************************************************************************/

int mpc_MPI_T_pvar_start( MPI_T_pvar_session session, MPI_T_pvar_handle handle );

int mpc_MPI_T_pvar_stop( MPI_T_pvar_session session, MPI_T_pvar_handle handle );

/************************************************************************/
/* INTERNAL MPI_T Performance variables PVARS Read and Write            */
/************************************************************************/

int mpc_MPI_T_pvar_read( MPI_T_pvar_session session, MPI_T_pvar_handle handle, void * buff );

int mpc_MPI_T_pvar_readreset( MPI_T_pvar_session session, MPI_T_pvar_handle handle, void * buff );

int mpc_MPI_T_pvar_write( MPI_T_pvar_session session, MPI_T_pvar_handle handle, const void * buff );

int mpc_MPI_T_pvar_reset( MPI_T_pvar_session session, MPI_T_pvar_handle handle );

/************************************************************************/
/* INTERNAL MPI_T variables categorization                              */
/************************************************************************/

int mpc_MPI_T_category_get_num( int * num_cat );


int mpc_MPI_T_category_get_info( int cat_index, 
		char * name, 
		int * name_len, 
		char * desc, 
		int * desc_len,
		int * num_cvars, 
		int * num_pvars,
		int * num_categories);

int mpc_MPI_T_category_get_index( char * name, int * cat_index );

int mpc_MPI_T_category_get_cvars( int cat_index, int len, int indices[]);

int mpc_MPI_T_category_get_pvars( int cat_index, int len, int indices[]);

int mpc_MPI_T_category_get_categories( int cat_index, int len, int indices[]);

int mpc_MPI_T_category_changed( int * stamp );

/************************************************************************/
/* INTERNAL MPI_T storage                                               */
/************************************************************************/

typedef enum
{
	MPC_T_CVAR_TYPE,
	MPC_T_PVAR_TYPE
}MPC_T_data_type;

struct MPC_T_data
{
	MPC_T_data_type var_type;
	MPI_Datatype type;
	
	union
	{
		int i;
		unsigned int ui;
		unsigned long int lu;
		unsigned long long  llu;
		double d;
		MPI_Count count;
		char c;
	}content_t;

	union
	{
		struct MPC_T_pvar *pvar;
		struct MPC_T_cvar * cvar;
	};

};


/************************************************************************/
/* INTERNAL MPI_T session                                               */
/************************************************************************/

struct MPC_T_session
{
	struct MPCHT handle_ht;
	int handle_count;
	sctk_spinlock_t lock;
};



struct MPC_T_session_array
{
	sctk_spinlock_t lock;
	struct MPC_T_session * sessions;
	int session_count;
};




#endif /* MPIT_INTERNAL_H */
