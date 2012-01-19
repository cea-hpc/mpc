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
/* #                                                                      # */
/* ######################################################################## */
#include "sctk_communicator.h"
#include "sctk_thread.h"
#include "sctk_alloc.h"
#include "sctk_inter_thread_comm.h"
#include "sctk_topology.h"
#include "sctk_low_level_comm.h"
#include "sctk_hybrid_comm.h"
#include "sctk_accessor.h"

#define SCTK_LOCAL_VERSION_MAJOR 0
#define SCTK_LOCAL_VERSION_MINOR 1

static int sctk_total_task_number = 0;
static sctk_alloc_thread_data_t *sctk_communicator_storage = NULL;
static sctk_thread_mutex_t sctk_global_communicator_number_lock =
  SCTK_THREAD_MUTEX_INITIALIZER;

static sctk_internal_communicator_t *volatile
  sctk_communicator_list[SCTK_MAX_COMMUNICATOR_NUMBER];

sctk_internal_communicator_t *
sctk_get_communicator (const sctk_communicator_t com_id)
{
  return sctk_communicator_list[com_id];
}

void sctk_init_comm_world(int sctk_total_task_number);
void
sctk_communicator_init (const int nb_task)
{
  int i;
  sctk_internal_communicator_t *tmp;
  int *rank_list;
  int *glob_list;
  sctk_total_task_number = nb_task;
  sctk_only_once ();
  sctk_print_version ("Communicators", SCTK_LOCAL_VERSION_MAJOR,
		      SCTK_LOCAL_VERSION_MINOR);

  sctk_collective_communication_init_mem ();

  sctk_communicator_storage = __sctk_create_thread_memory_area ();
  for (i = 0; i < SCTK_MAX_COMMUNICATOR_NUMBER; i++)
    {
      sctk_communicator_list[i] = NULL;
    }

  /*INIT COMM_WORLD */
  tmp = (sctk_internal_communicator_t *)
    __sctk_malloc (sctk_aligned_sizeof (sctk_internal_communicator_t)
		   /*  + */
		   /*                2*sctk_aligned_size (sctk_total_task_number * sizeof (int)) */
		   , sctk_communicator_storage);

  rank_list = NULL;
  glob_list = NULL;

  tmp->communicator_number = SCTK_COMM_WORLD;
  tmp->origin_communicator = SCTK_COMM_EMPTY;
  tmp->new_communicator = -1;
  tmp->nb_task_involved = nb_task;
  tmp->nb_task_registered = nb_task;

  tmp->rank_in_communicator = rank_list;

  tmp->local_communicator_size = nb_task;
  tmp->global_in_communicator_local = glob_list;
  tmp->remote_communicator_size = nb_task;
  tmp->global_in_communicator_remote = glob_list;

  sctk_thread_mutex_init (&tmp->lock, NULL);
  tmp->collective_communications =
    sctk_collective_communications_init (nb_task);
  sctk_communicator_list[SCTK_COMM_WORLD] = tmp;
  tmp->is_inter_comm = 0;

  /*INIT COMM_SELF */
  tmp = (sctk_internal_communicator_t *)
    __sctk_malloc (sctk_aligned_sizeof (sctk_internal_communicator_t)
		   /*  + */
		   /*                2*sctk_aligned_size (sctk_total_task_number * sizeof (int)) */
		   , sctk_communicator_storage);

  rank_list = NULL;
  glob_list = NULL;

  tmp->communicator_number = SCTK_COMM_SELF;
  tmp->origin_communicator = SCTK_COMM_EMPTY;
  tmp->new_communicator = -1;
  tmp->nb_task_involved = nb_task;
  tmp->nb_task_registered = nb_task;

  tmp->rank_in_communicator = rank_list;

  tmp->local_communicator_size = nb_task;
  tmp->global_in_communicator_local = glob_list;
  tmp->remote_communicator_size = nb_task;
  tmp->global_in_communicator_remote = glob_list;

  sctk_thread_mutex_init (&tmp->lock, NULL);
  tmp->collective_communications =
    sctk_collective_communications_init (nb_task);
  sctk_communicator_list[SCTK_COMM_SELF] = tmp;
  tmp->is_inter_comm = 0;


  sctk_ptp_init (nb_task);

  sctk_init_comm_world(nb_task);
}

void
sctk_communicator_delete ()
{
  int i;

  for (i = 0; i < SCTK_MAX_COMMUNICATOR_NUMBER; i++)
    {
      if (sctk_communicator_list[i] != NULL)
	{
	  sctk_collective_communications_delete (sctk_communicator_list
						 [i]->
						 collective_communications);
	  sctk_free (sctk_communicator_list[i]);
	}
    }
  sctk_ptp_delete ();
}

void
sctk_get_free_communicator_on_root (const sctk_communicator_t
				    origin_communicator)
{
  int i;
  int done = 0;
  assume (sctk_process_rank == 0);
  sctk_thread_mutex_lock (&sctk_global_communicator_number_lock);
  if (sctk_communicator_list[origin_communicator]->new_communicator == (sctk_communicator_t)-1)
    {
      char name[2048];
      FILE *file;
      for (i = 0; i < SCTK_MAX_COMMUNICATOR_NUMBER; i++)
	{
	  if (sctk_communicator_list[i] == NULL)
	    {
	      sctk_nodebug ("found rank = %d, origin_communicator %d",
			    i, origin_communicator);
	      sctk_communicator_list[origin_communicator]->
		new_communicator = i;
	      done = 1;
	      break;
	    }
	}
      sprintf (name, "%s/communicators", sctk_store_dir);
      sctk_nodebug ("Open %s", name);
      file = fopen (name, "a");
      assume (file != NULL);
      fprintf (file, "C %d\n", i);
      fclose (file);
    }
  sctk_thread_mutex_unlock (&sctk_global_communicator_number_lock);
  assume (sctk_communicator_list[origin_communicator]->new_communicator !=
	  (sctk_communicator_t)-1);

  if ((done == 1) && (sctk_process_number != 1))
    {
      /*Broadcast result to all processes */
      sctk_net_set_free_communicator
	(origin_communicator,
	 sctk_communicator_list[origin_communicator]->new_communicator);
    }
}

void
sctk_get_free_communicator_on_root_no_rpc (const sctk_communicator_t
					   origin_communicator, int rank)
{
  int i;
  int done = 0;
  assume (sctk_process_rank == 0);
  sctk_thread_mutex_lock (&sctk_global_communicator_number_lock);
  if (sctk_communicator_list[origin_communicator]->new_communicator == (sctk_communicator_t)-1)
    {
      for (i = 0; i < SCTK_MAX_COMMUNICATOR_NUMBER; i++)
	{
	  if (sctk_communicator_list[i] == NULL)
	    {
	      sctk_nodebug ("found rank = %d, origin_communicator %d root process %d",
			  i, origin_communicator,rank);
	      sctk_communicator_list[origin_communicator]->
		new_communicator = i;
	      done = 1;
	      break;
	    }
        }
#warning LOG deactivated ( Comm_dup problem )
/*
      sprintf (name, "%s/communicators", sctk_store_dir);
      sctk_nodebug ("Open %s", name);
      file = fopen (name, "a");
      assume (file != NULL);
      fprintf (file, "C %d\n", i);
      fclose (file);
*/
    }
  sctk_thread_mutex_unlock (&sctk_global_communicator_number_lock);
  assume (sctk_communicator_list[origin_communicator]->new_communicator !=
	  (sctk_communicator_t)-1);
  if (sctk_process_rank != rank)
    {
      sctk_net_set_free_communicator_one
	(origin_communicator,
	 sctk_communicator_list[origin_communicator]->new_communicator,rank);
    }
}

  sctk_internal_communicator_t*
sctk_update_new_communicator (const sctk_communicator_t origin_communicator,
			      const int nb_task_involved,
			      const int *task_list)
{
  int i;
  sctk_internal_communicator_t *tmp = NULL;
  int pos;
  int *rank_list;
  int *glob_list;
  int no_rank_list = 0;

  sctk_thread_mutex_lock (&sctk_global_communicator_number_lock);
  pos = sctk_communicator_list[origin_communicator]->new_communicator;
  sctk_nodebug("COM:%d, position:%d", origin_communicator, pos);
  tmp = sctk_communicator_list[pos];
  if (tmp == NULL)
    {

/*       { */
/* 	int w;  */
/* 	for( w = 0; w < nb_task_involved; w++){ */
/* 	  sctk_debug("recv %d = %d",w,task_list[w]); */
/* 	} */
/*       } */

      if (nb_task_involved == sctk_total_task_number)
	{
	  no_rank_list = 1;
	  for (i = 0; i < nb_task_involved; i++)
	    {
	      if (task_list[i] != i)
		{
		  no_rank_list = 0;
		  break;
		}
	    }
	}

      if (no_rank_list == 0)
	{
	  tmp = (sctk_internal_communicator_t *)
	    __sctk_malloc (sctk_aligned_sizeof
			   (sctk_internal_communicator_t)
			   +
			   2 *
			   sctk_aligned_size (sctk_total_task_number *
					      sizeof (int)),
			   sctk_communicator_storage);
	}
      else
	{
	  tmp = (sctk_internal_communicator_t *)
	    __sctk_malloc (sctk_aligned_sizeof
			   (sctk_internal_communicator_t),
			   sctk_communicator_storage);
	}

      sctk_communicator_list[pos] = tmp;
      if (no_rank_list == 0)
	{
	  rank_list =
	    (int *) ((char *) tmp +
		     sctk_aligned_sizeof (sctk_internal_communicator_t));
	  for (i = 0; i < sctk_total_task_number; i++)
	    {
	      rank_list[i] = -1;
	    }
	  glob_list =
	    (int *) ((char *) tmp +
		     sctk_aligned_sizeof (sctk_internal_communicator_t)
		     +
		     sctk_aligned_size (sctk_total_task_number *
					sizeof (int)));
	  for (i = 0; i < sctk_total_task_number; i++)
	    {
	      glob_list[i] = -1;
	    }
	}
      else
	{
	  rank_list = NULL;
	  glob_list = NULL;
	}

      tmp->communicator_number = pos;
      tmp->origin_communicator = origin_communicator;
      tmp->new_communicator = -1;
      tmp->nb_task_involved = nb_task_involved;
      tmp->nb_task_registered = 0;

      tmp->rank_in_communicator = rank_list;

      tmp->local_communicator_size = nb_task_involved;
      tmp->global_in_communicator_local = glob_list;
      tmp->remote_communicator_size = nb_task_involved;
      tmp->global_in_communicator_remote = glob_list;

      sctk_thread_mutex_init (&tmp->lock, NULL);
      tmp->collective_communications =
	sctk_collective_communications_create (nb_task_involved);

      if (no_rank_list == 0)
	{
	  /*Update new rank list */
	  for (i = 0; i < nb_task_involved; i++)
	    {
	      int j;
	      for (j = 0; j < sctk_total_task_number; j++)
		{
		  int val;
		  if (sctk_communicator_list[origin_communicator]->
		      rank_in_communicator != NULL)
		    {
		      val = sctk_communicator_list[origin_communicator]->
			rank_in_communicator[j];
		    }
		  else
		    {
		      val = j;
		    }
		  if (val == task_list[i])
		    {
		      rank_list[j] = i;
		      break;
		    }
		}
	    }


	  for (i = 0; i < sctk_total_task_number; i++)
	    {
	      int val;
	      if (sctk_communicator_list[origin_communicator]->
		  global_in_communicator_local != NULL)
		{
		  if (rank_list[i] != -1)
		    val = sctk_communicator_list[origin_communicator]->
		      global_in_communicator_local[task_list[rank_list[i]]];
		  else
		    val = -1;
		}
	      else
		{
		  if (rank_list[i] != -1)
		    val = task_list[rank_list[i]];
		  else
		    val = -1;
		}
	      sctk_nodebug ("%d rank_list[i]%d", val, rank_list[i]);
	      if (rank_list[i] != -1)
		{
		  sctk_nodebug ("%d -> %d", val, i);
		  glob_list[rank_list[i]] = val;
		}
	    }

	}
    }

  tmp->nb_task_registered++;
  if ((int) tmp->nb_task_registered == (int) tmp->nb_task_involved)
    {
      sctk_communicator_list[origin_communicator]->new_communicator = -1;
    }
  sctk_nodebug ("new comm number = %d", tmp->communicator_number);
  sctk_nodebug ("%d done of %d", (int) tmp->nb_task_registered,
		(int) tmp->nb_task_involved);
  sctk_thread_mutex_unlock (&sctk_global_communicator_number_lock);

  return tmp;
}

static void
sctk_update_new_communicator_all_dup (const sctk_communicator_t
				      origin_communicator,const sctk_communicator_t dest_comm)
{
  sctk_internal_communicator_t *tmp = NULL;
  sctk_internal_communicator_t *orig = NULL;
  int nb_task_involved;
  int pos;
  int *rank_list;
  int *glob_list;
  int i;

  sctk_thread_mutex_lock (&sctk_global_communicator_number_lock);
  tmp = sctk_communicator_list[dest_comm];
  if(tmp == NULL){
    orig = sctk_communicator_list[origin_communicator];

    nb_task_involved = orig->nb_task_involved;
    pos = dest_comm;

    if(orig->rank_in_communicator != NULL)
      {
	tmp = (sctk_internal_communicator_t *)
	  __sctk_malloc (sctk_aligned_sizeof
			 (sctk_internal_communicator_t)
			 +
			 2 *
			 sctk_aligned_size (sctk_total_task_number *
					    sizeof (int)),
			 sctk_communicator_storage);
      }
    else
      {
	tmp = (sctk_internal_communicator_t *)
	  __sctk_malloc (sctk_aligned_sizeof
			 (sctk_internal_communicator_t),
			 sctk_communicator_storage);
      }

    sctk_nodebug("New communicator allocated");
    sctk_communicator_list[pos] = tmp;
    if (orig->rank_in_communicator != NULL)
      {
	rank_list =
	  (int *) ((char *) tmp +
		   sctk_aligned_sizeof (sctk_internal_communicator_t));
	for (i = 0; i < sctk_total_task_number; i++)
	  {
	    rank_list[i] = orig->rank_in_communicator[i];
	  }
	glob_list =
	  (int *) ((char *) tmp +
		   sctk_aligned_sizeof (sctk_internal_communicator_t)
		   +
		   sctk_aligned_size (sctk_total_task_number *
				      sizeof (int)));
	for (i = 0; i < sctk_total_task_number; i++)
	  {
	    glob_list[i] = tmp->global_in_communicator_local[i];
	  }
      }
    else
      {
	rank_list = NULL;
	glob_list = NULL;
      }
    sctk_nodebug("Lists updated");
    

    tmp->communicator_number = dest_comm;
    tmp->origin_communicator = origin_communicator;
    tmp->new_communicator = -1;
    tmp->nb_task_involved = nb_task_involved;
    tmp->nb_task_registered = 0;
    
    tmp->rank_in_communicator = rank_list;
    
    tmp->local_communicator_size = nb_task_involved;
    tmp->global_in_communicator_local = glob_list;
    tmp->remote_communicator_size = nb_task_involved;
    tmp->global_in_communicator_remote = glob_list;
    
    sctk_thread_mutex_init (&tmp->lock, NULL);
    tmp->collective_communications =
      sctk_collective_communications_create (nb_task_involved);
    sctk_collective_communications_duplicate(orig->collective_communications,tmp->collective_communications,nb_task_involved,dest_comm);
    
    tmp->nb_task_registered = nb_task_involved;
    
    sctk_nodebug("Communicator created");
    
    sctk_nodebug("Task_list %p %p",rank_list,glob_list);
#warning "Have to check if rank_list != NULL"
    sctk_net_hybrid_init_new_com(tmp, nb_task_involved, rank_list);
    sctk_nodebug("Hybride done");

  }

  sctk_thread_mutex_unlock (&sctk_global_communicator_number_lock);
}

static void
sctk_update_new_communicator_all (const sctk_communicator_t
				  origin_communicator,
				  const int nb_task_involved,
				  const int *task_list)
{
  int i;

/*       { */
/* 	int w;  */
/* 	for( w = 0; w < nb_task_involved; w++){ */
/* 	  sctk_debug("send %d = %d",w,task_list[w]); */
/* 	} */
/*       } */

  for (i = 0; i < sctk_process_number; i++)
    {
      if (i == sctk_process_rank)
	{
	  sctk_update_new_communicator (origin_communicator,
					nb_task_involved, task_list);
	}
      else
	{
	  sctk_net_update_new_communicator (origin_communicator,
					    nb_task_involved, task_list, i);
	}
    }
}

static void
sctk_get_free_communicator (const sctk_communicator_t origin_communicator)
{
  if (sctk_process_rank == 0)
    {
      sctk_get_free_communicator_on_root (origin_communicator);
    }
  else
    {
      sctk_thread_mutex_lock (&sctk_global_communicator_number_lock);
      if (sctk_communicator_list[origin_communicator]->new_communicator == (sctk_communicator_t)-1)
	{
	  /*The network mode have to block thread util completion */
	  sctk_net_get_free_communicator (origin_communicator);
	}
      assume (sctk_communicator_list[origin_communicator]->
	      new_communicator != (sctk_communicator_t)-1);
      sctk_thread_mutex_unlock (&sctk_global_communicator_number_lock);
    }
  assume (sctk_communicator_list[origin_communicator]->new_communicator !=
	  (sctk_communicator_t)-1);
}

static void
sctk_get_free_communicator_duplicate (const sctk_communicator_t origin_communicator, int rank)
{
  int root = 0;
#warning "Compute optimal root"
  if(rank == root){
    if(sctk_process_rank == 0){
      sctk_get_free_communicator_on_root_no_rpc (origin_communicator, sctk_process_rank);
    } else {
      sctk_net_get_free_communicator_no_rpc  (origin_communicator);
    }
    assume (sctk_communicator_list[origin_communicator]->
	    new_communicator != (sctk_communicator_t)-1);
  }
  sctk_broadcast ((void*)&(sctk_communicator_list[origin_communicator]->new_communicator),sizeof(sctk_communicator_t),root,origin_communicator);
}

static inline int
__sctk_get_rank (const sctk_communicator_t communicator,
		 const int comm_world_rank)
{
  int res;
  if (sctk_communicator_list[communicator]->rank_in_communicator != NULL)
    res = sctk_communicator_list[communicator]->
      rank_in_communicator[comm_world_rank];
  else
    res = comm_world_rank;
  return res;
}

int
sctk_is_inter_comm (const sctk_communicator_t communicator)
{
  return sctk_communicator_list[communicator]->is_inter_comm;
}

sctk_communicator_t
sctk_duplicate_communicator (const sctk_communicator_t origin_communicator,
			     int is_inter_comm, int rank)
{
  if(is_inter_comm == 0){
    sctk_internal_communicator_t *tmp = NULL;
    sctk_communicator_t res = 0;
#warning "Optimize this part"
    sctk_get_free_communicator_duplicate (origin_communicator,rank);
    sctk_barrier (origin_communicator);
    res = sctk_communicator_list[origin_communicator]->new_communicator;
    
    sctk_update_new_communicator_all_dup (origin_communicator,res);

    tmp = sctk_communicator_list[res];

    sctk_nodebug("sctk_update_new_communicator_all_dup DONE");
    sctk_barrier (origin_communicator);
    sctk_communicator_list[origin_communicator]->new_communicator = -1;
    sctk_barrier (origin_communicator);

    sctk_nodebug("Barrier origin");
    
#warning "Have to save comm dup"
#if 0
    if (sctk_get_rank (origin_communicator, sctk_get_task_rank ()) ==
	task_list[0])
      {
	char name[2048];
	FILE *file;
	int i;
	i = -1;
	
	do
	  {
	    i++;
	    sprintf (name, "%s/communicator_%d_%d", sctk_store_dir,
		     (int) tmp->communicator_number, i);
	    sctk_nodebug("%s", name);
	    file = fopen (name, "r");
	  }
	while (file != NULL);

	sprintf (name, "%s/communicator_%d_%d", sctk_store_dir,
		 (int) tmp->communicator_number, i);
	file = fopen (name, "w");
	assume (file != NULL);
	fprintf (file, "%d\n", (int) origin_communicator);
	fprintf (file, "%d\n", (int) nb_task_involved);
	for (i = 0; i < nb_task_involved; i++)
	  {
	    fprintf (file, "%d\n", task_list[i]);
	  }
	fclose (file);
      }
#endif

    tmp->is_inter_comm = is_inter_comm;
    sctk_barrier (origin_communicator);

    sctk_nodebug("Communicator created");

    sctk_barrier (tmp->communicator_number);
    sctk_nodebug("Barrier on new communicator done");


    return tmp->communicator_number;
  } else {
    not_implemented();
    return 0;
  }
}
sctk_communicator_t
sctk_create_communicator (const sctk_communicator_t origin_communicator,
			  const int nb_task_involved, const int *task_list,
			  int is_inter_comm)
{
  sctk_internal_communicator_t *tmp = NULL;
  sctk_communicator_t res = 0;

  sctk_nodebug ("Create communicator for %d task on %d comm",
		nb_task_involved, origin_communicator);

  sctk_get_free_communicator (origin_communicator);

  sctk_barrier (origin_communicator);
  res = sctk_communicator_list[origin_communicator]->new_communicator;

  sctk_update_new_communicator_all (origin_communicator, nb_task_involved,
				    task_list);

  tmp = sctk_communicator_list[res];
  sctk_thread_wait_for_value ((int *) &(tmp->nb_task_registered),
			      (int) tmp->nb_task_involved);

  sctk_nodebug ("Create communicator for %d task on %d comm done new %d",
		nb_task_involved, origin_communicator,
		tmp->communicator_number);

  if (sctk_get_rank (origin_communicator, sctk_get_task_rank ()) ==
      task_list[0])
    {
      char name[2048];
      FILE *file;
      int i;
      i = -1;

      do
	{
	  i++;
	  sprintf (name, "%s/communicator_%d_%d", sctk_store_dir,
		   (int) tmp->communicator_number, i);
    sctk_nodebug("%s", name);
	  file = fopen (name, "r");
	}
      while (file != NULL);

      sprintf (name, "%s/communicator_%d_%d", sctk_store_dir,
	       (int) tmp->communicator_number, i);
      file = fopen (name, "w");
      assume (file != NULL);
      fprintf (file, "%d\n", (int) origin_communicator);
      fprintf (file, "%d\n", (int) nb_task_involved);
      for (i = 0; i < nb_task_involved; i++)
	{
	  fprintf (file, "%d\n", task_list[i]);
	}
      fclose (file);
    }

  tmp->is_inter_comm = is_inter_comm;

  return tmp->communicator_number;
}

void
sctk_update_free_communicator (const sctk_communicator_t communicator)
{
  sctk_internal_communicator_t *tmp;
  tmp = sctk_communicator_list[communicator];

  sctk_thread_mutex_lock (&sctk_global_communicator_number_lock);
  tmp->nb_task_registered--;
  if (tmp->nb_task_registered == 0)
    {
      /*Collective communication destruction */
      sctk_collective_communications_delete (tmp->collective_communications);

      sctk_communicator_list[communicator] = NULL;
      __sctk_free (tmp, sctk_communicator_storage);

      if (sctk_process_rank == 0)
	{
	  char name[2048];
	  FILE *file;
	  sprintf (name, "%s/communicators", sctk_store_dir);
	  file = fopen (name, "a");
	  assume (file != NULL);
	  fprintf (file, "D %d\n", (int) communicator);
	  fclose (file);
	}
    }
  sctk_thread_mutex_unlock (&sctk_global_communicator_number_lock);
}

sctk_communicator_t
sctk_delete_communicator (const sctk_communicator_t communicator)
{
  int i;
  for (i = 0; i < sctk_process_number; i++)
    {
      if (i != sctk_process_rank)
	{
	  sctk_net_update_new_communicator (communicator, 0, NULL, i);
	}
    }
  sctk_update_free_communicator (communicator);

  return SCTK_COMM_EMPTY;
}

int
sctk_get_rank (const sctk_communicator_t communicator,
	       const int comm_world_rank)
{
  int res;
  res = __sctk_get_rank (communicator, comm_world_rank);
  sctk_nodebug("RANK : %d", res);
  assume (res >= 0);
  return res;
}

int
sctk_get_nb_task_local (const sctk_communicator_t communicator)
{
  return sctk_communicator_list[communicator]->local_communicator_size;
}

int
sctk_get_nb_task_remote (const sctk_communicator_t communicator)
{
  return sctk_communicator_list[communicator]->remote_communicator_size;
}


void
sctk_get_rank_size_local (const sctk_communicator_t communicator, int *rank,
			  int *size, int glob_rank)
{
  int comm_world_rank;
  comm_world_rank = glob_rank;
  if (sctk_communicator_list[communicator]->rank_in_communicator != NULL)
    *rank = sctk_communicator_list[communicator]->
      rank_in_communicator[comm_world_rank];
  else
    *rank = comm_world_rank;
  *size = sctk_communicator_list[communicator]->local_communicator_size;
}

void
sctk_get_rank_size_remote (const sctk_communicator_t communicator, int *rank,
			   int *size, int glob_rank)
{
  int comm_world_rank;
  comm_world_rank = glob_rank;
  if (sctk_communicator_list[communicator]->rank_in_communicator != NULL)
    *rank = sctk_communicator_list[communicator]->
      rank_in_communicator[comm_world_rank];
  else
    *rank = comm_world_rank;
  *size = sctk_communicator_list[communicator]->remote_communicator_size;
}

int
sctk_is_valid_comm (const sctk_communicator_t communicator)
{
  return (sctk_communicator_list[communicator] != NULL);
}
