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
/* #   - VALAT Sebastien sebastien.valat@cea.fr                           # */
/* #   - AUTOMATIC GENERATION                                             # */
/* #                                                                      # */
/* ######################################################################## */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "sctk_runtime_config_struct.h"
#include "sctk_runtime_config_struct_defaults.h"
#include "sctk_runtime_config_mapper.h"
#ifdef MPC_MPI
#include "mpit_internal.h"
/* This is used to avoid variable duplicates for CVAR handles */ 
static struct MPC_T_cvar * the_cvar = NULL;
#endif
#include "sctk_runtime_config_mapper.h"
/* This is used to avoid variable duplicates for CVAR resolution*/ 
static int the_temp_index = -1;

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_accl_cuda(void * struct_ptr)
{
	struct sctk_runtime_config_struct_accl_cuda * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->enabled = false;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_accl_openacc(void * struct_ptr)
{
	struct sctk_runtime_config_struct_accl_openacc * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->enabled = false;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_accl_opencl(void * struct_ptr)
{
	struct sctk_runtime_config_struct_accl_opencl * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->enabled = false;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_accl(void * struct_ptr)
{
	struct sctk_runtime_config_struct_accl * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->enabled = false;
	sctk_runtime_config_struct_init_accl_cuda(&obj->cuda);
	sctk_runtime_config_struct_init_accl_openacc(&obj->openacc);
	sctk_runtime_config_struct_init_accl_opencl(&obj->opencl);
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_allocator(void * struct_ptr)
{
	struct sctk_runtime_config_struct_allocator * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->numa_migration = false;
	obj->realloc_factor = 2;
	obj->realloc_threashold = sctk_runtime_config_map_entry_parse_size("50MB");
	obj->numa = true;
	obj->strict = false;
	obj->keep_mem = sctk_runtime_config_map_entry_parse_size("500MB");
	obj->keep_max = sctk_runtime_config_map_entry_parse_size("8MB");
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_launcher(void * struct_ptr)
{
	struct sctk_runtime_config_struct_launcher * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->verbosity = 0;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "LN_VERBOSITY" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->verbosity ) )
			{
				fprintf(stderr,"Error size mismatch for LN_VERBOSITY");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->verbosity);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for LN_VERBOSITY");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for LN_VERBOSITY");
			abort();
		}
#endif
				obj->banner = true;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "LN_BANNER" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->banner ) )
			{
				fprintf(stderr,"Error size mismatch for LN_BANNER");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->banner);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for LN_BANNER");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for LN_BANNER");
			abort();
		}
#endif
				obj->autokill = 0;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "LN_AUTOKILL" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->autokill ) )
			{
				fprintf(stderr,"Error size mismatch for LN_AUTOKILL");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->autokill);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for LN_AUTOKILL");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for LN_AUTOKILL");
			abort();
		}
#endif
				obj->user_launchers = "default";
	obj->keep_rand_addr = true;
	obj->disable_rand_addr = false;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "LN_ADDR_RAND_DISABLE" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->disable_rand_addr ) )
			{
				fprintf(stderr,"Error size mismatch for LN_ADDR_RAND_DISABLE");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->disable_rand_addr);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for LN_ADDR_RAND_DISABLE");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for LN_ADDR_RAND_DISABLE");
			abort();
		}
#endif
				obj->disable_mpc = false;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "LN_DISABLE_MPC" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->disable_mpc ) )
			{
				fprintf(stderr,"Error size mismatch for LN_DISABLE_MPC");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->disable_mpc);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for LN_DISABLE_MPC");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for LN_DISABLE_MPC");
			abort();
		}
#endif
				obj->thread_init.name = "sctk_use_ethread_mxn";
	*(void **) &(obj->thread_init.value) = sctk_runtime_config_get_symbol("sctk_use_ethread_mxn");
	obj->nb_task = 1;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "LN_MPI_TASK" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->nb_task ) )
			{
				fprintf(stderr,"Error size mismatch for LN_MPI_TASK");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->nb_task);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for LN_MPI_TASK");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for LN_MPI_TASK");
			abort();
		}
#endif
				obj->nb_process = 1;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "LN_MPC_PROCESS" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->nb_process ) )
			{
				fprintf(stderr,"Error size mismatch for LN_MPC_PROCESS");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->nb_process);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for LN_MPC_PROCESS");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for LN_MPC_PROCESS");
			abort();
		}
#endif
				obj->nb_processor = 0;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "LN_MPC_VP" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->nb_processor ) )
			{
				fprintf(stderr,"Error size mismatch for LN_MPC_VP");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->nb_processor);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for LN_MPC_VP");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for LN_MPC_VP");
			abort();
		}
#endif
				obj->nb_node = 1;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "LN_MPC_NODE" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->nb_node ) )
			{
				fprintf(stderr,"Error size mismatch for LN_MPC_NODE");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->nb_node);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for LN_MPC_NODE");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for LN_MPC_NODE");
			abort();
		}
#endif
				obj->launcher = "none";
	obj->max_try = 10;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "LN_MAX_TOPO_TRY" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->max_try ) )
			{
				fprintf(stderr,"Error size mismatch for LN_MAX_TOPO_TRY");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->max_try);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for LN_MAX_TOPO_TRY");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for LN_MAX_TOPO_TRY");
			abort();
		}
#endif
				obj->vers_details = false;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "LN_DISPLAY_VERS" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->vers_details ) )
			{
				fprintf(stderr,"Error size mismatch for LN_DISPLAY_VERS");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->vers_details);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for LN_DISPLAY_VERS");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for LN_DISPLAY_VERS");
			abort();
		}
#endif
				obj->profiling = "stdout";
	obj->enable_smt = false;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "LN_ENABLE_SMT" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->enable_smt ) )
			{
				fprintf(stderr,"Error size mismatch for LN_ENABLE_SMT");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->enable_smt);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for LN_ENABLE_SMT");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for LN_ENABLE_SMT");
			abort();
		}
#endif
				obj->share_node = false;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "LN_SHARE_NODE" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->share_node ) )
			{
				fprintf(stderr,"Error size mismatch for LN_SHARE_NODE");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->share_node);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for LN_SHARE_NODE");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for LN_SHARE_NODE");
			abort();
		}
#endif
				obj->restart = false;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "LN_RESTART" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->restart ) )
			{
				fprintf(stderr,"Error size mismatch for LN_RESTART");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->restart);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for LN_RESTART");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for LN_RESTART");
			abort();
		}
#endif
				obj->checkpoint = false;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "LN_CHECKPOINT" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->checkpoint ) )
			{
				fprintf(stderr,"Error size mismatch for LN_CHECKPOINT");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->checkpoint);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for LN_CHECKPOINT");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for LN_CHECKPOINT");
			abort();
		}
#endif
				obj->migration = false;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "LN_MIGRATION" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->migration ) )
			{
				fprintf(stderr,"Error size mismatch for LN_MIGRATION");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->migration);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for LN_MIGRATION");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for LN_MIGRATION");
			abort();
		}
#endif
				obj->report = false;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "LN_REPORT" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->report ) )
			{
				fprintf(stderr,"Error size mismatch for LN_REPORT");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->report);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for LN_REPORT");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for LN_REPORT");
			abort();
		}
#endif
				obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_debugger(void * struct_ptr)
{
	struct sctk_runtime_config_struct_debugger * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->colors = true;
	obj->max_filename_size = 1024;
	obj->mpc_bt_sig = 1;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_ft(void * struct_ptr)
{
	struct sctk_runtime_config_struct_ft * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->enabled = false;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_collectives_shm_shared(void * struct_ptr)
{
	struct sctk_runtime_config_struct_collectives_shm_shared * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->barrier_intra_shared_node.name = "__INTERNAL__PMPI_Barrier_intra_shared_node";
	*(void **) &(obj->barrier_intra_shared_node.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Barrier_intra_shared_node");
	obj->bcast_intra_shared_node.name = "__INTERNAL__PMPI_Bcast_intra_shared_node";
	*(void **) &(obj->bcast_intra_shared_node.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Bcast_intra_shared_node");
	obj->alltoall_intra_shared_node.name = "__INTERNAL__PMPI_Alltoall_intra_shared_node";
	*(void **) &(obj->alltoall_intra_shared_node.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Alltoall_intra_shared_node");
	obj->scatter_intra_shared_node.name = "__INTERNAL__PMPI_Scatter_intra_shared_node";
	*(void **) &(obj->scatter_intra_shared_node.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Scatter_intra_shared_node");
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_collectives_shm(void * struct_ptr)
{
	struct sctk_runtime_config_struct_collectives_shm * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->barrier_intra_shm.name = "__INTERNAL__PMPI_Barrier_intra_shm";
	*(void **) &(obj->barrier_intra_shm.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Barrier_intra_shm");
	obj->bcast_intra_shm.name = "__INTERNAL__PMPI_Bcast_intra_shm";
	*(void **) &(obj->bcast_intra_shm.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Bcast_intra_shm");
	obj->alltoallv_intra_shm.name = "__INTERNAL__PMPI_Alltoallv_intra_shm";
	*(void **) &(obj->alltoallv_intra_shm.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Alltoallv_intra_shm");
	obj->gatherv_intra_shm.name = "__INTERNAL__PMPI_Gatherv_intra_shm";
	*(void **) &(obj->gatherv_intra_shm.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Gatherv_intra_shm");
	obj->scatterv_intra_shm.name = "__INTERNAL__PMPI_Scatterv_intra_shm";
	*(void **) &(obj->scatterv_intra_shm.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Scatterv_intra_shm");
	obj->reduce_intra_shm.name = "__INTERNAL__PMPI_Reduce_shm";
	*(void **) &(obj->reduce_intra_shm.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Reduce_shm");
	obj->topo_tree_arity = -1;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "MPI_COLL_TOPO_TREE_ARITY" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->topo_tree_arity ) )
			{
				fprintf(stderr,"Error size mismatch for MPI_COLL_TOPO_TREE_ARITY");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->topo_tree_arity);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for MPI_COLL_TOPO_TREE_ARITY");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for MPI_COLL_TOPO_TREE_ARITY");
			abort();
		}
#endif
				obj->topo_tree_dump = false;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "MPI_COLL_TOPO_TREE_DUMP" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->topo_tree_dump ) )
			{
				fprintf(stderr,"Error size mismatch for MPI_COLL_TOPO_TREE_DUMP");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->topo_tree_dump);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for MPI_COLL_TOPO_TREE_DUMP");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for MPI_COLL_TOPO_TREE_DUMP");
			abort();
		}
#endif
				obj->coll_force_nocommute = false;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "MPI_COLL_NO_COMMUTE" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->coll_force_nocommute ) )
			{
				fprintf(stderr,"Error size mismatch for MPI_COLL_NO_COMMUTE");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->coll_force_nocommute);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for MPI_COLL_NO_COMMUTE");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for MPI_COLL_NO_COMMUTE");
			abort();
		}
#endif
				obj->reduce_pipelined_blocks = 16;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "MPI_COLL_REDUCE_PIPELINED_BLOCKS" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->reduce_pipelined_blocks ) )
			{
				fprintf(stderr,"Error size mismatch for MPI_COLL_REDUCE_PIPELINED_BLOCKS");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->reduce_pipelined_blocks);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for MPI_COLL_REDUCE_PIPELINED_BLOCKS");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for MPI_COLL_REDUCE_PIPELINED_BLOCKS");
			abort();
		}
#endif
				obj->reduce_pipelined_tresh = sctk_runtime_config_map_entry_parse_size("1KB");

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "MPI_COLL_REDUCE_INTERLEAVE_TRSH" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->reduce_pipelined_tresh ) )
			{
				fprintf(stderr,"Error size mismatch for MPI_COLL_REDUCE_INTERLEAVE_TRSH");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->reduce_pipelined_tresh);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for MPI_COLL_REDUCE_INTERLEAVE_TRSH");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for MPI_COLL_REDUCE_INTERLEAVE_TRSH");
			abort();
		}
#endif
				obj->reduce_interleave = 8;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "MPI_COLL_REDUCE_INTERLEAVE" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->reduce_interleave ) )
			{
				fprintf(stderr,"Error size mismatch for MPI_COLL_REDUCE_INTERLEAVE");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->reduce_interleave);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for MPI_COLL_REDUCE_INTERLEAVE");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for MPI_COLL_REDUCE_INTERLEAVE");
			abort();
		}
#endif
				obj->bcast_interleave = 8;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "MPI_COLL_BCAST_INTERLEAVE" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->bcast_interleave ) )
			{
				fprintf(stderr,"Error size mismatch for MPI_COLL_BCAST_INTERLEAVE");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->bcast_interleave);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for MPI_COLL_BCAST_INTERLEAVE");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for MPI_COLL_BCAST_INTERLEAVE");
			abort();
		}
#endif
				obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_collectives_intra(void * struct_ptr)
{
	struct sctk_runtime_config_struct_collectives_intra * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->barrier_intra.name = "__INTERNAL__PMPI_Barrier_intra";
	*(void **) &(obj->barrier_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Barrier_intra");
	obj->barrier_intra_for_trsh = 33;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "MPI_COLL_BARRIER_FOR_TRSH" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->barrier_intra_for_trsh ) )
			{
				fprintf(stderr,"Error size mismatch for MPI_COLL_BARRIER_FOR_TRSH");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->barrier_intra_for_trsh);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for MPI_COLL_BARRIER_FOR_TRSH");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for MPI_COLL_BARRIER_FOR_TRSH");
			abort();
		}
#endif
				obj->bcast_intra.name = "__INTERNAL__PMPI_Bcast_intra";
	*(void **) &(obj->bcast_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Bcast_intra");
	obj->bcast_intra_for_trsh = 33;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "MPI_COLL_BCAST_FOR_TRSH" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->bcast_intra_for_trsh ) )
			{
				fprintf(stderr,"Error size mismatch for MPI_COLL_BCAST_FOR_TRSH");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->bcast_intra_for_trsh);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for MPI_COLL_BCAST_FOR_TRSH");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for MPI_COLL_BCAST_FOR_TRSH");
			abort();
		}
#endif
				obj->bcast_intra_for_count_trsh = 1024;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "MPI_COLL_BCAST_FOR_ELEM_TRSH" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->bcast_intra_for_count_trsh ) )
			{
				fprintf(stderr,"Error size mismatch for MPI_COLL_BCAST_FOR_ELEM_TRSH");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->bcast_intra_for_count_trsh);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for MPI_COLL_BCAST_FOR_ELEM_TRSH");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for MPI_COLL_BCAST_FOR_ELEM_TRSH");
			abort();
		}
#endif
				obj->allgather_intra.name = "__INTERNAL__PMPI_Allgather_intra";
	*(void **) &(obj->allgather_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Allgather_intra");
	obj->allgatherv_intra.name = "__INTERNAL__PMPI_Allgatherv_intra";
	*(void **) &(obj->allgatherv_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Allgatherv_intra");
	obj->alltoall_intra.name = "__INTERNAL__PMPI_Alltoall_intra";
	*(void **) &(obj->alltoall_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Alltoall_intra");
	obj->alltoallv_intra.name = "__INTERNAL__PMPI_Alltoallv_intra";
	*(void **) &(obj->alltoallv_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Alltoallv_intra");
	obj->alltoallw_intra.name = "__INTERNAL__PMPI_Alltoallw_intra";
	*(void **) &(obj->alltoallw_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Alltoallw_intra");
	obj->gather_intra.name = "__INTERNAL__PMPI_Gather_intra";
	*(void **) &(obj->gather_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Gather_intra");
	obj->gatherv_intra.name = "__INTERNAL__PMPI_Gatherv_intra";
	*(void **) &(obj->gatherv_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Gatherv_intra");
	obj->scatter_intra.name = "__INTERNAL__PMPI_Scatter_intra";
	*(void **) &(obj->scatter_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Scatter_intra");
	obj->scatterv_intra.name = "__INTERNAL__PMPI_Scatterv_intra";
	*(void **) &(obj->scatterv_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Scatterv_intra");
	obj->scan_intra.name = "__INTERNAL__PMPI_Scan_intra";
	*(void **) &(obj->scan_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Scan_intra");
	obj->exscan_intra.name = "__INTERNAL__PMPI_Exscan_intra";
	*(void **) &(obj->exscan_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Exscan_intra");
	obj->reduce_intra.name = "__INTERNAL__PMPI_Reduce_intra";
	*(void **) &(obj->reduce_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Reduce_intra");
	obj->reduce_intra_for_trsh = 33;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "MPI_COLL_REDUCE_FOR_TRSH" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->reduce_intra_for_trsh ) )
			{
				fprintf(stderr,"Error size mismatch for MPI_COLL_REDUCE_FOR_TRSH");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->reduce_intra_for_trsh);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for MPI_COLL_REDUCE_FOR_TRSH");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for MPI_COLL_REDUCE_FOR_TRSH");
			abort();
		}
#endif
				obj->reduce_intra_for_count_trsh = 1024;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "MPI_COLL_REDUCE_FOR_ELEM_TRSH" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->reduce_intra_for_count_trsh ) )
			{
				fprintf(stderr,"Error size mismatch for MPI_COLL_REDUCE_FOR_ELEM_TRSH");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->reduce_intra_for_count_trsh);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for MPI_COLL_REDUCE_FOR_ELEM_TRSH");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for MPI_COLL_REDUCE_FOR_ELEM_TRSH");
			abort();
		}
#endif
				obj->allreduce_intra.name = "__INTERNAL__PMPI_Allreduce_intra";
	*(void **) &(obj->allreduce_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Allreduce_intra");
	obj->reduce_scatter_intra.name = "__INTERNAL__PMPI_Reduce_scatter_intra";
	*(void **) &(obj->reduce_scatter_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Reduce_scatter_intra");
	obj->reduce_scatter_block_intra.name = "__INTERNAL__PMPI_Reduce_scatter_block_intra";
	*(void **) &(obj->reduce_scatter_block_intra.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Reduce_scatter_block_intra");
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_collectives_inter(void * struct_ptr)
{
	struct sctk_runtime_config_struct_collectives_inter * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->barrier_inter.name = "__INTERNAL__PMPI_Barrier_inter";
	*(void **) &(obj->barrier_inter.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Barrier_inter");
	obj->bcast_inter.name = "__INTERNAL__PMPI_Bcast_inter";
	*(void **) &(obj->bcast_inter.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Bcast_inter");
	obj->allgather_inter.name = "__INTERNAL__PMPI_Allgather_inter";
	*(void **) &(obj->allgather_inter.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Allgather_inter");
	obj->allgatherv_inter.name = "__INTERNAL__PMPI_Allgatherv_inter";
	*(void **) &(obj->allgatherv_inter.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Allgatherv_inter");
	obj->alltoall_inter.name = "__INTERNAL__PMPI_Alltoall_inter";
	*(void **) &(obj->alltoall_inter.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Alltoall_inter");
	obj->alltoallv_inter.name = "__INTERNAL__PMPI_Alltoallv_inter";
	*(void **) &(obj->alltoallv_inter.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Alltoallv_inter");
	obj->alltoallw_inter.name = "__INTERNAL__PMPI_Alltoallw_inter";
	*(void **) &(obj->alltoallw_inter.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Alltoallw_inter");
	obj->gather_inter.name = "__INTERNAL__PMPI_Gather_inter";
	*(void **) &(obj->gather_inter.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Gather_inter");
	obj->gatherv_inter.name = "__INTERNAL__PMPI_Gatherv_inter";
	*(void **) &(obj->gatherv_inter.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Gatherv_inter");
	obj->scatter_inter.name = "__INTERNAL__PMPI_Scatter_inter";
	*(void **) &(obj->scatter_inter.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Scatter_inter");
	obj->scatterv_inter.name = "__INTERNAL__PMPI_Scatterv_inter";
	*(void **) &(obj->scatterv_inter.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Scatterv_inter");
	obj->reduce_inter.name = "__INTERNAL__PMPI_Reduce_inter";
	*(void **) &(obj->reduce_inter.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Reduce_inter");
	obj->allreduce_inter.name = "__INTERNAL__PMPI_Allreduce_inter";
	*(void **) &(obj->allreduce_inter.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Allreduce_inter");
	obj->reduce_scatter_inter.name = "__INTERNAL__PMPI_Reduce_scatter_inter";
	*(void **) &(obj->reduce_scatter_inter.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Reduce_scatter_inter");
	obj->reduce_scatter_block_inter.name = "__INTERNAL__PMPI_Reduce_scatter_block_inter";
	*(void **) &(obj->reduce_scatter_block_inter.value) = sctk_runtime_config_get_symbol("__INTERNAL__PMPI_Reduce_scatter_block_inter");
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_nbc(void * struct_ptr)
{
	struct sctk_runtime_config_struct_nbc * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->use_progress_thread = 0;
	obj->progress_thread_binding.name = "sctk_get_progress_thread_binding_bind";
	*(void **) &(obj->progress_thread_binding.value) = sctk_runtime_config_get_symbol("sctk_get_progress_thread_binding_bind");
	obj->use_egreq_bcast = 0;
	obj->use_egreq_scatter = 0;
	obj->use_egreq_gather = 0;
	obj->use_egreq_reduce = 0;
	obj->use_egreq_barrier = 0;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_mpi_rma(void * struct_ptr)
{
	struct sctk_runtime_config_struct_mpi_rma * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->alloc_mem_pool_enable = 1;
	obj->alloc_mem_pool_size = sctk_runtime_config_map_entry_parse_size("1MB");
	obj->alloc_mem_pool_autodetect = 1;
	obj->alloc_mem_pool_force_process_linear = 0;
	obj->alloc_mem_pool_per_process_size = sctk_runtime_config_map_entry_parse_size("1MB");
	obj->win_thread_pool_max = 2;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_mpc(void * struct_ptr)
{
	struct sctk_runtime_config_struct_mpc * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->log_debug = false;
	obj->hard_checking = false;
	obj->buffering = false;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_net_driver_topological(void * struct_ptr)
{
	struct sctk_runtime_config_struct_net_driver_topological * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->dummy = 0;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_net_driver_infiniband(void * struct_ptr)
{
	struct sctk_runtime_config_struct_net_driver_infiniband * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->pkey = "undefined";
	obj->adm_port = 1;
	obj->verbose_level = 0;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_VERBOSE" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->verbose_level ) )
			{
				fprintf(stderr,"Error size mismatch for IB_VERBOSE");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->verbose_level);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_VERBOSE");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_VERBOSE");
			abort();
		}
#endif
				obj->eager_limit = 12288;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_EAGER_THRESH" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->eager_limit ) )
			{
				fprintf(stderr,"Error size mismatch for IB_EAGER_THRESH");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->eager_limit);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_EAGER_THRESH");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_EAGER_THRESH");
			abort();
		}
#endif
				obj->buffered_limit = 262114;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_BUFFERED_THRESH" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->buffered_limit ) )
			{
				fprintf(stderr,"Error size mismatch for IB_BUFFERED_THRESH");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->buffered_limit);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_BUFFERED_THRESH");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_BUFFERED_THRESH");
			abort();
		}
#endif
				obj->qp_tx_depth = 15000;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_TX_DEPTH" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->qp_tx_depth ) )
			{
				fprintf(stderr,"Error size mismatch for IB_TX_DEPTH");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->qp_tx_depth);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_TX_DEPTH");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_TX_DEPTH");
			abort();
		}
#endif
				obj->qp_rx_depth = 0;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_RX_DEPTH" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->qp_rx_depth ) )
			{
				fprintf(stderr,"Error size mismatch for IB_RX_DEPTH");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->qp_rx_depth);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_RX_DEPTH");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_RX_DEPTH");
			abort();
		}
#endif
				obj->cq_depth = 40000;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_CQ_DEPTH" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->cq_depth ) )
			{
				fprintf(stderr,"Error size mismatch for IB_CQ_DEPTH");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->cq_depth);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_CQ_DEPTH");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_CQ_DEPTH");
			abort();
		}
#endif
				obj->rdma_depth = 16;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_RDMA_DEPTH" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->rdma_depth ) )
			{
				fprintf(stderr,"Error size mismatch for IB_RDMA_DEPTH");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->rdma_depth);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_RDMA_DEPTH");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_RDMA_DEPTH");
			abort();
		}
#endif
				obj->max_sg_sq = 4;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_MAX_SG_SQ" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->max_sg_sq ) )
			{
				fprintf(stderr,"Error size mismatch for IB_MAX_SG_SQ");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->max_sg_sq);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_MAX_SG_SQ");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_MAX_SG_SQ");
			abort();
		}
#endif
				obj->max_sg_rq = 4;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_MAX_SG_RQ" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->max_sg_rq ) )
			{
				fprintf(stderr,"Error size mismatch for IB_MAX_SG_RQ");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->max_sg_rq);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_MAX_SG_RQ");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_MAX_SG_RQ");
			abort();
		}
#endif
				obj->max_inline = 128;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_MAX_INLINE" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->max_inline ) )
			{
				fprintf(stderr,"Error size mismatch for IB_MAX_INLINE");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->max_inline);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_MAX_INLINE");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_MAX_INLINE");
			abort();
		}
#endif
				obj->rdma_resizing = 0;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_RDMA_RESIZING" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->rdma_resizing ) )
			{
				fprintf(stderr,"Error size mismatch for IB_RDMA_RESIZING");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->rdma_resizing);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_RDMA_RESIZING");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_RDMA_RESIZING");
			abort();
		}
#endif
				obj->max_rdma_connections = 0;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_RDMA_CONN" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->max_rdma_connections ) )
			{
				fprintf(stderr,"Error size mismatch for IB_RDMA_CONN");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->max_rdma_connections);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_RDMA_CONN");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_RDMA_CONN");
			abort();
		}
#endif
				obj->max_rdma_resizing = 0;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_RDMA_MAX_RESIZING" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->max_rdma_resizing ) )
			{
				fprintf(stderr,"Error size mismatch for IB_RDMA_MAX_RESIZING");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->max_rdma_resizing);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_RDMA_MAX_RESIZING");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_RDMA_MAX_RESIZING");
			abort();
		}
#endif
				obj->init_ibufs = 1000;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_INIT_IBUF" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->init_ibufs ) )
			{
				fprintf(stderr,"Error size mismatch for IB_INIT_IBUF");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->init_ibufs);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_INIT_IBUF");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_INIT_IBUF");
			abort();
		}
#endif
				obj->init_recv_ibufs = 200;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_INIT_RECV_IBUF" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->init_recv_ibufs ) )
			{
				fprintf(stderr,"Error size mismatch for IB_INIT_RECV_IBUF");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->init_recv_ibufs);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_INIT_RECV_IBUF");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_INIT_RECV_IBUF");
			abort();
		}
#endif
				obj->max_srq_ibufs_posted = 1500;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_MAX_SRQ_IBUF_POSTED" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->max_srq_ibufs_posted ) )
			{
				fprintf(stderr,"Error size mismatch for IB_MAX_SRQ_IBUF_POSTED");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->max_srq_ibufs_posted);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_MAX_SRQ_IBUF_POSTED");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_MAX_SRQ_IBUF_POSTED");
			abort();
		}
#endif
				obj->max_srq_ibufs = 1000;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_MAX_SRQ_IBUF" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->max_srq_ibufs ) )
			{
				fprintf(stderr,"Error size mismatch for IB_MAX_SRQ_IBUF");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->max_srq_ibufs);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_MAX_SRQ_IBUF");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_MAX_SRQ_IBUF");
			abort();
		}
#endif
				obj->srq_credit_limit = 500;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_SRQ_CREDIT_LIMIT" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->srq_credit_limit ) )
			{
				fprintf(stderr,"Error size mismatch for IB_SRQ_CREDIT_LIMIT");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->srq_credit_limit);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_SRQ_CREDIT_LIMIT");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_SRQ_CREDIT_LIMIT");
			abort();
		}
#endif
				obj->srq_credit_thread_limit = 100;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_SRQ_CREDIT_LIMIT_THREAD" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->srq_credit_thread_limit ) )
			{
				fprintf(stderr,"Error size mismatch for IB_SRQ_CREDIT_LIMIT_THREAD");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->srq_credit_thread_limit);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_SRQ_CREDIT_LIMIT_THREAD");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_SRQ_CREDIT_LIMIT_THREAD");
			abort();
		}
#endif
				obj->size_ibufs_chunk = 100;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_IBUF_CHUNK" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->size_ibufs_chunk ) )
			{
				fprintf(stderr,"Error size mismatch for IB_IBUF_CHUNK");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->size_ibufs_chunk);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_IBUF_CHUNK");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_IBUF_CHUNK");
			abort();
		}
#endif
				obj->init_mr = 400;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_MMU_INIT" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->init_mr ) )
			{
				fprintf(stderr,"Error size mismatch for IB_MMU_INIT");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->init_mr);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_MMU_INIT");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_MMU_INIT");
			abort();
		}
#endif
				obj->steal = 2;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_CAN_STEAL" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->steal ) )
			{
				fprintf(stderr,"Error size mismatch for IB_CAN_STEAL");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->steal);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_CAN_STEAL");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_CAN_STEAL");
			abort();
		}
#endif
				obj->quiet_crash = 0;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_QUIET_CRASH" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->quiet_crash ) )
			{
				fprintf(stderr,"Error size mismatch for IB_QUIET_CRASH");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->quiet_crash);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_QUIET_CRASH");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_QUIET_CRASH");
			abort();
		}
#endif
				obj->async_thread = 0;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_ASYNC_THREAD" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->async_thread ) )
			{
				fprintf(stderr,"Error size mismatch for IB_ASYNC_THREAD");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->async_thread);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_ASYNC_THREAD");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_ASYNC_THREAD");
			abort();
		}
#endif
				obj->wc_in_number = 0;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_WC_IN" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->wc_in_number ) )
			{
				fprintf(stderr,"Error size mismatch for IB_WC_IN");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->wc_in_number);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_WC_IN");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_WC_IN");
			abort();
		}
#endif
				obj->wc_out_number = 0;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_WC_OUT" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->wc_out_number ) )
			{
				fprintf(stderr,"Error size mismatch for IB_WC_OUT");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->wc_out_number);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_WC_OUT");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_WC_OUT");
			abort();
		}
#endif
				obj->low_memory = false;
	obj->rdvz_protocol = IBV_RDVZ_WRITE_PROTOCOL;
	obj->rdma_min_size = 1024;
	obj->rdma_max_size = 4096;
	obj->rdma_min_nb = 8;
	obj->rdma_max_nb = 32;
	obj->rdma_resizing_min_size = 1024;
	obj->rdma_resizing_max_size = 4096;
	obj->rdma_resizing_min_nb = 8;
	obj->rdma_resizing_max_nb = 32;
	obj->size_recv_ibufs_chunk = 400;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_RECV_IBUF_CHUNK" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->size_recv_ibufs_chunk ) )
			{
				fprintf(stderr,"Error size mismatch for IB_RECV_IBUF_CHUNK");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->size_recv_ibufs_chunk);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_RECV_IBUF_CHUNK");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_RECV_IBUF_CHUNK");
			abort();
		}
#endif
				obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_ib_global(void * struct_ptr)
{
	struct sctk_runtime_config_struct_ib_global * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->mmu_cache_enabled = 1;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_MMU_CHACHE_ENABLED" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->mmu_cache_enabled ) )
			{
				fprintf(stderr,"Error size mismatch for IB_MMU_CHACHE_ENABLED");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->mmu_cache_enabled);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_MMU_CHACHE_ENABLED");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_MMU_CHACHE_ENABLED");
			abort();
		}
#endif
				obj->mmu_cache_entry_count = 1000;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_MMU_CHACHE_COUNT" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->mmu_cache_entry_count ) )
			{
				fprintf(stderr,"Error size mismatch for IB_MMU_CHACHE_COUNT");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->mmu_cache_entry_count);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_MMU_CHACHE_COUNT");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_MMU_CHACHE_COUNT");
			abort();
		}
#endif
				obj->mmu_cache_maximum_size = sctk_runtime_config_map_entry_parse_size("4GB");

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_MMU_CHACHE_MAX_SIZE" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->mmu_cache_maximum_size ) )
			{
				fprintf(stderr,"Error size mismatch for IB_MMU_CHACHE_MAX_SIZE");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->mmu_cache_maximum_size);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_MMU_CHACHE_MAX_SIZE");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_MMU_CHACHE_MAX_SIZE");
			abort();
		}
#endif
				obj->mmu_cache_maximum_pin_size = sctk_runtime_config_map_entry_parse_size("1GB");

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "IB_MMU_CHACHE_MAX_PIN_SIZE" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->mmu_cache_maximum_pin_size ) )
			{
				fprintf(stderr,"Error size mismatch for IB_MMU_CHACHE_MAX_PIN_SIZE");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->mmu_cache_maximum_pin_size);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for IB_MMU_CHACHE_MAX_PIN_SIZE");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for IB_MMU_CHACHE_MAX_PIN_SIZE");
			abort();
		}
#endif
				obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_enum_init_ibv_rdvz_protocol()
{
	struct enum_type * current_enum = (struct enum_type *) malloc(sizeof(struct enum_type));
	struct enum_value * current_value, * values = NULL;

	strncpy(current_enum->name, "enum ibv_rdvz_protocol", 50);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "IBV_RDVZ_WRITE_PROTOCOL", 50);
	current_value->value = IBV_RDVZ_WRITE_PROTOCOL;
	HASH_ADD_STR(values, name, current_value);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "IBV_RDVZ_READ_PROTOCOL", 50);
	current_value->value = IBV_RDVZ_READ_PROTOCOL;
	HASH_ADD_STR(values, name, current_value);

	current_enum->values = values;
	HASH_ADD_STR(enums_types, name, current_enum);
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_net_driver_portals(void * struct_ptr)
{
	struct sctk_runtime_config_struct_net_driver_portals * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->eager_limit = sctk_runtime_config_map_entry_parse_size("8 KB");
	obj->min_comms = 1;
	obj->block_cut = sctk_runtime_config_map_entry_parse_size("2 GB");
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_net_driver_tcp(void * struct_ptr)
{
	struct sctk_runtime_config_struct_net_driver_tcp * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->tcpoib = 1;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_net_driver_tcp_rdma(void * struct_ptr)
{
	struct sctk_runtime_config_struct_net_driver_tcp_rdma * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->tcpoib = 1;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_net_driver_shm(void * struct_ptr)
{
	struct sctk_runtime_config_struct_net_driver_shm * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->buffered_priority = 0;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "SHM_BUFF_PRIORITY" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->buffered_priority ) )
			{
				fprintf(stderr,"Error size mismatch for SHM_BUFF_PRIORITY");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->buffered_priority);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for SHM_BUFF_PRIORITY");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for SHM_BUFF_PRIORITY");
			abort();
		}
#endif
				obj->buffered_min_size = 0;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "SHM_BUFF_MIN_SIZE" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->buffered_min_size ) )
			{
				fprintf(stderr,"Error size mismatch for SHM_BUFF_MIN_SIZE");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->buffered_min_size);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for SHM_BUFF_MIN_SIZE");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for SHM_BUFF_MIN_SIZE");
			abort();
		}
#endif
				obj->buffered_max_size = 4096;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "SHM_BUFF_MAX_SIZE" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->buffered_max_size ) )
			{
				fprintf(stderr,"Error size mismatch for SHM_BUFF_MAX_SIZE");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->buffered_max_size);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for SHM_BUFF_MAX_SIZE");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for SHM_BUFF_MAX_SIZE");
			abort();
		}
#endif
				obj->buffered_zerocopy = false;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "SHM_BUFF_ZEROCOPY" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->buffered_zerocopy ) )
			{
				fprintf(stderr,"Error size mismatch for SHM_BUFF_ZEROCOPY");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->buffered_zerocopy);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for SHM_BUFF_ZEROCOPY");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for SHM_BUFF_ZEROCOPY");
			abort();
		}
#endif
				obj->cma_enable = true;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "SHM_CMA_ENABLE" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->cma_enable ) )
			{
				fprintf(stderr,"Error size mismatch for SHM_CMA_ENABLE");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->cma_enable);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for SHM_CMA_ENABLE");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for SHM_CMA_ENABLE");
			abort();
		}
#endif
				obj->cma_priority = 1;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "SHM_CMA_PRIORITY" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->cma_priority ) )
			{
				fprintf(stderr,"Error size mismatch for SHM_CMA_PRIORITY");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->cma_priority);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for SHM_CMA_PRIORITY");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for SHM_CMA_PRIORITY");
			abort();
		}
#endif
				obj->cma_min_size = 4096;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "SHM_CMA_MIN_SIZE" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->cma_min_size ) )
			{
				fprintf(stderr,"Error size mismatch for SHM_CMA_MIN_SIZE");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->cma_min_size);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for SHM_CMA_MIN_SIZE");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for SHM_CMA_MIN_SIZE");
			abort();
		}
#endif
				obj->cma_max_size = 0;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "SHM_CMA_MAX_SIZE" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->cma_max_size ) )
			{
				fprintf(stderr,"Error size mismatch for SHM_CMA_MAX_SIZE");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->cma_max_size);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for SHM_CMA_MAX_SIZE");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for SHM_CMA_MAX_SIZE");
			abort();
		}
#endif
				obj->cma_zerocopy = false;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "SHM_CMA_ZEROCOPY" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->cma_zerocopy ) )
			{
				fprintf(stderr,"Error size mismatch for SHM_CMA_ZEROCOPY");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->cma_zerocopy);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for SHM_CMA_ZEROCOPY");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for SHM_CMA_ZEROCOPY");
			abort();
		}
#endif
				obj->frag_priority = 2;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "SHM_FRAG_PRIORITY" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->frag_priority ) )
			{
				fprintf(stderr,"Error size mismatch for SHM_FRAG_PRIORITY");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->frag_priority);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for SHM_FRAG_PRIORITY");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for SHM_FRAG_PRIORITY");
			abort();
		}
#endif
				obj->frag_min_size = 4096;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "SHM_FRAG_MIN_SIZE" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->frag_min_size ) )
			{
				fprintf(stderr,"Error size mismatch for SHM_FRAG_MIN_SIZE");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->frag_min_size);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for SHM_FRAG_MIN_SIZE");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for SHM_FRAG_MIN_SIZE");
			abort();
		}
#endif
				obj->frag_max_size = 0;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "SHM_FRAG_MAX_SIZE" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->frag_max_size ) )
			{
				fprintf(stderr,"Error size mismatch for SHM_FRAG_MAX_SIZE");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->frag_max_size);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for SHM_FRAG_MAX_SIZE");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for SHM_FRAG_MAX_SIZE");
			abort();
		}
#endif
				obj->frag_zerocopy = false;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "SHM_FRAG_ZEROCOPY" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->frag_zerocopy ) )
			{
				fprintf(stderr,"Error size mismatch for SHM_FRAG_ZEROCOPY");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->frag_zerocopy);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for SHM_FRAG_ZEROCOPY");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for SHM_FRAG_ZEROCOPY");
			abort();
		}
#endif
				obj->shmem_size = 1024;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "SHM_SIZE" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->shmem_size ) )
			{
				fprintf(stderr,"Error size mismatch for SHM_SIZE");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->shmem_size);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for SHM_SIZE");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for SHM_SIZE");
			abort();
		}
#endif
				obj->cells_num = 2048;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "SHM_CELLS" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->cells_num ) )
			{
				fprintf(stderr,"Error size mismatch for SHM_CELLS");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->cells_num);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for SHM_CELLS");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for SHM_CELLS");
			abort();
		}
#endif
				obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_net_driver(void * struct_ptr)
{
	struct sctk_runtime_config_struct_net_driver * obj = struct_ptr;
	obj->type = SCTK_RTCFG_net_driver_NONE;
	memset(&obj->value,0,sizeof(obj->value));
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_net_driver_config(void * struct_ptr)
{
	struct sctk_runtime_config_struct_net_driver_config * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->name = NULL;
	sctk_runtime_config_struct_init_net_driver(&obj->driver);
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_gate_boolean(void * struct_ptr)
{
	struct sctk_runtime_config_struct_gate_boolean * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->value = 1;
	obj->gatefunc.name = "sctk_rail_gate_boolean";
	*(void **) &(obj->gatefunc.value) = sctk_runtime_config_get_symbol("sctk_rail_gate_boolean");
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_gate_probabilistic(void * struct_ptr)
{
	struct sctk_runtime_config_struct_gate_probabilistic * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->probability = 50;
	obj->gatefunc.name = "sctk_rail_gate_probabilistic";
	*(void **) &(obj->gatefunc.value) = sctk_runtime_config_get_symbol("sctk_rail_gate_probabilistic");
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_gate_min_size(void * struct_ptr)
{
	struct sctk_runtime_config_struct_gate_min_size * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->value = 0;
	obj->gatefunc.name = "sctk_rail_gate_minsize";
	*(void **) &(obj->gatefunc.value) = sctk_runtime_config_get_symbol("sctk_rail_gate_minsize");
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_gate_max_size(void * struct_ptr)
{
	struct sctk_runtime_config_struct_gate_max_size * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->value = 0;
	obj->gatefunc.name = "sctk_rail_gate_maxsize";
	*(void **) &(obj->gatefunc.value) = sctk_runtime_config_get_symbol("sctk_rail_gate_maxsize");
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_gate_message_type(void * struct_ptr)
{
	struct sctk_runtime_config_struct_gate_message_type * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->process = 1;
	obj->task = 1;
	obj->emulated_rma = 1;
	obj->common = 1;
	obj->gatefunc.name = "sctk_rail_gate_msgtype";
	*(void **) &(obj->gatefunc.value) = sctk_runtime_config_get_symbol("sctk_rail_gate_msgtype");
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_gate_user(void * struct_ptr)
{
	struct sctk_runtime_config_struct_gate_user * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->gatefunc.name = "sctk_rail_gate_true";
	*(void **) &(obj->gatefunc.value) = sctk_runtime_config_get_symbol("sctk_rail_gate_true");
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_net_gate(void * struct_ptr)
{
	struct sctk_runtime_config_struct_net_gate * obj = struct_ptr;
	obj->type = SCTK_RTCFG_net_gate_NONE;
	memset(&obj->value,0,sizeof(obj->value));
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_enum_init_rail_topological_polling_level()
{
	struct enum_type * current_enum = (struct enum_type *) malloc(sizeof(struct enum_type));
	struct enum_value * current_value, * values = NULL;

	strncpy(current_enum->name, "enum rail_topological_polling_level", 50);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "RAIL_POLL_NONE", 50);
	current_value->value = RAIL_POLL_NONE;
	HASH_ADD_STR(values, name, current_value);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "RAIL_POLL_PU", 50);
	current_value->value = RAIL_POLL_PU;
	HASH_ADD_STR(values, name, current_value);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "RAIL_POLL_CORE", 50);
	current_value->value = RAIL_POLL_CORE;
	HASH_ADD_STR(values, name, current_value);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "RAIL_POLL_SOCKET", 50);
	current_value->value = RAIL_POLL_SOCKET;
	HASH_ADD_STR(values, name, current_value);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "RAIL_POLL_NUMA", 50);
	current_value->value = RAIL_POLL_NUMA;
	HASH_ADD_STR(values, name, current_value);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "RAIL_POLL_MACHINE", 50);
	current_value->value = RAIL_POLL_MACHINE;
	HASH_ADD_STR(values, name, current_value);

	current_enum->values = values;
	HASH_ADD_STR(enums_types, name, current_enum);
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_topological_polling(void * struct_ptr)
{
	struct sctk_runtime_config_struct_topological_polling * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->range = RAIL_POLL_MACHINE;
	obj->trigger = RAIL_POLL_SOCKET;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_net_rail(void * struct_ptr)
{
	struct sctk_runtime_config_struct_net_rail * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->name = NULL;
	obj->priority = 1;
	obj->device = "default";
	sctk_runtime_config_struct_init_topological_polling(&obj->idle_polling);
	sctk_runtime_config_struct_init_topological_polling(&obj->any_source_polling);
	obj->topology = "ring";
	obj->ondemand = 1;
	obj->rdma = 0;
	obj->config = "topological";
	/* array */
	obj->gates = NULL;
	obj->gates_size = 0;
	/* array */
	obj->subrails = NULL;
	obj->subrails_size = 0;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_net_cli_option(void * struct_ptr)
{
	struct sctk_runtime_config_struct_net_cli_option * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->name = NULL;
	/* array */
	obj->rails = NULL;
	obj->rails_size = 0;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_networks(void * struct_ptr)
{
	struct sctk_runtime_config_struct_networks * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	/* array */
	obj->configs = NULL;
	obj->configs_size = 0;
	/* array */
	obj->rails = NULL;
	obj->rails_size = 0;
	/* array */
	obj->cli_options = NULL;
	obj->cli_options_size = 0;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_inter_thread_comm(void * struct_ptr)
{
	struct sctk_runtime_config_struct_inter_thread_comm * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->barrier_arity = 8;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "ITT_BARRIER_ARITY" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->barrier_arity ) )
			{
				fprintf(stderr,"Error size mismatch for ITT_BARRIER_ARITY");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->barrier_arity);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for ITT_BARRIER_ARITY");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for ITT_BARRIER_ARITY");
			abort();
		}
#endif
				obj->broadcast_arity_max = 32;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "ITT_BROADCAST_ARITY" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->broadcast_arity_max ) )
			{
				fprintf(stderr,"Error size mismatch for ITT_BROADCAST_ARITY");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->broadcast_arity_max);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for ITT_BROADCAST_ARITY");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for ITT_BROADCAST_ARITY");
			abort();
		}
#endif
				obj->broadcast_max_size = 1024;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "ITT_BROADCAST_MAX_SIZE" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->broadcast_max_size ) )
			{
				fprintf(stderr,"Error size mismatch for ITT_BROADCAST_MAX_SIZE");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->broadcast_max_size);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for ITT_BROADCAST_MAX_SIZE");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for ITT_BROADCAST_MAX_SIZE");
			abort();
		}
#endif
				obj->broadcast_check_threshold = 512;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "ITT_BROADCAST_CHECK_TH" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->broadcast_check_threshold ) )
			{
				fprintf(stderr,"Error size mismatch for ITT_BROADCAST_CHECK_TH");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->broadcast_check_threshold);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for ITT_BROADCAST_CHECK_TH");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for ITT_BROADCAST_CHECK_TH");
			abort();
		}
#endif
				obj->allreduce_arity_max = 8;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "ITT_ALLREDUCE_ARITY_MAX" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->allreduce_arity_max ) )
			{
				fprintf(stderr,"Error size mismatch for ITT_ALLREDUCE_ARITY_MAX");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->allreduce_arity_max);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for ITT_ALLREDUCE_ARITY_MAX");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for ITT_ALLREDUCE_ARITY_MAX");
			abort();
		}
#endif
				obj->allreduce_max_size = 4096;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "ITT_ALLREDUCE_MAX_SIZE" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->allreduce_max_size ) )
			{
				fprintf(stderr,"Error size mismatch for ITT_ALLREDUCE_MAX_SIZE");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->allreduce_max_size);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for ITT_ALLREDUCE_MAX_SIZE");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for ITT_ALLREDUCE_MAX_SIZE");
			abort();
		}
#endif
				obj->allreduce_check_threshold = 8192;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "ITT_ALLREDUCE_CHECK_TH" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->allreduce_check_threshold ) )
			{
				fprintf(stderr,"Error size mismatch for ITT_ALLREDUCE_CHECK_TH");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->allreduce_check_threshold);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for ITT_ALLREDUCE_CHECK_TH");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for ITT_ALLREDUCE_CHECK_TH");
			abort();
		}
#endif
				obj->ALLREDUCE_MAX_SLOT = 65536;

#ifdef MPC_MPI
		if( mpc_MPI_T_cvar_get_index( "ITT_ALLREDUCE_MAX_SLOT" , &the_temp_index ) == MPI_SUCCESS )
		{
			the_cvar = MPI_T_cvars_array_get( the_temp_index );

			
			if( MPC_T_data_get_size( &the_cvar->data ) != sizeof( 
			obj->ALLREDUCE_MAX_SLOT ) )
			{
				fprintf(stderr,"Error size mismatch for ITT_ALLREDUCE_MAX_SLOT");
				abort();	
			}

			if( the_cvar )
			{
									MPC_T_data_alias(&the_cvar->data, &obj->ALLREDUCE_MAX_SLOT);
	
			}
			else
			{
				fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for ITT_ALLREDUCE_MAX_SLOT");
				abort();
			}
		
		}
		else
		{
			fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for ITT_ALLREDUCE_MAX_SLOT");
			abort();
		}
#endif
				obj->collectives_init_hook.name = "sctk_collectives_init_opt_noalloc_split_messages";
	*(void **) &(obj->collectives_init_hook.value) = sctk_runtime_config_get_symbol("sctk_collectives_init_opt_noalloc_split_messages");
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_low_level_comm(void * struct_ptr)
{
	struct sctk_runtime_config_struct_low_level_comm * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->checksum = true;
	obj->send_msg.name = "sctk_network_send_message_default";
	*(void **) &(obj->send_msg.value) = sctk_runtime_config_get_symbol("sctk_network_send_message_default");
	obj->network_mode = "default";
	obj->dyn_reordering = false;
	obj->enable_idle_polling = false;
	sctk_runtime_config_struct_init_ib_global(&obj->ib_global);
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_enum_init_mpcomp_task_larceny_mode_t()
{
	struct enum_type * current_enum = (struct enum_type *) malloc(sizeof(struct enum_type));
	struct enum_value * current_value, * values = NULL;

	strncpy(current_enum->name, "enum mpcomp_task_larceny_mode_t", 50);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "MPCOMP_TASK_LARCENY_MODE_HIERARCHICAL", 50);
	current_value->value = MPCOMP_TASK_LARCENY_MODE_HIERARCHICAL;
	HASH_ADD_STR(values, name, current_value);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "MPCOMP_TASK_LARCENY_MODE_RANDOM", 50);
	current_value->value = MPCOMP_TASK_LARCENY_MODE_RANDOM;
	HASH_ADD_STR(values, name, current_value);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "MPCOMP_TASK_LARCENY_MODE_RANDOM_ORDER", 50);
	current_value->value = MPCOMP_TASK_LARCENY_MODE_RANDOM_ORDER;
	HASH_ADD_STR(values, name, current_value);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "MPCOMP_TASK_LARCENY_MODE_ROUNDROBIN", 50);
	current_value->value = MPCOMP_TASK_LARCENY_MODE_ROUNDROBIN;
	HASH_ADD_STR(values, name, current_value);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "MPCOMP_TASK_LARCENY_MODE_PRODUCER", 50);
	current_value->value = MPCOMP_TASK_LARCENY_MODE_PRODUCER;
	HASH_ADD_STR(values, name, current_value);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "MPCOMP_TASK_LARCENY_MODE_PRODUCER_ORDER", 50);
	current_value->value = MPCOMP_TASK_LARCENY_MODE_PRODUCER_ORDER;
	HASH_ADD_STR(values, name, current_value);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "MPCOMP_TASK_LARCENY_MODE_HIERARCHICAL_RANDOM", 50);
	current_value->value = MPCOMP_TASK_LARCENY_MODE_HIERARCHICAL_RANDOM;
	HASH_ADD_STR(values, name, current_value);

	current_value = (struct enum_value *) malloc(sizeof(struct enum_value));
	strncpy(current_value->name, "MPCOMP_TASK_LARCENY_MODE_COUNT", 50);
	current_value->value = MPCOMP_TASK_LARCENY_MODE_COUNT;
	HASH_ADD_STR(values, name, current_value);

	current_enum->values = values;
	HASH_ADD_STR(enums_types, name, current_enum);
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_openmp(void * struct_ptr)
{
	struct sctk_runtime_config_struct_openmp * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->vp = 0;
	obj->schedule = "static";
	obj->nb_threads = 0;
	obj->adjustment = false;
	obj->proc_bind = true;
	obj->nested = false;
	obj->stack_size = 0;
	obj->wait_policy = 0;
	obj->thread_limit = 0;
	obj->max_active_levels = 0;
	obj->tree = "";
	obj->max_threads = 64;
	obj->max_alive_for_dyn = 7;
	obj->max_alive_for_guided = 3;
	obj->max_alive_sections = 3;
	obj->max_alive_single = 3;
	obj->warn_nested = false;
	obj->mode = "simple-mixed";
	obj->affinity = "balanced";
	obj->omp_new_task_depth = 10;
	obj->omp_untied_task_depth = 10;
	obj->omp_task_larceny_mode = MPCOMP_TASK_LARCENY_MODE_PRODUCER;
	obj->omp_task_nesting_max = 1000000;
	obj->mpcomp_task_max_delayed = 1024;
	obj->omp_task_steal_last_stolen_list = false;
	obj->omp_task_resteal_to_last_thief = false;
	obj->omp_task_use_lockfree_queue = true;
	obj->places = "cores";
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_profiler(void * struct_ptr)
{
	struct sctk_runtime_config_struct_profiler * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->file_prefix = "mpc_profile";
	obj->append_date = true;
	obj->color_stdout = true;
	/* array */
	obj->level_colors = calloc(6,sizeof(char *));
	obj->level_colors[0] = "#3A4D85";
	obj->level_colors[1] = "#82A2FF";
	obj->level_colors[2] = "#B8BDCB";
	obj->level_colors[3] = "#5D6782";
	obj->level_colors[4] = "#838383";
	obj->level_colors[5] = "#5A5757";
	obj->level_colors_size = 6;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_thread(void * struct_ptr)
{
	struct sctk_runtime_config_struct_thread * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->spin_delay = 10;
	obj->interval = 10;
	obj->kthread_stack_size = sctk_runtime_config_map_entry_parse_size("10MB");
	obj->placement_policy.name = "sctk_get_init_vp_and_nbvp_default";
	*(void **) &(obj->placement_policy.value) = sctk_runtime_config_get_symbol("sctk_get_init_vp_and_nbvp_default");
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_struct_init_scheduler(void * struct_ptr)
{
	struct sctk_runtime_config_struct_scheduler * obj = struct_ptr;
	/* Make sure this element is not initialized yet       */
	/* It allows us to know when we are facing dynamically */
	/* allocated objects requiring an init                 */
	if( obj->init_done != 0 ) return;

	/* Simple params : */
	obj->timestamp_threshold = 0.0;
	obj->task_polling_thread_basic_priority = 20;
	obj->task_polling_thread_basic_priority_step = 20;
	obj->task_polling_thread_current_priority_step = 20;
	obj->sched_NBC_Pthread_basic_priority = 20;
	obj->sched_NBC_Pthread_basic_priority_step = 20;
	obj->sched_NBC_Pthread_current_priority_step = 20;
	obj->mpi_basic_priority = 20;
	obj->omp_basic_priority = 20;
	obj->posix_basic_priority = 20;
	obj->progress_basic_priority = 20;
	obj->init_done = 1;
}

/*******************  FUNCTION  *********************/
void sctk_runtime_config_reset(struct sctk_runtime_config * config)
{
	memset(config, 0, sizeof(struct sctk_runtime_config));
	sctk_handler = dlopen(0, RTLD_LAZY | RTLD_GLOBAL);
#ifdef MPC_Accelerators
	sctk_runtime_config_struct_init_accl(&config->modules.accelerator);
#endif
#ifdef MPC_Allocator
	sctk_runtime_config_struct_init_allocator(&config->modules.allocator);
#endif
#ifdef MPC_Common
	sctk_runtime_config_struct_init_launcher(&config->modules.launcher);
	sctk_runtime_config_struct_init_debugger(&config->modules.debugger);
#endif
#ifdef MPC_Fault_Tolerance
	sctk_runtime_config_struct_init_ft(&config->modules.ft_system);
#endif
#ifdef MPC_MPI
	sctk_runtime_config_struct_init_collectives_shm_shared(&config->modules.collectives_shm_shared);
	sctk_runtime_config_struct_init_collectives_shm(&config->modules.collectives_shm);
	sctk_runtime_config_struct_init_collectives_intra(&config->modules.collectives_intra);
	sctk_runtime_config_struct_init_collectives_inter(&config->modules.collectives_inter);
	sctk_runtime_config_struct_init_nbc(&config->modules.nbc);
	sctk_runtime_config_struct_init_mpc(&config->modules.mpc);
	sctk_runtime_config_struct_init_mpi_rma(&config->modules.rma);
#endif
#ifdef MPC_Message_Passing
	sctk_runtime_config_struct_init_inter_thread_comm(&config->modules.inter_thread_comm);
	sctk_runtime_config_struct_init_low_level_comm(&config->modules.low_level_comm);
	sctk_runtime_config_enum_init_ibv_rdvz_protocol();
	sctk_runtime_config_enum_init_rail_topological_polling_level();
#endif
#ifdef MPC_OpenMP
	sctk_runtime_config_struct_init_openmp(&config->modules.openmp);
	sctk_runtime_config_enum_init_mpcomp_task_larceny_mode_t();
#endif
#ifdef MPC_Profiler
	sctk_runtime_config_struct_init_profiler(&config->modules.profiler);
#endif
#ifdef MPC_Threads
	sctk_runtime_config_struct_init_thread(&config->modules.thread);
	sctk_runtime_config_struct_init_scheduler(&config->modules.scheduler);
#endif
	sctk_runtime_config_struct_init_networks(&config->networks);
	dlclose(sctk_handler);
}


/*******************  FUNCTION  *********************/
void sctk_runtime_config_clean_hash_tables()
{
	struct enum_type * current_enum, * tmp_enum;
	struct enum_value * current_value, * tmp_value;

	HASH_ITER(hh, enums_types, current_enum, tmp_enum) {
		HASH_ITER(hh, current_enum->values, current_value, tmp_value) {
			HASH_DEL(current_enum->values, current_value);
			free(current_value);
		}
		HASH_DEL(enums_types, current_enum);
		free(current_enum);
	}
}


/*******************  FUNCTION  *********************/
void sctk_runtime_config_reset_struct_default_if_needed(const char * structname, void * ptr )
{
	if( !strcmp( structname , "sctk_runtime_config_struct_accl_cuda") )
	{
		sctk_runtime_config_struct_init_accl_cuda( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_accl_openacc") )
	{
		sctk_runtime_config_struct_init_accl_openacc( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_accl_opencl") )
	{
		sctk_runtime_config_struct_init_accl_opencl( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_accl") )
	{
		sctk_runtime_config_struct_init_accl( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_allocator") )
	{
		sctk_runtime_config_struct_init_allocator( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_launcher") )
	{
		sctk_runtime_config_struct_init_launcher( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_debugger") )
	{
		sctk_runtime_config_struct_init_debugger( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_ft") )
	{
		sctk_runtime_config_struct_init_ft( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_collectives_shm_shared") )
	{
		sctk_runtime_config_struct_init_collectives_shm_shared( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_collectives_shm") )
	{
		sctk_runtime_config_struct_init_collectives_shm( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_collectives_intra") )
	{
		sctk_runtime_config_struct_init_collectives_intra( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_collectives_inter") )
	{
		sctk_runtime_config_struct_init_collectives_inter( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_nbc") )
	{
		sctk_runtime_config_struct_init_nbc( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_mpi_rma") )
	{
		sctk_runtime_config_struct_init_mpi_rma( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_mpc") )
	{
		sctk_runtime_config_struct_init_mpc( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_net_driver_topological") )
	{
		sctk_runtime_config_struct_init_net_driver_topological( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_net_driver_infiniband") )
	{
		sctk_runtime_config_struct_init_net_driver_infiniband( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_ib_global") )
	{
		sctk_runtime_config_struct_init_ib_global( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_net_driver_portals") )
	{
		sctk_runtime_config_struct_init_net_driver_portals( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_net_driver_tcp") )
	{
		sctk_runtime_config_struct_init_net_driver_tcp( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_net_driver_tcp_rdma") )
	{
		sctk_runtime_config_struct_init_net_driver_tcp_rdma( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_net_driver_shm") )
	{
		sctk_runtime_config_struct_init_net_driver_shm( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_net_driver_config") )
	{
		sctk_runtime_config_struct_init_net_driver_config( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_gate_boolean") )
	{
		sctk_runtime_config_struct_init_gate_boolean( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_gate_probabilistic") )
	{
		sctk_runtime_config_struct_init_gate_probabilistic( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_gate_min_size") )
	{
		sctk_runtime_config_struct_init_gate_min_size( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_gate_max_size") )
	{
		sctk_runtime_config_struct_init_gate_max_size( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_gate_message_type") )
	{
		sctk_runtime_config_struct_init_gate_message_type( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_gate_user") )
	{
		sctk_runtime_config_struct_init_gate_user( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_topological_polling") )
	{
		sctk_runtime_config_struct_init_topological_polling( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_net_rail") )
	{
		sctk_runtime_config_struct_init_net_rail( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_net_cli_option") )
	{
		sctk_runtime_config_struct_init_net_cli_option( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_networks") )
	{
		sctk_runtime_config_struct_init_networks( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_inter_thread_comm") )
	{
		sctk_runtime_config_struct_init_inter_thread_comm( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_low_level_comm") )
	{
		sctk_runtime_config_struct_init_low_level_comm( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_openmp") )
	{
		sctk_runtime_config_struct_init_openmp( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_profiler") )
	{
		sctk_runtime_config_struct_init_profiler( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_thread") )
	{
		sctk_runtime_config_struct_init_thread( ptr );
		return;
	}

	if( !strcmp( structname , "sctk_runtime_config_struct_scheduler") )
	{
		sctk_runtime_config_struct_init_scheduler( ptr );
		return;
	}

}


/*******************  FUNCTION  *********************/
void * sctk_runtime_config_get_union_value_offset(const char * unionname, void * ptr )
{
	if( !strcmp( unionname , "sctk_runtime_config_struct_net_driver") )
	{
		struct sctk_runtime_config_struct_net_driver	* obj_net_driver = ptr;
		return &(obj_net_driver->value);
	}

	if( !strcmp( unionname , "sctk_runtime_config_struct_net_gate") )
	{
		struct sctk_runtime_config_struct_net_gate	* obj_net_gate = ptr;
		return &(obj_net_gate->value);
	}

	/* If not found assume sizeof (int) */
	return (char *)ptr + sizeof(int);
}

