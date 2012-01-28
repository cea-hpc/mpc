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
#include <string.h>
#include <sched.h>
#include "sctk_collective_communications.h"
#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_topology.h"
#include "sctk_thread.h"
#include "sctk_alloc.h"
#include "sctk_communicator.h"
#include "sctk_low_level_comm.h"
#include "sctk_asm.h"

#define SCTK_LOCAL_VERSION_MAJOR 0
#define SCTK_LOCAL_VERSION_MINOR 2

#define SCTK_NUMBER_OF_SONS 2

static sctk_messages_alloc_thread_data_t collective_comm_allocator;

void
sctk_collective_communication_init_mem ()
{
  sctk_only_once ();
  sctk_print_version ("Collective", SCTK_LOCAL_VERSION_MAJOR,
		      SCTK_LOCAL_VERSION_MINOR);

  collective_comm_allocator.alloc = __sctk_create_thread_memory_area ();
  collective_comm_allocator.lock = 0;

}

static void
sctk_collective_communication_set_tree (const int start, const int end,
					sctk_virtual_processor_t **
					virtual_processors,
					sctk_virtual_processor_t * father)
{
  sctk_nodebug ("Build from %d to %d", start, end);
  if ((end - start) < SCTK_NUMBER_OF_SONS)
    {
      int i;
      /*End of recursion */
      for (i = start; i <= end; i++)
	{
	  virtual_processors[i]->father = father;
	}
    }
  else
    {
      int i;
      int nb_blocks, offset;
      int tmp_start, tmp_end;
      sctk_virtual_processor_t *tmp_father;
      nb_blocks = (end - start + 1) / SCTK_NUMBER_OF_SONS;
      if ((end - start + 1) % SCTK_NUMBER_OF_SONS)
	nb_blocks++;
      if (nb_blocks > SCTK_NUMBER_OF_SONS)
	nb_blocks = SCTK_NUMBER_OF_SONS;

      offset = (end - start + 1) / nb_blocks;

      tmp_start = start;
      tmp_end = start + offset - 1;
      for (i = 0; i < nb_blocks; i++)
	{
	  if ((end - start + 1) % nb_blocks > i){
	    tmp_end++;
	  }
	  sctk_spinlock_lock(&(collective_comm_allocator.lock));
	  tmp_father = (sctk_virtual_processor_t *)
	    __sctk_malloc_on_node (sizeof (sctk_virtual_processor_t),
				   sctk_get_node_from_cpu (tmp_start),
				   collective_comm_allocator.alloc);

	  tmp_father->list = NULL;
	  tmp_father->data.data_in = NULL;
	  tmp_father->data.data_out = NULL;
	  tmp_father->nb_task_involved = 0;
	  tmp_father->nb_task_registered = 0;

	  tmp_father->data.data_out_tab =
	    (sctk_collective_communications_data_t **)
	    __sctk_malloc_on_node (sizeof
				   (sctk_collective_communications_data_t
				    *) * SCTK_NUMBER_OF_SONS,
				   sctk_get_node_from_cpu (tmp_start),
				   collective_comm_allocator.alloc);
	  sctk_spinlock_unlock(&(collective_comm_allocator.lock));
	  sctk_thread_mutex_init (&(tmp_father->lock), NULL);
	  sctk_nodebug ("%p->%p", father, tmp_father);
	  tmp_father->father = father;

	  sctk_collective_communication_set_tree (tmp_start, tmp_end,
						  virtual_processors,
						  tmp_father);
	  tmp_start = tmp_end + 1;
	  tmp_end = tmp_start + offset - 1;
	}
    }
}

void
sctk_net_realloc (sctk_collective_communications_t * com, size_t size)
{
  sctk_spinlock_lock(&(collective_comm_allocator.lock));
  com->net_tmp_data = __sctk_realloc (com->net_tmp_data, size, collective_comm_allocator.alloc);
  sctk_spinlock_unlock(&(collective_comm_allocator.lock));
}

void sctk_collective_communications_duplicate(sctk_collective_communications_t
					      * from , sctk_collective_communications_t
					      *tmp,int nb_task_involved,const sctk_communicator_t com_id){
  int i;
  for (i = 0; i < nb_task_involved; i++)
    {
      sctk_init_collective_communicator (from->last_vp[i], i, com_id,
					     from->last_process[i]);
    }

  sctk_nodebug("Initialized %d", tmp->initialized);
  
}

sctk_collective_communications_t
  * sctk_collective_communications_create (const int nb_task_involved)
{
  int i, cpu_number;
  sctk_virtual_processor_t *tmp_father;
  sctk_collective_communications_t *tmp = NULL;
  int sctk_total_task_number;

  sctk_total_task_number = nb_task_involved;

  sctk_nodebug ("sctk_collective_communications_create");

  sctk_spinlock_lock(&(collective_comm_allocator.lock));
  tmp = (sctk_collective_communications_t *)
    __sctk_malloc (sizeof (sctk_collective_communications_t), collective_comm_allocator.alloc);
  tmp->migration_list = NULL;
  sctk_thread_mutex_init (&(tmp->lock_migration_list), NULL);

/*
  tmp->data.elem_size = 0;
  tmp->data.nb_elem = 0;
*/
  tmp->data.data_in = NULL;
  tmp->data.data_out = NULL;
/*   tmp->data.func = NULL; */

  tmp->virtual_processors = NULL;
  tmp->net_tmp_data = NULL;

  tmp->nb_task_involved = nb_task_involved;
  tmp->nb_task_registered = 0;

  tmp->initialized = 0;

  sctk_thread_mutex_init (&(tmp->lock), NULL);

  cpu_number = sctk_get_cpu_number ();

  tmp->last_vp = (int *)
    __sctk_malloc (nb_task_involved * sizeof (int), collective_comm_allocator.alloc);
  for (i = 0; i < nb_task_involved; i++)
    {
      tmp->last_vp[i] = -1;
    }
  tmp->last_process = (int *)
    __sctk_malloc (nb_task_involved * sizeof (int), collective_comm_allocator.alloc);
  for (i = 0; i < nb_task_involved; i++)
    {
      tmp->last_process[i] = -1;
    }
  tmp->involved_list = (int *)
    __sctk_malloc (sctk_process_number * sizeof (int), collective_comm_allocator.alloc);
  for (i = 0; i < sctk_process_number; i++)
    {
      tmp->involved_list[i] = 0;
    }
  tmp->receive_list = (int *)
    __sctk_malloc (sctk_process_number * sizeof (int), collective_comm_allocator.alloc);
  sctk_nodebug ("sctk_collective_communications_create %p",
		tmp->receive_list);
  for (i = 0; i < sctk_process_number; i++)
    {
      tmp->receive_list[i] = -1;
    }

  tmp->virtual_processors = (sctk_virtual_processor_t **)
    __sctk_malloc (cpu_number * sizeof (sctk_virtual_processor_t *),
		   collective_comm_allocator.alloc);


  for (i = 0; i < cpu_number; i++)
    {
      tmp->virtual_processors[i] = (sctk_virtual_processor_t *)
	__sctk_malloc_on_node (sizeof (sctk_virtual_processor_t),
			       sctk_get_node_from_cpu (i), collective_comm_allocator.alloc);
      tmp->virtual_processors[i]->list = NULL;
/*
    tmp->virtual_processors[i]->data.elem_size = 0;
    tmp->virtual_processors[i]->data.nb_elem = 0;
*/
      tmp->virtual_processors[i]->data.data_in = NULL;
      tmp->virtual_processors[i]->data.data_out = NULL;
/*     tmp->virtual_processors[i]->data.func = NULL; */
      tmp->virtual_processors[i]->nb_task_involved = 0;
      tmp->virtual_processors[i]->nb_task_registered = 0;
      sctk_thread_mutex_init (&(tmp->virtual_processors[i]->lock), NULL);
      tmp->virtual_processors[i]->father = NULL;
      tmp->virtual_processors[i]->data.data_out_tab =
	(sctk_collective_communications_data_t **)
	__sctk_malloc_on_node (sizeof
			       (sctk_collective_communications_data_t *)
			       * nb_task_involved,
			       sctk_get_node_from_cpu (i), collective_comm_allocator.alloc);
    }


  /*Initialise evaluation tree */
  tmp_father = (sctk_virtual_processor_t *)
    __sctk_malloc_on_node (sizeof (sctk_virtual_processor_t),
			   sctk_get_node_from_cpu (0), collective_comm_allocator.alloc);

  tmp_father->list = NULL;
/*
  tmp_father->data.elem_size = 0;
  tmp_father->data.nb_elem = 0;
*/
  tmp_father->data.data_in = NULL;
  tmp_father->data.data_out = NULL;
/*   tmp_father->data.func = NULL; */
  tmp_father->nb_task_involved = 0;
  tmp_father->nb_task_registered = 0;
  tmp_father->data.data_out_tab =
    (sctk_collective_communications_data_t **)
    __sctk_malloc_on_node (sizeof
			   (sctk_collective_communications_data_t *) *
			   SCTK_NUMBER_OF_SONS,
			   sctk_get_node_from_cpu (0), collective_comm_allocator.alloc);
  sctk_thread_mutex_init (&(tmp_father->lock), NULL);
  tmp_father->father = NULL;
  sctk_spinlock_unlock(&(collective_comm_allocator.lock));
  sctk_collective_communication_set_tree (0, cpu_number - 1,
					  tmp->virtual_processors,
					  tmp_father);
  tmp->initialized = 0;
  sctk_nodebug ("initalisation of tree done");

  return tmp;
}

void sctk_init_comm_world(int sctk_total_task_number){
  int i;

  sctk_nodebug ("initalisation of remote COMM_WORLD start");
  {
      for(i = 0; i < sctk_total_task_number; i++){
	int j;
	int pos = -1;
	int local_threads;
	int start_thread;
	int THREAD_NUMBER;
	j = i;
	THREAD_NUMBER = sctk_total_task_number;

	do{
	  pos ++;
	  local_threads = THREAD_NUMBER / sctk_process_number;
	  if (THREAD_NUMBER % sctk_process_number > pos)
	    local_threads++;

	  start_thread = local_threads * pos;
	  if (THREAD_NUMBER % sctk_process_number <= pos)
	    start_thread += THREAD_NUMBER % sctk_process_number;
	  sctk_nodebug("Start %d end %d on %d thread %d",start_thread,start_thread + local_threads,pos,j);
	}while(j >= start_thread + local_threads);
	if(pos != sctk_process_rank){
	  sctk_init_collective_communicator (0, j, 0,
					     pos);
	}
      }
  }
  sctk_nodebug ("initalisation of remote COMM_WORLD done");
}

static void
sctk_free_branche (sctk_virtual_processor_t * my_vp)
{
  if (my_vp->father != NULL)
    {
      sctk_nodebug ("FATHER %p %d", my_vp->father,
		    my_vp->father->nb_task_involved);
      my_vp->father->nb_task_involved--;
      if (my_vp->father->nb_task_involved == 0)
	{
	  sctk_free_branche (my_vp->father);
	  sctk_nodebug ("FREE FATHER %p", my_vp->father);
	  sctk_free (my_vp->father->data.data_out_tab);
	  sctk_free (my_vp->father);
	}
    }
}

static void
sctk_deactivate_branche (sctk_virtual_processor_t * my_vp)
{
  if (my_vp != NULL)
    {
      my_vp->nb_task_involved = 0;
      sctk_deactivate_branche (my_vp->father);
    }
}

static void
sctk_activate_branche (sctk_virtual_processor_t * my_vp)
{
  if (my_vp != NULL)
    {
      my_vp->nb_task_involved++;
      my_vp->nb_task_registered = 0;
      sctk_nodebug ("Nb involved %d", my_vp->nb_task_involved);
      if (my_vp->nb_task_involved != 1)
	return;
      sctk_activate_branche (my_vp->father);
    }
}

void
sctk_collective_communications_delete (sctk_collective_communications_t * com)
{
  int i, cpu_number;
  cpu_number = sctk_get_cpu_number ();
  sctk_free (com->last_vp);
  sctk_free (com->last_process);
  sctk_free ((void *) com->involved_list);
  sctk_nodebug ("FREE %p", com->receive_list);
  sctk_free ((void *) com->receive_list);

  /*delete collective communication tree */
  for (i = 0; i < cpu_number; i++)
    {
      sctk_deactivate_branche (com->virtual_processors[i]);
    }
  for (i = 0; i < cpu_number; i++)
    {
      sctk_activate_branche (com->virtual_processors[i]);
    }
  for (i = 0; i < cpu_number; i++)
    {
      sctk_free_branche (com->virtual_processors[i]);
      sctk_free (com->virtual_processors[i]->data.data_out_tab);
      sctk_free (com->virtual_processors[i]);
    }

  sctk_free (com->virtual_processors);

  sctk_free (com);
}

sctk_collective_communications_t *
sctk_collective_communications_init (const int nb_task_involved)
{
  sctk_collective_communications_t *tmp = NULL;
  sctk_print_version ("Collective communications",
		      SCTK_LOCAL_VERSION_MAJOR, SCTK_LOCAL_VERSION_MINOR);
  tmp = sctk_collective_communications_create (nb_task_involved);
  return tmp;
}

static inline void
sctk_perform_collective_communication_tree (sctk_collective_communications_t
					    * restrict com,
					    sctk_virtual_processor_t *
					    restrict my_vp,
					    const size_t elem_size,
					    const size_t nb_elem,
					    void (*func) (const void *,
							  void *, size_t,
							  sctk_datatype_t),
					    const sctk_datatype_t data_type)
{
  sctk_virtual_processor_t *father;
  my_vp->data.done = 0;
  father = my_vp->father;
  if (father == NULL)
    {
      if (com->nb_task_involved_in_this_process < com->nb_task_involved)
	{
	  /*Multiple process involved */
	  sctk_nodebug ("Entering net collective communication");
	  sctk_net_collective_op (com, my_vp, elem_size, nb_elem, -1, func,
				  data_type);
	  sctk_nodebug ("Leaving net collective communication");
	}
      else
	{
	  /* process involved */
	  if ((nb_elem != 0) && (func == NULL))
	    {
	      my_vp->data.data_out = my_vp->data.data_in;
	    }
	}


      if (com->reinitialize == 1)
	com->initialized = 0;

      /*wrong virtual processor liberation */
      if (com->migration_list != NULL)
	{
	  sctk_thread_mutex_lock (&com->lock_migration_list);
	  {
	    sctk_migration_data_t *mig_tmp;
	    sctk_migration_data_t *mig_cur;
	    mig_tmp = (sctk_migration_data_t *) com->migration_list;
	    mig_cur = mig_tmp;
	    while (mig_cur != NULL)
	      {
		mig_tmp = mig_cur->next;
		if (mig_cur->data != NULL)
		  {
		    if (my_vp->data.data_out == NULL)
		      memcpy (mig_cur->data, my_vp->data.data_in,
			      elem_size * nb_elem);
		    else
		      memcpy (mig_cur->data, my_vp->data.data_out,
			      elem_size * nb_elem);
		  }
		mig_cur->done = 1;
		mig_cur = mig_tmp;
	      }
	  }
	  com->migration_list = NULL;
	  sctk_thread_mutex_unlock (&com->lock_migration_list);
	}
    }
  else
    {
      int i;
      int nb_registered;
/*       sctk_thread_mutex_lock (&father->lock); */
      while (sctk_thread_mutex_trylock (&father->lock) != 0)
	sctk_thread_yield ();

      /*Contribute */
      nb_registered = father->nb_task_registered;
      if (nb_elem != 0)
	{
	  if (nb_registered == 0)
	    {
	      /*I am the first */
	      father->data.data_in = my_vp->data.data_in;
	      father->data.data_out = my_vp->data.data_out;
	    }
	  else
	    {
	      if (func != NULL)
		{
		  /*Reduction */
		  func (my_vp->data.data_out, father->data.data_out,
			nb_elem, data_type);
		}
	      else
		{
		  /*Broadcast */
		  if (my_vp->data.data_in != NULL)
		    {
		      father->data.data_in = my_vp->data.data_in;
		    }

		}
	    }
	}

      father->data.data_out_tab[nb_registered] = &(my_vp->data);
      nb_registered++;
      father->nb_task_registered = nb_registered;
      if (nb_registered == father->nb_task_involved)
	{
	  sctk_perform_collective_communication_tree (com, father,
						      elem_size, nb_elem,
						      func, data_type);
	  father->nb_task_registered = 0;
	  sctk_nodebug("REINIT %d %s",father->nb_task_registered,__func__);

	  if (nb_elem != 0)
	    {
	      void *tmp_data_out;
	      unsigned long tab_size;
	      if (func == NULL)
		{
		  if (father->data.data_out == NULL)
		    father->data.data_out = father->data.data_in;
		}
	      tmp_data_out = father->data.data_out;
	      tab_size = elem_size * nb_elem;
	      for (i = 0; i < father->nb_task_involved; i++)
		{
		  void *tmp_data_out_recv;
		  tmp_data_out_recv = father->data.data_out_tab[i]->data_out;
		  if (tmp_data_out_recv != NULL)
		    {
		      if (tmp_data_out_recv != tmp_data_out)
			memcpy (tmp_data_out_recv, tmp_data_out, tab_size);
		    }
		}
	    }
	  for (i = 0; i < father->nb_task_involved; i++)
	    {
	      father->data.data_out_tab[i]->done = 1;
	    }
	  /*Wake */
	  sctk_thread_mutex_unlock (&father->lock);
	}
      else
	{
	  /*Freeze */
	  sctk_thread_mutex_unlock (&father->lock);
	  if (my_vp->data.done != 1)
	    {
	      sctk_thread_wait_for_value ((int *) &(my_vp->data.done), 1);
	    }
	}
    }
}
static inline void
  sctk_perform_collective_communication_tree_broadcast
  (sctk_collective_communications_t * restrict com,
   sctk_virtual_processor_t * restrict my_vp, const size_t elem_size,
   const size_t nb_elem, const int root, const sctk_datatype_t data_type)
{
  sctk_virtual_processor_t *restrict father;
  sctk_nodebug ("BCAST TREE Using vp %p com_id %d root %d", my_vp, com->id, root);
  my_vp->data.done = 0;
  father = my_vp->father;
  if (expect_false (father == NULL))
    {
      if (expect_false
	  (com->nb_task_involved_in_this_process < com->nb_task_involved))
	{
	  /*Multiple process involved */
	  sctk_nodebug ("Entering net collective communication with root %d", root);
	  sctk_net_collective_op (com, my_vp, elem_size, nb_elem, root, NULL,
				  data_type);
	  sctk_nodebug ("Leaving net collective communication");
	}
      else
	{
	  /* process involved */
	  my_vp->data.data_out = my_vp->data.data_in;
	}


      if (expect_false (com->reinitialize == 1))
	com->initialized = 0;

      /*wrong virtual processor liberation */
      if (expect_false (com->migration_list != NULL))
	{
	  sctk_thread_mutex_lock (&com->lock_migration_list);
	  {
	    sctk_migration_data_t *mig_tmp;
	    sctk_migration_data_t *mig_cur;
	    mig_tmp = (sctk_migration_data_t *) com->migration_list;
	    mig_cur = mig_tmp;
	    while (mig_cur != NULL)
	      {
		mig_tmp = mig_cur->next;
		if (mig_cur->data != NULL)
		  {
		    if (my_vp->data.data_out == NULL)
		      memcpy (mig_cur->data, my_vp->data.data_in,
			      elem_size * nb_elem);
		    else
		      memcpy (mig_cur->data, my_vp->data.data_out,
			      elem_size * nb_elem);
		  }
		mig_cur->done = 1;
		mig_cur = mig_tmp;
	      }
	  }
	  com->migration_list = NULL;
	  sctk_thread_mutex_unlock (&com->lock_migration_list);
	}
    }
  else
    {
      int i;
      int nb_registered;
      int last;
      struct sctk_collective_communications_data_s **data_out_tab;
/*       sctk_thread_mutex_lock (&father->lock); */
      while (expect_false (sctk_thread_mutex_trylock (&father->lock) != 0))
	sctk_thread_yield ();

      /*Contribute */
      nb_registered = father->nb_task_registered;
      last = father->nb_task_involved;
      if (expect_false (nb_registered == 0))
	{
	  /*I am the first */
	  father->data.data_in = my_vp->data.data_in;
	  father->data.data_out = my_vp->data.data_out;
	}
      else
	{
	  /*Broadcast */
	  if (expect_false (my_vp->data.data_in != NULL))
	    {
	      father->data.data_in = my_vp->data.data_in;
	    }
	}

      sctk_nodebug ("BCAST %d registered on %d involved %p (%d)",
		    nb_registered, my_vp->nb_task_involved, my_vp, com->id);
      data_out_tab = father->data.data_out_tab;
      data_out_tab[nb_registered] = &(my_vp->data);
      nb_registered++;
      father->nb_task_registered = nb_registered;
      if (expect_false (nb_registered == father->nb_task_involved))
	{
	  void *tmp_data_out;
	  unsigned long tab_size;
	  sctk_perform_collective_communication_tree_broadcast (com,
								father,
								elem_size,
								nb_elem,
                root,
								data_type);
	  father->nb_task_registered = 0;
	  sctk_nodebug("REINIT %d %s",father->nb_task_registered,__func__);

	  if (father->data.data_out == NULL)
	    father->data.data_out = father->data.data_in;

	  tmp_data_out = father->data.data_out;
	  tab_size = elem_size * nb_elem;

	  if (expect_true (last == 2))
	    {
	      void *tmp_data_out_recv0;
	      void *tmp_data_out_recv1;
	      tmp_data_out_recv0 = data_out_tab[0]->data_out;
	      if (expect_true (tmp_data_out_recv0 != NULL))
		{
		  if (expect_true (tmp_data_out_recv0 != tmp_data_out))
		    memcpy (tmp_data_out_recv0, tmp_data_out, tab_size);
		}

	      tmp_data_out_recv1 = data_out_tab[1]->data_out;
	      if (expect_true (tmp_data_out_recv1 != NULL))
		{
		  if (expect_true (tmp_data_out_recv1 != tmp_data_out))
		    memcpy (tmp_data_out_recv1, tmp_data_out, tab_size);
		}

	      father->data.data_out_tab[0]->done = 1;
	      father->data.data_out_tab[1]->done = 1;
	    }
	  else
	    {
	      for (i = 0; i < last; i++)
		{
		  void *tmp_data_out_recv;
		  tmp_data_out_recv = data_out_tab[i]->data_out;
		  if (expect_true (tmp_data_out_recv != NULL))
		    {
		      if (expect_true (tmp_data_out_recv != tmp_data_out))
			memcpy (tmp_data_out_recv, tmp_data_out, tab_size);
		    }
		}

	      for (i = 0; i < last; i++)
		{
		  father->data.data_out_tab[i]->done = 1;
		}
	    }

	  /*Wake */
	  sctk_thread_mutex_unlock (&father->lock);
	}
      else
	{
	  /*Freeze */
	  sctk_thread_mutex_unlock (&father->lock);
	  if (expect_true (my_vp->data.done != 1))
	    {
	      sctk_thread_wait_for_value ((int *) &(my_vp->data.done), 1);
	    }
	}
    }
}

static inline void
  sctk_perform_collective_communication_tree_reduction
  (sctk_collective_communications_t * restrict com,
   sctk_virtual_processor_t * restrict my_vp, const size_t elem_size,
   const size_t nb_elem, void (*func) (const void *, void *, size_t,
				       sctk_datatype_t),
   const sctk_datatype_t data_type)
{
  sctk_virtual_processor_t *restrict father;

  sctk_nodebug ("REDUCTION TREE Using vp %p com_id %d", my_vp, com->id);

  my_vp->data.done = 0;
  father = my_vp->father;
  sctk_assert (my_vp->data.data_out);
  sctk_assert (my_vp->data.data_in);
  if (expect_false (father == NULL))
    {
      if (expect_false
	  (com->nb_task_involved_in_this_process < com->nb_task_involved))
	{
	  /*Multiple process involved */
	  sctk_nodebug ("Entering net collective communication");
	  sctk_net_collective_op (com, my_vp, elem_size, nb_elem, -1, func,
				  data_type);
	  sctk_nodebug ("Leaving net collective communication");
	}

      if (expect_false (com->reinitialize == 1))
	com->initialized = 0;

      /*wrong virtual processor liberation */
      if (expect_false (com->migration_list != NULL))
	{
	  sctk_thread_mutex_lock (&com->lock_migration_list);
	  {
	    sctk_migration_data_t *mig_tmp;
	    sctk_migration_data_t *mig_cur;
	    mig_tmp = (sctk_migration_data_t *) com->migration_list;
	    mig_cur = mig_tmp;
	    while (mig_cur != NULL)
	      {
		mig_tmp = mig_cur->next;
		if (mig_cur->data != NULL)
		  {
		    if (my_vp->data.data_out == NULL)
		      memcpy (mig_cur->data, my_vp->data.data_in,
			      elem_size * nb_elem);
		    else
		      memcpy (mig_cur->data, my_vp->data.data_out,
			      elem_size * nb_elem);
		  }
		mig_cur->done = 1;
		mig_cur = mig_tmp;
	      }
	  }
	  com->migration_list = NULL;
	  sctk_thread_mutex_unlock (&com->lock_migration_list);
	}
    }
  else
    {
      int i;
      int nb_registered;
      int last;
      struct sctk_collective_communications_data_s **data_out_tab;
/*       sctk_thread_mutex_lock (&father->lock); */
      while (expect_false (sctk_thread_mutex_trylock (&father->lock) != 0))
	sctk_thread_yield ();

      /*Contribute */
      nb_registered = father->nb_task_registered;
      last = father->nb_task_involved;

      sctk_nodebug ("RED %d registered on %d involved %p (%d)", nb_registered,
		    my_vp->nb_task_involved, my_vp, com->id);
      if (expect_false (nb_registered == 0))
	{
	  /*I am the first */
	  father->data.data_in = my_vp->data.data_in;
	  father->data.data_out = my_vp->data.data_out;
	  sctk_assert (father->data.data_out);
	  sctk_assert (father->data.data_in);
	  sctk_nodebug ("I am the first in tree");
	}
      else
	{
	  sctk_assert (father->data.data_out);
	  sctk_assert (father->data.data_in);
	  func (my_vp->data.data_out, father->data.data_out, nb_elem,
		data_type);
	}


      data_out_tab = father->data.data_out_tab;
      data_out_tab[nb_registered] = &(my_vp->data);
      nb_registered++;
      father->nb_task_registered = nb_registered;
      if (expect_false (nb_registered == last))
	{
	  void *tmp_data_out;
	  unsigned long tab_size;
	  sctk_perform_collective_communication_tree_reduction (com,
								father,
								elem_size,
								nb_elem,
								func,
								data_type);
	  father->nb_task_registered = 0;
	  sctk_nodebug("REINIT %d %s",father->nb_task_registered,__func__);

	  tmp_data_out = father->data.data_out;
	  tab_size = elem_size * nb_elem;

	  for (i = 0; i < last; i++)
	    {
	      void *tmp_data_out_recv;
	      tmp_data_out_recv = data_out_tab[i]->data_out;
	      if (expect_true (tmp_data_out_recv != NULL))
		{
		  if (expect_true (tmp_data_out_recv != tmp_data_out))
		    memcpy (tmp_data_out_recv, tmp_data_out, tab_size);
		}
/* 	      data_out_tab[i]->done = 1; */
	    }

	  for (i = 0; i < last; i++)
	    {
	      data_out_tab[i]->done = 1;
	    }
	  /*Wake */
	  sctk_thread_mutex_unlock (&father->lock);
	}
      else
	{
	  /*Freeze */
	  sctk_thread_mutex_unlock (&father->lock);
/* 	  while (expect_true (my_vp->data.done != 1)) */
/* 	    { */
/* 	      sctk_thread_yield (); */
/* 	    } */
	  if (expect_true (my_vp->data.done != 1))
	    {
	      sctk_thread_wait_for_value ((int *) &(my_vp->data.done), 1);
	    }
	}
    }
}

static inline void
  sctk_perform_collective_communication_tree_barrier
  (sctk_collective_communications_t * restrict com,
   sctk_virtual_processor_t * restrict my_vp)
{
  sctk_virtual_processor_t *restrict father;
  sctk_nodebug ("BARRIER TREE Using vp %p com_id %d", my_vp, com->id);
  my_vp->data.done = 0;
  father = my_vp->father;
  if (expect_false (father == NULL))
    {
      if (expect_false
	  (com->nb_task_involved_in_this_process < com->nb_task_involved))
	{
	  /*Multiple process involved */
	  sctk_nodebug ("Entering net collective communication");
	  sctk_net_collective_op (com, my_vp, 0, 0, -1, NULL,
				  sctk_null_data_type);
	  sctk_nodebug ("Leaving net collective communication");
	}
      if (expect_false (com->reinitialize == 1))
	com->initialized = 0;

      /*wrong virtual processor liberation */
      if (expect_false (com->migration_list != NULL))
	{
	  sctk_thread_mutex_lock (&com->lock_migration_list);
	  {
	    sctk_migration_data_t *mig_tmp;
	    sctk_migration_data_t *mig_cur;
	    mig_tmp = (sctk_migration_data_t *) com->migration_list;
	    mig_cur = mig_tmp;
	    while (mig_cur != NULL)
	      {
		mig_tmp = mig_cur->next;
		mig_cur->done = 1;
		mig_cur = mig_tmp;
	      }
	  }
	  com->migration_list = NULL;
	  sctk_thread_mutex_unlock (&com->lock_migration_list);
	}
    }
  else
    {
      int i;
      int nb_registered;
      int nb_tasks;

/*       sctk_thread_mutex_lock (&father->lock); */
      while (expect_false (sctk_thread_mutex_trylock (&father->lock) != 0))
	sctk_thread_yield ();

      /*Contribute */
      nb_registered = father->nb_task_registered;
      nb_tasks = father->nb_task_involved;

      sctk_nodebug ("BARRIER %d registered on %d involved %p (%d)",
		    nb_registered, my_vp->nb_task_involved, my_vp, com->id);

      father->data.data_out_tab[nb_registered] = &(my_vp->data);

      nb_registered++;

      father->nb_task_registered = nb_registered;
      if (expect_false (nb_registered == nb_tasks))
	{
	  sctk_perform_collective_communication_tree_barrier (com, father);
	  father->nb_task_registered = 0;
	  sctk_nodebug("REINIT %d %s",father->nb_task_registered,__func__);

	  if (expect_true (nb_tasks == 2))
	    {
	      father->data.data_out_tab[0]->done = 1;
	      father->data.data_out_tab[1]->done = 1;
	    }
	  else
	    {
	      for (i = 0; i < nb_tasks; i++)
		{
		  father->data.data_out_tab[i]->done = 1;
		}
	    }
	  /*Wake */
	  sctk_thread_mutex_unlock (&father->lock);
	}
      else
	{
	  /*Freeze */
	  sctk_thread_mutex_unlock (&father->lock);
	  while (expect_true (my_vp->data.done != 1))
	    {
	      sctk_thread_yield ();
	    }
	}
    }
}

static void
sctk_reinit_collective_communicator (const int vp,
				   const int task_id,
				   const sctk_communicator_t com_id,
				   const int process)
{
  sctk_collective_communications_t *com;
  sctk_virtual_processor_t *my_vp;
  com = sctk_get_communicator (com_id)->collective_communications;
  sctk_nodebug ("%d tasks on %d involved START", com->nb_task_registered,
		com->nb_task_involved);
  my_vp = com->virtual_processors[vp];
  sctk_thread_mutex_lock (&my_vp->lock);
  sctk_nodebug("REGISTER 1 on this VP %d",my_vp->nb_task_registered);
  my_vp->nb_task_registered++;
  sctk_thread_mutex_unlock (&my_vp->lock);
  sctk_nodebug("REGISTER 2 on this VP %d",my_vp->nb_task_registered);
  sctk_net_reinit_communicator(task_id,com_id,vp);
  sctk_nodebug("REGISTER 3 on this VP %d",my_vp->nb_task_registered);
  sctk_init_collective_communicator(vp,task_id,com_id,process);

  sctk_thread_wait_for_value ((int *) &(com->initialized), 1);

  sctk_nodebug ("%d tasks on %d involved END %d", com->nb_task_registered,
	      com->nb_task_involved,com->initialized);
}

void
sctk_init_collective_communicator (const int vp,
				   const int task_id,
				   const sctk_communicator_t com_id,
				   const int process)
{
  sctk_collective_communications_t *com;

  sctk_nodebug ("Init collective %d on %d to %d process %d", task_id, com_id, vp,process);

  com = sctk_get_communicator (com_id)->collective_communications;

  assume(com->initialized == 0);

  com->reinitialize = 0;

  sctk_thread_mutex_lock (&com->lock);
  assume (task_id >= 0);
  com->last_vp[task_id] = vp;
  com->last_process[task_id] = process;

  com->nb_task_registered++;
  sctk_nodebug("REGISTER 4 on this VP %d",com->virtual_processors[0]->nb_task_registered);
  sctk_nodebug ("%d tasks on %d involved, %p", com->nb_task_registered,
	      com->nb_task_involved,com);
  if (com->nb_task_registered == com->nb_task_involved)
    {
      int i, cpu_number;
      int nb_thread_involved = 0;
      cpu_number = sctk_get_cpu_number ();
      for (i = 0; i < cpu_number; i++)
	{
	  sctk_deactivate_branche (com->virtual_processors[i]);
	}
#if 0
      for (i = 0; i < cpu_number; i++)
	{
	  com->virtual_processors[i]->nb_task_involved =
	    com->virtual_processors[i]->nb_task_registered;
	  com->virtual_processors[i]->nb_task_registered = 0;
	  if (com->virtual_processors[i]->nb_task_involved != 0)
	    {
	      sctk_nodebug ("Activate branche %d", i);
	      sctk_activate_branche (com->virtual_processors[i]->father);
	      nb_thread_involved +=
		com->virtual_processors[i]->nb_task_involved;
	    }
	}
#else
      for (i = 0; i < cpu_number; i++)
	{
	  com->virtual_processors[i]->nb_task_involved = 0;
	  com->virtual_processors[i]->nb_task_registered = 0;
	}
       for (i = 0; i < com->nb_task_involved; i++)
	{
	  if(com->last_process[i] == sctk_process_rank){
	    com->virtual_processors[com->last_vp[i]]->nb_task_involved++;
	  }
	  sctk_nodebug("Task %d on proc %d vp %d",i,com->last_process[i],com->last_vp[i]);
	}
      for (i = 0; i < cpu_number; i++)
	{
	  if (com->virtual_processors[i]->nb_task_involved != 0)
	    {
	      sctk_nodebug ("Activate branche %d", i);
	      sctk_activate_branche (com->virtual_processors[i]->father);
	      nb_thread_involved +=
		com->virtual_processors[i]->nb_task_involved;
	    }

	}
#endif

      com->nb_task_involved_in_this_process = nb_thread_involved;

      sctk_nodebug ("%d thread involved in this process", nb_thread_involved);
      if ((nb_thread_involved < com->nb_task_involved)
	  && (nb_thread_involved > 0))
	{
	  int j;
	  int *tmp;
	  int local_rank;
	  int local_nb;
	  int pos = 2;

	  sctk_spinlock_lock(&(collective_comm_allocator.lock));
	  tmp = (int *)
	    __sctk_malloc (sctk_process_number * sizeof (int), collective_comm_allocator.alloc);
	  sctk_spinlock_unlock(&(collective_comm_allocator.lock));
	  for (i = 0; i < sctk_process_number; i++)
	    {
	      tmp[i] = -1;
	    }
	  for (i = 0; i < sctk_process_number; i++)
	    {
	      com->involved_list[i] = 0;
	    }

	  com->receive_number = 0;
	  /*Reinit tab */
	  for (i = 0; i < sctk_process_number; i++)
	    {
	      com->receive_list[i] = -1;
	    }

	  /*Check for involved processes */
	  for (i = 0; i < com->nb_task_involved; i++)
	    {
	      if (com->last_process[i] >= 0)
		{
		  com->receive_list[com->last_process[i]] =
		    com->last_process[i];
		}
	      else
		{
		  not_reachable ();
		}
	    }

	  for (i = 0; i < sctk_process_number; i++)
	    {
	      if (com->receive_list[i] != -1)
		{
		  com->involved_list[i] = 1;
		}
	    }
	  for (i = 0; i < sctk_process_number; i++)
	    {
	      if (com->involved_list[i])
		{
		  com->involved_list[i] = 2;
		  break;
		}
	    }
	  com->involved_nb = 0;
	  for (i = 0; i < sctk_process_number; i++)
	    {
	      if (com->involved_list[i])
		{
		  com->involved_nb++;
		}
	    }
	  for (i = (sctk_process_rank + 1) % sctk_process_number;;
	       i = (i + 1) % sctk_process_number)
	    {
	      if (com->involved_list[i])
		{
		  break;
		}
	    }
	  com->involved_list_next = i;



	  /*Compaction of used processes */
	  for (i = 0; i < sctk_process_number; i++)
	    {
	      if (com->receive_list[i] == -1)
		{
		  for (j = i + 1; j < sctk_process_number; j++)
		    {
		      if (com->receive_list[j] != -1)
			{
			  com->receive_list[i] = com->receive_list[j];
			  com->receive_list[j] = -1;
			  break;
			}
		    }
		}
	      if (com->receive_list[i] != -1)
		{
		  com->receive_number++;
		}
	    }

	  local_rank = -1;
	  /*Compute local rank */
	  for (i = 0; i < sctk_process_number; i++)
	    {
	      if (com->receive_list[i] == sctk_process_rank)
		{
		  local_rank = i;
		}
	      sctk_nodebug ("local rank %d %d != %d", local_rank,
			    com->receive_list[i], sctk_process_rank);
	    }
	  sctk_nodebug ("local rank %d", local_rank);

	  local_nb = com->receive_number;
	  sctk_nodebug ("%d process involved local rank %d", local_nb,
			local_rank);
	  pos = 2;
	  com->receive_number = 0;
	  com->send_process = -1;
	  while (pos <= local_nb)
	    {
	      if (local_rank % pos != 0)
		{
		  if (local_rank != 0)
		    {
		      com->send_process =
			com->receive_list[local_rank - (local_rank % pos)];
		      sctk_nodebug ("%d send to %d", sctk_process_rank,
				    com->send_process);
		      break;
		    }
		}
	      else
		{
		  if (local_rank + (pos / 2) < local_nb)
		    {
		      tmp[com->receive_number] =
			com->receive_list[local_rank + (pos / 2)];
		      sctk_nodebug ("%d[%d] = %d", sctk_process_rank,
				    com->receive_number,
				    tmp[com->receive_number]);
		      com->receive_number++;
		    }
		}
	      pos = pos * 2;
	    }
	  if ((pos != local_nb) && (com->send_process == -1))
	    {
	      if (local_rank % pos != 0)
		{
		  if (local_rank != 0)
		    {
		      com->send_process =
			com->receive_list[local_rank - (local_rank % pos)];
		      sctk_nodebug ("%d send to %d", sctk_process_rank,
				    com->send_process);
		    }
		}
	      else
		{
		  if (local_rank + (pos / 2) < local_nb)
		    {
		      tmp[com->receive_number] =
			com->receive_list[local_rank + (pos / 2)];
		      sctk_nodebug ("%d[%d] = %d", sctk_process_rank,
				    com->receive_number,
				    tmp[com->receive_number]);
		      com->receive_number++;
		    }
		}
	      pos = pos * 2;
	    }

	  sctk_free ((void *) com->receive_list);
	  com->receive_list = tmp;
	}
      else
	{
	  com->receive_number = 0;
	  com->send_process = -1;
	}

      if (nb_thread_involved > 0)
	{
	  com->initialized = 1;
	}
      com->nb_task_registered = 0;
      sctk_nodebug ("Communicator initialised value %d",com->initialized);
    }
  sctk_thread_mutex_unlock (&com->lock);
  sctk_nodebug ("Init collective %d on %d to %d done", task_id, com_id, vp);
}

static inline int
__sctk_perform_collective_comm_intern (const size_t elem_size,
				       const size_t nb_elem,
				       const void *data_in,
				       void *const data_out,
				       void (*func) (const void *, void *,
						     size_t,
						     sctk_datatype_t),
				       const sctk_communicator_t com_id,
				       const int vp,
				       const sctk_datatype_t data_type)
{
  sctk_virtual_processor_t *my_vp;
  sctk_collective_communications_t *com;

  int nb_registered;

  com = sctk_get_communicator (com_id)->collective_communications;
  com->id = com_id;
  my_vp = com->virtual_processors[vp];

  sctk_thread_mutex_lock (&my_vp->lock);
  /*Contribute */
  nb_registered = my_vp->nb_task_registered;
  if (nb_registered == 0)
    {
      /*I am the first */
      my_vp->data.data_in = (void *) data_in;
      my_vp->data.data_out = (void *) data_out;
      if (func != NULL)
	{
	  memcpy (my_vp->data.data_out, data_in, nb_elem * elem_size);
	}
    }
  else
    {
      sctk_nodebug ("My data_in to %p", data_in);
      if (nb_elem != 0)
	{
	  if (func != NULL)
	    {
	      /*Reduction */
	      func (data_in, my_vp->data.data_out, nb_elem, data_type);
	    }
	  else
	    {
	      {
		/*Broadcast */
		if (data_in != NULL)
		  {
		    sctk_nodebug ("Reset_in to %p", data_in);
		    my_vp->data.data_in = (void *) data_in;
		    sctk_nodebug ("put int /%s/", my_vp->data.data_in);
		  }
		if (my_vp->data.data_out == NULL)
		  {
		    my_vp->data.data_out = data_out;
		    sctk_nodebug ("put out /%s/", my_vp->data.data_out);
		  }
	      }
	    }
	}
    }
  my_vp->data.data_out_tab[nb_registered] = data_out;
  nb_registered++;
  my_vp->nb_task_registered = nb_registered;
  sctk_debug ("%d registered on %d involved", nb_registered,
		my_vp->nb_task_involved);

  if (nb_registered == my_vp->nb_task_involved)
    {
      int i;
      /*Last one */
      /*Perform evaluation tree */
      sctk_debug ("Entering tree");
      sctk_perform_collective_communication_tree (com, my_vp, elem_size,
						  nb_elem, func, data_type);

      /*Broadcast result */
      if (nb_elem != 0)
	{
	  size_t tmp_size_copy;
	  char *tmp_in_tab;

	  tmp_size_copy = elem_size * nb_elem;
	  tmp_in_tab = (char *) my_vp->data.data_out;

	  for (i = 0; i < my_vp->nb_task_involved; i++)
	    {
	      char *tmp_out_tab;
	      tmp_out_tab = (char *) my_vp->data.data_out_tab[i];

	      if ((tmp_out_tab != NULL) && (tmp_out_tab != tmp_in_tab))
		memcpy (tmp_out_tab, tmp_in_tab, tmp_size_copy);
	    }
	}

      my_vp->nb_task_registered = 0;
      sctk_nodebug("REINIT %d %s",my_vp->nb_task_registered,__func__);
      /*Wake */
      sctk_debug ("Wake %p", my_vp->list);
      sctk_thread_wake_thread_on_vp ((void **) &(my_vp->list));
      sctk_debug ("Wake %p done", my_vp->list);
      sctk_thread_mutex_unlock (&my_vp->lock);
      return 0;
    }
  else
    {
      return 1;
    }

}

static inline int
__sctk_perform_collective_comm_intern_broadcast (const size_t elem_size,
						 const size_t nb_elem,
             const int root,
						 const void *data_in,
						 void *const data_out,
						 const sctk_communicator_t
						 com_id, const int vp,
						 const sctk_datatype_t
						 data_type)
{
  sctk_virtual_processor_t *restrict my_vp;
  sctk_collective_communications_t *restrict com;

  int nb_registered;
  int last;

  com = sctk_get_communicator (com_id)->collective_communications;
  com->id = com_id;
  my_vp = com->virtual_processors[vp];
  sctk_nodebug ("BCAST Use comm id %d vp %d", com_id, vp);

  sctk_thread_mutex_lock (&my_vp->lock);
  /*Contribute */
  nb_registered = my_vp->nb_task_registered;
  last = my_vp->nb_task_involved;

  if (expect_false (nb_registered == 0))
    {
      /*I am the first */
      my_vp->data.data_in = (void *) data_in;
      my_vp->data.data_out = (void *) data_out;
    }
  else
    {
      sctk_nodebug ("My data_in to %p", data_in);
      /*Broadcast */
      if (expect_false (data_in != NULL))
	{
	  sctk_nodebug ("Reset_in to %p", data_in);
	  my_vp->data.data_in = (void *) data_in;
	  sctk_nodebug ("put int /%s/", my_vp->data.data_in);
	}
      if (expect_true (my_vp->data.data_out == NULL))
	{
	  my_vp->data.data_out = data_out;
	  sctk_nodebug ("put out /%s/", my_vp->data.data_out);
	}
    }
  my_vp->data.data_out_tab[nb_registered] = data_out;
  nb_registered++;
  my_vp->nb_task_registered = nb_registered;
  sctk_nodebug ("%d registered on %d involved", nb_registered,
		my_vp->nb_task_involved);

  if (expect_false (nb_registered == last))
    {
      int i;
      size_t tmp_size_copy;
      char *tmp_in_tab;
      struct sctk_collective_communications_data_s **data_out_tab;
      /*Last one */
      /*Perform evaluation tree */
      sctk_nodebug ("Entering tree");
      sctk_perform_collective_communication_tree_broadcast (com, my_vp,
							    elem_size,
							    nb_elem,
                  root,
							    data_type);

      tmp_size_copy = elem_size * nb_elem;
      tmp_in_tab = (char *) my_vp->data.data_out;
      data_out_tab = my_vp->data.data_out_tab;

      for (i = 0; i < last; i++)
	{
	  char *tmp_out_tab;
	  tmp_out_tab = (char *) data_out_tab[i];

	  if (expect_true
	      ((tmp_out_tab != NULL) && (tmp_out_tab != tmp_in_tab)))
	    memcpy (tmp_out_tab, tmp_in_tab, tmp_size_copy);
	}


      my_vp->nb_task_registered = 0;
      sctk_nodebug("REINIT %d %s",my_vp->nb_task_registered,__func__);
      /*Wake */
      sctk_nodebug ("Wake %p", my_vp->list);
      sctk_thread_wake_thread_on_vp ((void **) &(my_vp->list));
      sctk_nodebug ("Wake %p done", my_vp->list);
      sctk_thread_mutex_unlock (&my_vp->lock);
      return 0;
    }
  else
    {
      return 1;
    }

}

static inline int
__sctk_perform_collective_comm_intern_reduction (const size_t elem_size,
						 const size_t nb_elem,
						 const void *data_in,
						 void *const data_out,
						 void (*func) (const void *,
							       void *,
							       size_t,
							       sctk_datatype_t),
						 const sctk_communicator_t
						 com_id, const int vp,
						 const sctk_datatype_t
						 data_type)
{
  sctk_virtual_processor_t *restrict my_vp;
  sctk_collective_communications_t *restrict com;

  int nb_registered;
  int last;


  com = sctk_get_communicator (com_id)->collective_communications;
  com->id = com_id;
  my_vp = com->virtual_processors[vp];
  sctk_nodebug ("REDUCTION Use comm id %d vp %d", com_id, vp);

  sctk_thread_mutex_lock (&my_vp->lock);
  /*Contribute */
  nb_registered = my_vp->nb_task_registered;
  last = my_vp->nb_task_involved;

  if (expect_false (nb_registered == 0))
    {
      /*I am the first */
      my_vp->data.data_in = (void *) data_in;
      my_vp->data.data_out = (void *) data_out;
      memcpy (my_vp->data.data_out, data_in, nb_elem * elem_size);
      sctk_nodebug ("I am the first %d/%d", nb_registered, last);
    }
  else
    {
      sctk_nodebug ("My data_in to %p", data_in);
      func (data_in, my_vp->data.data_out, nb_elem, data_type);
      sctk_nodebug ("My data_out to %p", data_out);
    }
  my_vp->data.data_out_tab[nb_registered] = data_out;
  nb_registered++;
  my_vp->nb_task_registered = nb_registered;
  sctk_nodebug ("%d registered on %d involved", nb_registered,
		my_vp->nb_task_involved);

  if (expect_false (nb_registered == last))
    {
      int i;
      size_t tmp_size_copy;
      char *tmp_in_tab;
      struct sctk_collective_communications_data_s **data_out_tab;
      /*Last one */
      /*Perform evaluation tree */
      sctk_nodebug ("Entering tree");
      sctk_perform_collective_communication_tree_reduction (com, my_vp,
							    elem_size,
							    nb_elem, func,
							    data_type);

      tmp_size_copy = elem_size * nb_elem;
      tmp_in_tab = (char *) my_vp->data.data_out;
      data_out_tab = my_vp->data.data_out_tab;

      for (i = 0; i < last; i++)
	{
	  char *tmp_out_tab;
	  tmp_out_tab = (char *) data_out_tab[i];

	  if (expect_true
	      ((tmp_out_tab != NULL) && (tmp_out_tab != tmp_in_tab)))
	    memcpy (tmp_out_tab, tmp_in_tab, tmp_size_copy);
	}


      my_vp->nb_task_registered = 0;
      sctk_nodebug("REINIT %d %s",my_vp->nb_task_registered,__func__);
      /*Wake */
      sctk_nodebug ("Wake %p", my_vp->list);
      sctk_thread_wake_thread_on_vp ((void **) &(my_vp->list));
      sctk_nodebug ("Wake %p done", my_vp->list);
      sctk_thread_mutex_unlock (&my_vp->lock);
      return 0;
    }
  else
    {
      return 1;
    }

}

static inline int
__sctk_perform_collective_comm_intern_barrier (const sctk_communicator_t
					       com_id, const int vp)
{
  sctk_virtual_processor_t *restrict my_vp;
  sctk_collective_communications_t *restrict com;

  int nb_registered;

  com = sctk_get_communicator (com_id)->collective_communications;
  com->id = com_id;
  my_vp = com->virtual_processors[vp];
  sctk_debug ("BARRIER Use comm id %d vp %d", com_id, vp);

  sctk_thread_mutex_lock (&my_vp->lock);
  /*Contribute */
  nb_registered = my_vp->nb_task_registered;
  nb_registered++;
  my_vp->nb_task_registered = nb_registered;
  sctk_debug ("%d registered on %d involved", nb_registered,
		my_vp->nb_task_involved);

  if (expect_false (nb_registered == my_vp->nb_task_involved))
    {
      /*Last one */
      /*Perform evaluation tree */
      sctk_nodebug ("Entering tree");
      sctk_perform_collective_communication_tree_barrier (com, my_vp);

      my_vp->nb_task_registered = 0;
      sctk_nodebug("REINIT %d %s",my_vp->nb_task_registered,__func__);
      /*Wake */
      sctk_debug ("Wake %p", my_vp->list);
      sctk_thread_wake_thread_on_vp ((void **) &(my_vp->list));
      sctk_debug ("Wake %p done", my_vp->list);
      sctk_thread_mutex_unlock (&my_vp->lock);
      return 0;
    }
  else
    {
      return 1;
    }
}

void
sctk_perform_collective_communication_rpc (const size_t elem_size,
					   const size_t nb_elem,
					   const void *data_in,
					   void *const data_out,
					   void (*func) (const void *,
							 void *, size_t,
							 sctk_datatype_t),
					   const sctk_communicator_t com_id,
					   const int vp, const int task_id,
					   const sctk_datatype_t data_type)
{
  sctk_migration_data_t mig_data;
  sctk_collective_communications_t *com;

  com = sctk_get_communicator (com_id)->collective_communications;

  com->reinitialize = 1;
  /*Prepare for waiting here */
  mig_data.done = 0;
  mig_data.data = data_out;
  sctk_thread_mutex_lock (&com->lock_migration_list);
  mig_data.next = (sctk_migration_data_t *) com->migration_list;
  com->migration_list = &mig_data;
  sctk_thread_mutex_unlock (&com->lock_migration_list);

  sctk_nodebug ("Add %p %p %d", &mig_data, com, com->reinitialize);

  /*Distant vp contribution */
  if (__sctk_perform_collective_comm_intern (elem_size, nb_elem,
					     data_in, data_out, func,
					     com_id,
					     com->last_vp[task_id],
					     data_type) == 1)
    {
      sctk_virtual_processor_t *my_vp;
      my_vp = com->virtual_processors[com->last_vp[task_id]];
      sctk_thread_mutex_unlock (&my_vp->lock);
    }

  /*Wait here */
  sctk_thread_wait_for_value ((int *) &(mig_data.done), 1);
  sctk_nodebug ("Add %p %p %d done", &mig_data, com, com->reinitialize);

  sctk_nodebug ("Previous virtual processor %d, actual %d done %d",
		com->last_vp[task_id], vp, com->initialized);
  sctk_nodebug ("Initialized ? %d", com->initialized);
}

/*red func void(*func)(const void*input,void*output,size_t nb_elem);*/
static void
sctk_perform_collective_communication_migration (sctk_virtual_processor_t *
						 my_vp,
						 sctk_collective_communications_t
						 * com,
						 const size_t elem_size,
						 const size_t nb_elem,
						 const void *data_in,
						 void *const data_out,
						 void (*func) (const void *,
							       void *,
							       size_t,
							       sctk_datatype_t),
						 const sctk_communicator_t
						 com_id, const int vp,
						 const int task_id,
						 const sctk_datatype_t
						 data_type)
{
  /*Migration */
  sctk_nodebug ("Task %p migration", sctk_thread_self ());

  if (vp != com->last_vp[task_id] &&
      (sctk_process_rank == com->last_process[task_id]))
    {
      sctk_migration_data_t mig_data;
      sctk_nodebug ("Previous virtual processor %d, actual %d",
		    com->last_vp[task_id], vp);
/*       sctk_warning("Migration part (inter-virtual processors) of collective communication not implemented"); */

      com->reinitialize = 1;
      /*Prepare for waiting here */
      mig_data.done = 0;
      mig_data.data = data_out;
      sctk_thread_mutex_lock (&com->lock_migration_list);
      mig_data.next = (sctk_migration_data_t *) com->migration_list;
      com->migration_list = &mig_data;
      sctk_thread_mutex_unlock (&com->lock_migration_list);

      sctk_nodebug ("Add %p %p %d", &mig_data, com, com->reinitialize);

      /*Distant vp contribution */
      if (__sctk_perform_collective_comm_intern (elem_size, nb_elem,
						 data_in, data_out, func,
						 com_id,
						 com->last_vp[task_id],
						 data_type) == 1)
	{
	  sctk_virtual_processor_t *my_vp_l;
	  my_vp_l = com->virtual_processors[com->last_vp[task_id]];
	  sctk_thread_mutex_unlock (&my_vp_l->lock);
	}
      sctk_thread_proc_migration (vp);

      /*Wait here */
      sctk_thread_wait_for_value ((int *) &(mig_data.done), 1);
      sctk_nodebug ("Previous virtual processor %d, actual %d done %d",
		    com->last_vp[task_id], vp, com->initialized);
      sctk_assert (com->initialized == 0);
      sctk_nodebug ("Initialized ? %d", com->initialized);
      return;

    }
  else
    {
      sctk_migration_data_t mig_data;
      com->reinitialize = 1;
/* 	  sctk_warning */
/* 	    ("Migration part (inter-processus) (%d->%d) of collective communication not implemented", */
/* 	     sctk_process_rank, com->last_process[task_id]); */

      sctk_nodebug ("MIGRATION COLL OP %p %p %p %lu %lu", data_in, data_out,
		    func, elem_size, nb_elem);
      if ((data_in == NULL) && (data_out == NULL)
	  && (nb_elem * elem_size != 0))
	{
	  assume (0);
	}

      /*Prepare for waiting here */
      mig_data.done = 0;
      mig_data.data = data_out;
      sctk_thread_mutex_lock (&com->lock_migration_list);
      mig_data.next = (sctk_migration_data_t *) com->migration_list;
      com->migration_list = &mig_data;
      sctk_thread_mutex_unlock (&com->lock_migration_list);

      /*Distant contribution */
      sctk_rpc_collective_op (elem_size,
			      nb_elem,
			      data_in,
			      data_out,
			      func,
			      com_id,
			      com->last_vp[task_id],
			      task_id, data_type, com->last_process[task_id]);

      /*Wait here */
      if (com->nb_task_involved_in_this_process > 0)
	{
	  /*There's other task in this process */
	  sctk_thread_wait_for_value ((int *) &(mig_data.done), 1);
	}
      else
	{
	  /*I'am the only one in this communicator */
	  not_implemented ();
	}

      sctk_nodebug ("Initialized ? %d", com->initialized);
      return;
    }
}
static void
sctk_perform_collective_communication_init (const size_t elem_size,
				       const size_t nb_elem,
				       const void *data_in,
				       void *const data_out,
				       void (*func) (const void *, void *,
						     size_t,
						     sctk_datatype_t),
				       const sctk_communicator_t com_id,
				       const int vp, const int task_id,
				       const sctk_datatype_t data_type)
{
  sctk_virtual_processor_t *my_vp;
  sctk_collective_communications_t *com;

  sctk_nodebug ("COLL OP %p %p %p %lu %lu", data_in, data_out, func,
		elem_size, nb_elem);

  if ((data_in == NULL) && (data_out == NULL) && (nb_elem * elem_size != 0))
    {
      assume (0);
    }

  com = sctk_get_communicator (com_id)->collective_communications;
  com->id = com_id;
  my_vp = com->virtual_processors[vp];
  sctk_debug("Com id : %d vp %d init %d", com_id,vp,com->initialized);

  if (com->initialized == 0)
    {
      sctk_thread_mutex_lock (&my_vp->lock);
      my_vp->nb_task_registered++;
      sctk_thread_mutex_unlock (&my_vp->lock);
      sctk_net_update_communicator (task_id, com_id, vp);
      sctk_init_collective_communicator (vp, task_id, com_id,
					 sctk_process_rank);
      sctk_thread_wait_for_value ((int *) &(com->initialized), 1);
    }
  sctk_debug("Init done");

  my_vp = com->virtual_processors[vp];

  if ((vp != com->last_vp[task_id]) ||
      (sctk_process_rank != com->last_process[task_id]))
    {
      sctk_debug("Migration");
      sctk_perform_collective_communication_migration (my_vp, com,
						       elem_size, nb_elem,
						       data_in, data_out,
						       func, com_id, vp,
						       task_id, data_type);
      return;
    }

  if (__sctk_perform_collective_comm_intern (elem_size, nb_elem,
					     data_in, data_out, func,
					     com_id, vp, data_type) == 1)
    {
      /*Freeze */
      sctk_thread_freeze_thread_on_vp (&(my_vp->lock),
				       (void **) &(my_vp->list));
    }
}

void
sctk_perform_collective_communication_broadcast (const size_t elem_size,
						 const size_t nb_elem,
             const int root,
						 const void *data_in,
						 void *const data_out,
						 const sctk_communicator_t
						 com_id, const int vp,
						 const int task_id,
						 const sctk_datatype_t
						 data_type)
{
  sctk_virtual_processor_t *my_vp;
  sctk_collective_communications_t *com;

  com = sctk_get_communicator (com_id)->collective_communications;
  com->id = com_id;
  my_vp = com->virtual_processors[vp];

  if (expect_false (com->initialized == 0))
    {
      sctk_reinit_collective_communicator (vp, task_id, com_id,
					 sctk_process_rank);
    }

  my_vp = com->virtual_processors[vp];

  if (expect_false ((vp != com->last_vp[task_id]) ||
		    (sctk_process_rank != com->last_process[task_id])))
    {
      sctk_perform_collective_communication_migration (my_vp, com,
						       elem_size, nb_elem,
						       data_in, data_out,
						       NULL, com_id, vp,
						       task_id, data_type);
      return;
    }

  if (expect_true
      (__sctk_perform_collective_comm_intern_broadcast
       (elem_size, nb_elem, root, data_in, data_out, com_id, vp, data_type) == 1))
    {
      /*Freeze */
      sctk_thread_freeze_thread_on_vp (&(my_vp->lock),
				       (void **) &(my_vp->list));
    }
}

void
sctk_perform_collective_communication_reduction (const size_t elem_size,
						 const size_t nb_elem,
						 const void *data_in,
						 void *const data_out,
						 void (*func) (const void *,
							       void *,
							       size_t,
							       sctk_datatype_t),
						 const sctk_communicator_t
						 com_id, const int vp,
						 const int task_id,
						 const sctk_datatype_t
						 data_type)
{
  sctk_virtual_processor_t *my_vp;
  sctk_collective_communications_t *com;

  com = sctk_get_communicator (com_id)->collective_communications;
  com->id = com_id;
  my_vp = com->virtual_processors[vp];

  if (expect_false (com->initialized == 0))
    {
      sctk_reinit_collective_communicator (vp, task_id, com_id,
					 sctk_process_rank);
    }

  my_vp = com->virtual_processors[vp];

  if (expect_false ((vp != com->last_vp[task_id]) ||
		    (sctk_process_rank != com->last_process[task_id])))
    {
      sctk_perform_collective_communication_migration (my_vp, com,
						       elem_size, nb_elem,
						       data_in, data_out,
						       func, com_id, vp,
						       task_id, data_type);
      return;
    }

  if (expect_true
      (__sctk_perform_collective_comm_intern_reduction
       (elem_size, nb_elem, data_in, data_out, func, com_id, vp,
	data_type) == 1))
    {
      /*Freeze */
      sctk_thread_freeze_thread_on_vp (&(my_vp->lock),
				       (void **) &(my_vp->list));
    }
}

void
sctk_perform_collective_communication_barrier (const sctk_communicator_t
					       com_id, const int vp,
					       const int task_id)
{
  sctk_virtual_processor_t *my_vp;
  sctk_collective_communications_t *com;


  com = sctk_get_communicator (com_id)->collective_communications;
  sctk_nodebug("%d begin barrier init done %d",task_id,com->initialized);
  com->id = com_id;
  my_vp = com->virtual_processors[vp];

  if (expect_false (com->initialized == 0))
    {
      sctk_reinit_collective_communicator (vp, task_id, com_id,
					 sctk_process_rank);
    }

  my_vp = com->virtual_processors[vp];

  if (expect_false ((vp != com->last_vp[task_id]) ||
		    (sctk_process_rank != com->last_process[task_id])))
    {
      sctk_perform_collective_communication_migration (my_vp, com, 0,
						       0,
						       NULL,
						       NULL,
						       NULL,
						       com_id,
						       vp, task_id,
						       sctk_null_data_type);
      sctk_nodebug("%d end barrier",task_id);
      return;
    }

  if (expect_true
      (__sctk_perform_collective_comm_intern_barrier (com_id, vp) == 1))
    {
      /*Freeze */
      sctk_thread_freeze_thread_on_vp (&(my_vp->lock),
				       (void **) &(my_vp->list));
    }
  sctk_nodebug("%d end barrier",task_id);
}

void
sctk_terminaison_barrier (const int id)
{
/*  sctk_collective_communications_t* com;*/
  int vp;

/*  com = sctk_get_communicator(SCTK_COMM_WORLD)->collective_communications;*/

  vp = sctk_thread_get_vp ();

  sctk_nodebug ("sctk_terminaison_barrier %d", id);
  sctk_perform_collective_communication_init (0, 0, NULL,
					      NULL, NULL,
					      SCTK_COMM_WORLD,
					      vp, id, sctk_null_data_type);
  sctk_nodebug ("sctk_terminaison_barrier %d done", id);
}
