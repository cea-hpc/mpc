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
#include "sctk_debug.h"
#include "sctk_inter_thread_comm.h"
#include "sctk_thread.h"
#include "sctk_alloc.h"
#include "sctk_asm.h"
#include "sctk_low_level_comm.h"
#include "sctk_hybrid_comm.h"
#include "sctk_accessor.h"

#define YIELD \
  /* sctk_thread_yield(); */  \
  /* Pool IB messages */  \
  sctk_net_hybrid_ptp_poll(NULL);

static sctk_ptp_data_t *sctk_ptp_list;
static volatile int *volatile sctk_ptp_process_localisation = NULL;
static int sctk_total_task_number = 0;

static sctk_messages_alloc_thread_data_t inter_thread_comm_allocator;

#define SCTK_LOCAL_VERSION_MAJOR 0
#define SCTK_LOCAL_VERSION_MINOR 1


static inline void __sctk_optimized_memcpy (void * dest, const void * src, size_t n)
{
#if !defined(NO_INTERNAL_ASSERT) && defined(MPC_Allocator)
  sctk_check_address(dest,n);
  sctk_check_address(src,n);
#endif
  memcpy(dest,src,n);
}

static inline void
sctk_thread_ptp_message_list_init (sctk_thread_ptp_message_list_t * list)
{
  list->head = NULL;
  list->tail = NULL;
#ifdef USE_OWNER_FLAG
  list->owner = NULL;
#endif
  list->lock = SCTK_SPINLOCK_INITIALIZER;
}

static inline void
sctk_thread_ptp_message_list_lock (sctk_thread_ptp_message_list_t * list)
{
  sctk_spinlock_lock (&(list->lock));
#ifdef USE_OWNER_FLAG
  list->owner = sctk_thread_self ();
#endif
}

static inline int
sctk_thread_ptp_message_list_trylock (sctk_thread_ptp_message_list_t * list)
{
  int tmp;

  tmp = sctk_spinlock_trylock (&(list->lock));
#ifdef USE_OWNER_FLAG
  if(tmp == 0){
    list->owner = sctk_thread_self ();
  }
#endif
  return tmp;
}

static inline void
sctk_thread_ptp_message_list_unlock (sctk_thread_ptp_message_list_t * list)
{
#ifdef USE_OWNER_FLAG
  assume(list->owner == sctk_thread_self ());
  list->owner = NULL;
#endif
  sctk_spinlock_unlock (&(list->lock));
}

static inline void
sctk_thread_ptp_message_list_assert_locked (sctk_thread_ptp_message_list_t *
					    list)
{
  assume ((list->lock != SCTK_SPINLOCK_INITIALIZER)
#ifdef USE_OWNER_FLAG
	  && (list->owner == sctk_thread_self ())
#endif
    );
}

static void
sctk_ptp_per_task_init_com (sctk_task_ptp_data_t * task, int k)
{

  task->source_task = k;

  sctk_thread_ptp_message_list_init (&(task->sctk_send_list));
/*   task->send_list = NULL; */
/*   task->send_list_tail = NULL; */

  sctk_thread_ptp_message_list_init (&(task->sctk_recv_list));
/*   task->recv_list = NULL; */
/*   task->recv_list_tail = NULL; */

  task->busy = 0;
  task->matched = NULL;
  task->spinlock_matched = 0;

  task->is_usable = 1;
  task->spinlock = 0;

  task->rank_send = 0;
  task->rank_recv = 0;
}

void
sctk_ptp_per_task_init (int i)
{
  int j, k;
  int nb_task;

  sctk_ptp_list[i].allocator.alloc = __sctk_create_thread_memory_area ();
  sctk_ptp_list[i].allocator.lock = 0;

  sctk_spinlock_lock(&(sctk_ptp_list[i].allocator.lock));

  nb_task = sctk_total_task_number;

  for (j = 0; j < SCTK_MAX_COMMUNICATOR_NUMBER; j++)
    {
      sctk_thread_ptp_message_list_init (&
					 (sctk_ptp_list[i].communicators[j].
					  sctk_wait_list));
      sctk_thread_ptp_message_list_init (&
					 (sctk_ptp_list[i].communicators[j].
					  sctk_wait_send_list));
      sctk_thread_ptp_message_list_init (&
					 (sctk_ptp_list[i].communicators[j].
					  sctk_any_source_list));
      sctk_ptp_list[i].communicators[j].rank_recv = 0;
      sctk_ptp_list[i].communicators[j].tasks = (sctk_task_ptp_data_t *)
	__sctk_calloc (nb_task, sizeof (sctk_task_ptp_data_t), sctk_ptp_list[i].allocator.alloc);
      assume (sctk_ptp_list[i].communicators[j].tasks != NULL);

      sctk_ptp_list[i].communicators[j].nb_tasks = nb_task;
      for (k = 0; k < nb_task; k++)
	{
	  sctk_ptp_per_task_init_com (&
				      (sctk_ptp_list[i].communicators[j].
				       tasks[k]), k);
	}
    }
  sctk_ptp_list[i].is_usable = 1;
  sctk_spinlock_unlock(&(sctk_ptp_list[i].allocator.lock));
}

void
sctk_ptp_init (const int nb_task)
{
  int i;
  sctk_only_once ();
  sctk_total_task_number = nb_task;
  sctk_print_version ("PtP", SCTK_LOCAL_VERSION_MAJOR,
		      SCTK_LOCAL_VERSION_MINOR);
  inter_thread_comm_allocator.alloc = __sctk_create_thread_memory_area ();
  inter_thread_comm_allocator.lock = 0;

  sctk_spinlock_lock(&inter_thread_comm_allocator.lock);
  sctk_ptp_list =
    (sctk_ptp_data_t *) __sctk_calloc (nb_task, sizeof (sctk_ptp_data_t),
				       inter_thread_comm_allocator.alloc);
  assume (sctk_ptp_list != NULL);

  sctk_ptp_process_localisation =
    (int *) __sctk_calloc (nb_task, sizeof (int), inter_thread_comm_allocator.alloc);
  assume (sctk_ptp_process_localisation != NULL);
  sctk_spinlock_unlock(&inter_thread_comm_allocator.lock);

  for (i = 0; i < nb_task; i++)
    {
      sctk_ptp_process_localisation[i] = -1;
    }
  for (i = 0; i < nb_task; i++)
    {
      sctk_ptp_list[i].allocator.alloc = NULL;
      sctk_ptp_list[i].allocator.lock = 0;
      sctk_ptp_list[i].free_list.free_list = NULL;
      sctk_ptp_list[i].free_list.lock = 0;
      sctk_ptp_list[i].is_usable = 0;
    }
}

void
sctk_ptp_delete ()
{
  int i, j;
  for (i = 0; i < sctk_total_task_number; i++)
    {
      for (j = 0; j < SCTK_MAX_COMMUNICATOR_NUMBER; j++)
	{
	  sctk_free ((void *) (sctk_ptp_list[i].communicators[j].tasks));
	}
    }
  sctk_free (sctk_ptp_list);
  sctk_free ((void *) sctk_ptp_process_localisation);
}

static inline void
sctk_copy_buffer_std_std (sctk_pack_indexes_t * restrict in_begins,
			  sctk_pack_indexes_t * restrict in_ends,
			  size_t in_sizes,
			  void *restrict in_adress,
			  size_t in_elem_size,
			  sctk_pack_indexes_t * restrict out_begins,
			  sctk_pack_indexes_t * restrict out_ends,
			  size_t out_sizes,
			  void *restrict out_adress, size_t out_elem_size)
{
  sctk_pack_indexes_t tmp_begin[1];
  sctk_pack_indexes_t tmp_end[1];
  if ((in_begins == NULL) && (out_begins == NULL))
    {
      sctk_nodebug ("sctk_copy_buffer_std_std no mpc_pack");
      sctk_nodebug ("%s == %s", out_adress, in_adress);
      __sctk_optimized_memcpy (out_adress, in_adress, in_sizes);
      sctk_nodebug ("%s == %s", out_adress, in_adress);
    }
  else
    {
      unsigned long i;
      unsigned long j;
      unsigned long in_i;
      unsigned long in_j;
      sctk_nodebug ("sctk_copy_buffer_std_std mpc_pack");
      if (in_begins == NULL)
	{
	  in_begins = tmp_begin;
	  in_begins[0] = 0;
	  in_ends = tmp_end;
	  in_ends[0] = in_sizes - 1;
	  in_elem_size = 1;
	  in_sizes = 1;
	}
      if (out_begins == NULL)
	{
	  out_begins = tmp_begin;
	  out_begins[0] = 0;
	  out_ends = tmp_end;
	  out_ends[0] = out_sizes - 1;
	  out_elem_size = 1;
	  out_sizes = 1;
	}
      in_i = 0;
      in_j = in_begins[in_i] * in_elem_size;
      for (i = 0; i < out_sizes; i++)
	{
	  for (j = out_begins[i] * out_elem_size;
	       j <= out_ends[i] * out_elem_size;)
	    {
	      size_t max_length;
	      if (in_j > in_ends[in_i] * in_elem_size)
		{
		  in_i++;
		  if (in_i >= in_sizes)
		    {
		      return;
		    }
		  in_j = in_begins[in_i] * in_elem_size;
		}

	      max_length =
		sctk_min ((out_ends[i] * out_elem_size - j +
			   out_elem_size),
			  (in_ends[in_i] * in_elem_size - in_j +
			   in_elem_size));

	      __sctk_optimized_memcpy (&(((char *) out_adress)[j]),
		      &(((char *) in_adress)[in_j]), max_length);
	      sctk_nodebug ("Copy out[%d-%d]%s == in[%d-%d]%s", j,
			    j + max_length, &(((char *) out_adress)[j]),
			    in_j, in_j + max_length,
			    &(((char *) in_adress)[in_j]));

	      j += max_length;
	      in_j += max_length;

	    }
	}
    }
}

static inline void
sctk_copy_buffer_absolute_absolute (sctk_pack_absolute_indexes_t *
				    restrict in_begins,
				    sctk_pack_absolute_indexes_t *
				    restrict in_ends, size_t in_sizes,
				    void *restrict in_adress,
				    size_t in_elem_size,
				    sctk_pack_absolute_indexes_t *
				    restrict out_begins,
				    sctk_pack_absolute_indexes_t *
				    restrict out_ends, size_t out_sizes,
				    void *restrict out_adress,
				    size_t out_elem_size)
{
  sctk_pack_absolute_indexes_t tmp_begin[1];
  sctk_pack_absolute_indexes_t tmp_end[1];
  if ((in_begins == NULL) && (out_begins == NULL))
    {
      sctk_nodebug ("sctk_copy_buffer_absolute_absolute no mpc_pack");
      sctk_nodebug ("%s == %s", out_adress, in_adress);
      __sctk_optimized_memcpy (out_adress, in_adress, in_sizes);
      sctk_nodebug ("%s == %s", out_adress, in_adress);
    }
  else
    {
      unsigned long i;
      unsigned long j;
      unsigned long in_i;
      unsigned long in_j;
      sctk_nodebug ("sctk_copy_buffer_absolute_absolute mpc_pack");
      if (in_begins == NULL)
	{
	  in_begins = tmp_begin;
	  in_begins[0] = 0;
	  in_ends = tmp_end;
	  in_ends[0] = in_sizes - 1;
	  in_elem_size = 1;
	  in_sizes = 1;
	}
      if (out_begins == NULL)
	{
	  out_begins = tmp_begin;
	  out_begins[0] = 0;
	  out_ends = tmp_end;
	  out_ends[0] = out_sizes - 1;
	  out_elem_size = 1;
	  out_sizes = 1;
	}
      in_i = 0;
      in_j = in_begins[in_i] * in_elem_size;

/*       { */
/* 	for(i = 0 ; i < in_sizes; i++){ */
/* 	  sctk_debug("in %lu %lu-%lu",i, in_begins[i],in_ends[i]); */
/* 	} */
/* 	for(i = 0 ; i < out_sizes; i++){ */
/* 	  sctk_debug("out %lu %lu-%lu",i, out_begins[i],out_ends[i]); */
/* 	} */
/*       } */

      for (i = 0; i < out_sizes; i++)
	{
	  for (j = out_begins[i] * out_elem_size;
	       j <= out_ends[i] * out_elem_size;)
	    {
	      size_t max_length;
	      if (in_j > in_ends[in_i] * in_elem_size)
		{
		  in_i++;
		  if (in_i >= in_sizes)
		    {
		      return;
		    }
		  in_j = in_begins[in_i] * in_elem_size;
		}

	      max_length =
		sctk_min ((out_ends[i] * out_elem_size - j +
			   out_elem_size),
			  (in_ends[in_i] * in_elem_size - in_j +
			   in_elem_size));

	      sctk_nodebug ("Copy out[%lu-%lu]%p == in[%lu-%lu]%p", j,
			    j + max_length, &(((char *) out_adress)[j]),
			    in_j, in_j + max_length,
			    &(((char *) in_adress)[in_j]));
	      __sctk_optimized_memcpy (&(((char *) out_adress)[j]),
		      &(((char *) in_adress)[in_j]), max_length);
	      sctk_nodebug ("Copy out[%d-%d]%d == in[%d-%d]%d", j,
			    j + max_length, (((char *) out_adress)[j]),
			    in_j, in_j + max_length,
			    (((char *) in_adress)[in_j]));

	      j += max_length;
	      in_j += max_length;
	    }
	}
    }
}

static inline void
sctk_copy_buffer_absolute_std (sctk_pack_absolute_indexes_t *
			       restrict in_begins,
			       sctk_pack_absolute_indexes_t *
			       restrict in_ends, size_t in_sizes,
			       void *restrict in_adress,
			       size_t in_elem_size,
			       sctk_pack_indexes_t * restrict out_begins,
			       sctk_pack_indexes_t * restrict out_ends,
			       size_t out_sizes, void *restrict out_adress,
			       size_t out_elem_size)
{
  sctk_pack_indexes_t tmp_begin[1];
  sctk_pack_indexes_t tmp_end[1];
  sctk_pack_absolute_indexes_t tmp_begin_absolute[1];
  sctk_pack_absolute_indexes_t tmp_end_absolute[1];
  if ((in_begins == NULL) && (out_begins == NULL))
    {
      sctk_nodebug ("sctk_copy_buffer_absolute_std no mpc_pack");
      sctk_nodebug ("%s == %s", out_adress, in_adress);
      __sctk_optimized_memcpy (out_adress, in_adress, in_sizes);
      sctk_nodebug ("%s == %s", out_adress, in_adress);
    }
  else
    {
      unsigned long i;
      unsigned long j;
      unsigned long in_i;
      unsigned long in_j;
      sctk_nodebug ("sctk_copy_buffer_absolute_std  mpc_pack");
      if (in_begins == NULL)
	{
	  in_begins = tmp_begin_absolute;
	  in_begins[0] = 0;
	  in_ends = tmp_end_absolute;
	  in_ends[0] = in_sizes - 1;
	  in_elem_size = 1;
	  in_sizes = 1;
	}
      if (out_begins == NULL)
	{
	  out_begins = tmp_begin;
	  out_begins[0] = 0;
	  out_ends = tmp_end;
	  out_ends[0] = out_sizes - 1;
	  out_elem_size = 1;
	  out_sizes = 1;
	}
      in_i = 0;
      in_j = in_begins[in_i] * in_elem_size;

      for (i = 0; i < out_sizes; i++)
	{
	  sctk_nodebug ("Step %lu/%lu in out size", i, out_sizes);
	  for (j = out_begins[i] * out_elem_size;
	       j <= out_ends[i] * out_elem_size;)
	    {
	      size_t max_length;
	      if (in_j > in_ends[in_i] * in_elem_size)
		{
		  in_i++;
		  if (in_i >= in_sizes)
		    {
		      return;
		    }
		  in_j = in_begins[in_i] * in_elem_size;
		}

	      max_length =
		sctk_min ((out_ends[i] * out_elem_size - j +
			   out_elem_size),
			  (in_ends[in_i] * in_elem_size - in_j +
			   in_elem_size));

	      __sctk_optimized_memcpy (&(((char *) out_adress)[j]),
		      &(((char *) in_adress)[in_j]), max_length);
	      sctk_nodebug ("Copy out[%d-%d]%s == in[%d-%d]%s", j,
			    j + max_length, &(((char *) out_adress)[j]),
			    in_j, in_j + max_length,
			    &(((char *) in_adress)[in_j]));

	      j += max_length;
	      in_j += max_length;
	    }
	}
    }
}

static inline void
sctk_copy_buffer_std_absolute (sctk_pack_indexes_t * restrict in_begins,
			       sctk_pack_indexes_t * restrict in_ends,
			       size_t in_sizes,
			       void *restrict in_adress,
			       size_t in_elem_size,
			       sctk_pack_absolute_indexes_t *
			       restrict out_begins,
			       sctk_pack_absolute_indexes_t *
			       restrict out_ends, size_t out_sizes,
			       void *restrict out_adress,
			       size_t out_elem_size)
{
  sctk_pack_indexes_t tmp_begin[1];
  sctk_pack_indexes_t tmp_end[1];
  sctk_pack_absolute_indexes_t tmp_begin_absolute[1];
  sctk_pack_absolute_indexes_t tmp_end_absolute[1];
  if ((in_begins == NULL) && (out_begins == NULL))
    {
      sctk_nodebug ("sctk_copy_buffer_std_absolute no mpc_pack");
      sctk_nodebug ("%s == %s", out_adress, in_adress);
      __sctk_optimized_memcpy (out_adress, in_adress, in_sizes);
      sctk_nodebug ("%s == %s", out_adress, in_adress);
    }
  else
    {
      unsigned long i;
      unsigned long j;
      unsigned long in_i;
      unsigned long in_j;
      sctk_nodebug ("sctk_copy_buffer_std_absolute mpc_pack");
      if (in_begins == NULL)
	{
	  in_begins = tmp_begin;
	  in_begins[0] = 0;
	  in_ends = tmp_end;
	  in_ends[0] = in_sizes - 1;
	  in_elem_size = 1;
	  in_sizes = 1;
	}
      if (out_begins == NULL)
	{
	  out_begins = tmp_begin_absolute;
	  out_begins[0] = 0;
	  out_ends = tmp_end_absolute;
	  out_ends[0] = out_sizes - 1;
	  out_elem_size = 1;
	  out_sizes = 1;
	}
      in_i = 0;
      in_j = in_begins[in_i] * in_elem_size;
      for (i = 0; i < out_sizes; i++)
	{
	  sctk_nodebug ("i = %d", i);
	  for (j = out_begins[i] * out_elem_size;
	       j <= out_ends[i] * out_elem_size;)
	    {
	      size_t max_length;
	      sctk_nodebug ("j = %d in _j %d in_sizes %lu", j, in_j,
			    in_sizes);
	      if (in_j > in_ends[in_i] * in_elem_size)
		{
		  in_i++;
		  if (in_i >= in_sizes)
		    {
		      return;
		    }
		  in_j = in_begins[in_i] * in_elem_size;
		}

	      max_length =
		sctk_min ((out_ends[i] * out_elem_size - j +
			   out_elem_size),
			  (in_ends[in_i] * in_elem_size - in_j +
			   in_elem_size));

	      __sctk_optimized_memcpy (&(((char *) out_adress)[j]),
		      &(((char *) in_adress)[in_j]), max_length);
	      sctk_nodebug ("Copy out[%d-%d]%s == in[%d-%d]%s", j,
			    j + max_length, &(((char *) out_adress)[j]),
			    in_j, in_j + max_length,
			    &(((char *) in_adress)[in_j]));
	      sctk_nodebug ("%d, %d", max_length, out_sizes);

	      j += max_length;
	      in_j += max_length;
	    }
	}
    }
}

static inline void
sctk_copy_buffer (sctk_pack_indexes_t * restrict in_begins,
		  sctk_pack_indexes_t * restrict in_ends,
		  sctk_pack_absolute_indexes_t *
		  restrict in_begins_absolute,
		  sctk_pack_absolute_indexes_t * restrict in_ends_absolute,
		  size_t in_sizes, void *restrict in_adress,
		  size_t in_elem_size,
		  sctk_pack_indexes_t * restrict out_begins,
		  sctk_pack_indexes_t * restrict out_ends,
		  sctk_pack_absolute_indexes_t *
		  restrict out_begins_absolute,
		  sctk_pack_absolute_indexes_t * restrict out_ends_absolute,
		  size_t out_sizes, void *restrict out_adress,
		  size_t out_elem_size)
{
  sctk_nodebug ("in %p out %p", in_adress, out_adress);
  if ((in_begins_absolute == NULL) && (out_begins_absolute == NULL))
    {
      sctk_copy_buffer_std_std (in_begins,
				in_ends,
				in_sizes,
				in_adress,
				in_elem_size,
				out_begins,
				out_ends,
				out_sizes, out_adress, out_elem_size);
    }
  else if ((in_begins_absolute != NULL) && (out_begins_absolute != NULL))
    {
      sctk_copy_buffer_absolute_absolute (in_begins_absolute,
					  in_ends_absolute, in_sizes,
					  in_adress, in_elem_size,
					  out_begins_absolute,
					  out_ends_absolute, out_sizes,
					  out_adress, out_elem_size);
    }
  else if ((in_begins_absolute == NULL) && (out_begins_absolute != NULL))
    {
      sctk_copy_buffer_std_absolute (in_begins, in_ends, in_sizes,
				     in_adress, in_elem_size,
				     out_begins_absolute,
				     out_ends_absolute, out_sizes,
				     out_adress, out_elem_size);
    }
  else
    {
      sctk_copy_buffer_absolute_std (in_begins_absolute,
				     in_ends_absolute,
				     in_sizes,
				     in_adress,
				     in_elem_size,
				     out_begins,
				     out_ends,
				     out_sizes, out_adress, out_elem_size);
    }
}

static inline void
sctk_copy_message_net (sctk_thread_ptp_message_t * restrict dest,
		       sctk_thread_ptp_message_t * restrict src)
{
  size_t i;
  char *cursor;
  size_t done = 0;
  sctk_nodebug ("Copy a message");
  cursor = src->net_mesg;

  if (cursor != (char *) (-1))
    {
      for (i = 0; i < dest->message.nb_items; i++)
	{
	  if (dest->message.begins_absolute[i] != NULL)
	    {
	      size_t j;
	      char *tmp;
	      size_t offset;
	      size_t size;
	      sctk_nodebug ("Copy a message pack");
	      for (j = 0; j < dest->message.sizes[i]; j++)
		{
		  tmp = dest->message.adresses[i];
		  offset = dest->message.begins_absolute[i][j] *
		    dest->message.elem_sizes[i];

		  size =
		    (dest->message.ends_absolute[i][j] -
		     dest->message.begins_absolute[i][j] + 1) *
		    (dest->message.elem_sizes[i]);

		  if (done + size > src->header.msg_size)
		    {
		      size = src->header.msg_size - done;
		    }

		  __sctk_optimized_memcpy (&(tmp[offset]), cursor, size);
		  cursor += size;
		  done += size;
		}
	    }
	  else
	    {
	      if (dest->message.begins[i] == NULL)
		{
		  size_t size;
		  sctk_nodebug ("Copy a message no pack");
		  sctk_nodebug ("%s == %s", dest->message.adresses[i],
				cursor);

		  size = dest->message.sizes[i];

		  if (done + size > src->header.msg_size)
		    {
		      size = src->header.msg_size - done;
		    }

		  __sctk_optimized_memcpy (dest->message.adresses[i], cursor, size);
		  cursor += size;
		  done += size;
		}
	      else
		{
		  size_t j;
		  char *tmp;
		  size_t offset;
		  size_t size;
		  sctk_nodebug ("Copy a message pack");
		  for (j = 0; j < dest->message.sizes[i]; j++)
		    {
		      tmp = dest->message.adresses[i];
		      offset = dest->message.begins[i][j] *
			dest->message.elem_sizes[i];

		      size =
			(dest->message.ends[i][j] -
			 dest->message.begins[i][j] + 1) *
			(dest->message.elem_sizes[i]);

		      if (done + size > src->header.msg_size)
			{
			  size = src->header.msg_size - done;
			}

		      __sctk_optimized_memcpy (&(tmp[offset]), cursor, size);
		      cursor += size;
		      done += size;
		    }
		}
	    }
	}
      sctk_nodebug ("Free %p", src);
      sctk_net_free_func ((sctk_thread_ptp_message_t *) src);
    }
  else
    {
      sctk_net_copy_message_func (dest, src);
    }
}

static inline void
sctk_remove_canceled (sctk_task_ptp_data_t * restrict data)
{
  register sctk_thread_ptp_message_t *restrict cursor;
  sctk_thread_ptp_message_t *restrict tmp;
  sctk_thread_ptp_message_t *restrict new_list_head = NULL;
  sctk_thread_ptp_message_t *restrict new_list_tail = NULL;

  cursor = (sctk_thread_ptp_message_t *) data->sctk_send_list.head;

  while (cursor != NULL)
    {
      tmp = (sctk_thread_ptp_message_t *)(cursor->next);
      if (cursor->completion_flag != 3)
	{
	  if (new_list_head == NULL)
	    {
	      new_list_head = cursor;
	    }
	  cursor->next = NULL;
	  new_list_tail = cursor;
	}
      else
	{
	  cursor->completion_flag = 2;
	}
      cursor = tmp;
    }
  data->sctk_send_list.head = new_list_head;
  data->sctk_send_list.tail = new_list_tail;
}

static inline sctk_thread_ptp_message_t *
sctk_find_matching (const
		    sctk_thread_ptp_message_t
		    * restrict dest, sctk_task_ptp_data_t * restrict data)
{
  register sctk_thread_ptp_message_t *restrict cursor;
  sctk_thread_ptp_message_t *restrict tmp;
  sctk_thread_ptp_message_t *restrict last;
  register int dest_tag;

restart:

  if (dest->completion_flag == 2)
    {
      return NULL;
    }

  dest_tag = dest->header.message_tag;

  sctk_thread_ptp_message_list_assert_locked (&(data->sctk_send_list));

  cursor = (sctk_thread_ptp_message_t *) data->sctk_send_list.head;

  if (cursor != NULL)
    {
      tmp = (sctk_thread_ptp_message_t *)cursor->next;
      if (cursor->completion_flag == 3)
	{
	  sctk_remove_canceled (data);
	  goto restart;
	}
      assume (cursor->completion_flag == 0);
      if ((dest_tag == cursor->header.message_tag) || (dest_tag == -1))
	{
	  cursor->next = NULL;
	  if (tmp == NULL)
	    {
	      data->sctk_send_list.tail = NULL;
	    }
	  data->sctk_send_list.head = tmp;
	  return cursor;
	}
      last = cursor;
      cursor = tmp;
    }
  else
    {
      return NULL;
    }

  while (cursor != NULL)
    {
      tmp = (sctk_thread_ptp_message_t *)cursor->next;
      if (cursor->completion_flag == 3)
	{
	  sctk_remove_canceled (data);
	  goto restart;
	}
      assume (cursor->completion_flag == 0);
      if ((dest_tag == cursor->header.message_tag) || (dest_tag == -1))
	{
	  cursor->next = NULL;
	  last->next = tmp;
	  if (tmp == NULL)
	    {
	      data->sctk_send_list.tail = last;
	    }
	  return cursor;
	}
      last = cursor;
      cursor = tmp;
    }
  return NULL;
}

static inline void
__sctk_memcpy (char *restrict s1, const char *restrict s2, size_t n)
{
  if (s1 != s2)
    {
      __sctk_optimized_memcpy (s1, s2, n);
    }
}


/* TODO */
static inline void
__sctk_perform_match_for_source_found (sctk_thread_ptp_message_t *
				       restrict cursor,
				       sctk_thread_ptp_message_t *
				       restrict src)
{

  assume (cursor->completion_flag == 0);
  assume (src->completion_flag == 0);

  sctk_nodebug ("Match %p(%p) -> %p(%p)", src, src->request, cursor,
		cursor->request);

  sctk_nodebug ("%p, %lu -> %lu", cursor, cursor->header.msg_size,
		src->header.msg_size);
  cursor->header.msg_size = src->header.msg_size;
  cursor->header.message_tag = src->header.message_tag;
  cursor->header.local_source = src->header.local_source;
  if (cursor->request != NULL)
    {
      cursor->request->header.msg_size = src->header.msg_size;
      cursor->request->header.message_tag = src->header.message_tag;
      cursor->request->header.local_source = src->header.local_source;
      sctk_nodebug ("%p, %lu -> %lu", cursor->request,
		    cursor->request->header.msg_size, src->header.msg_size);
    }

  sctk_nodebug ("Copy message");

  /*Message match */
  if (src->net_mesg == NULL)
    {
      sctk_nodebug ("There are %d parts", cursor->message.nb_items);
      if ((cursor->message.nb_items == 1)
	  && (src->message.begins[0] == NULL)
	  && (cursor->message.begins[0] == NULL)
	  && (src->message.begins_absolute[0] == NULL)
	  && (cursor->message.begins_absolute[0] == NULL))
	{
	  __sctk_optimized_memcpy (cursor->message.adresses[0],
				   src->message.adresses[0], src->message.sizes[0]);
	}
      else
	{
	  size_t i;
	  for (i = 0; i < cursor->message.nb_items; i++)
	    {
	      sctk_copy_buffer (src->message.begins[i],
				src->message.ends[i],
				src->message.
				begins_absolute[i],
				src->message.ends_absolute[i],
				src->message.sizes[i],
				src->message.adresses[i],
				src->message.elem_sizes[i],
				cursor->message.begins[i],
				cursor->message.ends[i],
				cursor->message.
				begins_absolute[i],
				cursor->message.
				ends_absolute[i],
				cursor->message.sizes[i],
				cursor->message.adresses[i],
				cursor->message.elem_sizes[i]);
	    }
	}
      sctk_nodebug ("Wake send part %p", &(src->completion_flag));
      src->completion_flag = 1;
    }
  else
    {
      sctk_copy_message_net (cursor, src);
    }

  /*Just flip the completion flag, memory liberation
     will be performed by the caller */
  sctk_nodebug ("Wake %p", &(cursor->completion_flag));
  cursor->completion_flag = 1;
}

static inline void
__sctk_perform_match_for_source_found_register(sctk_thread_ptp_message_t *
					       restrict cursor,
					       sctk_thread_ptp_message_t *
					       restrict src,sctk_task_ptp_data_t * restrict data,
					       sctk_thread_ptp_message_t *restrict target){
#warning "Add a policy to select witch method is used for copy"
  if(((target == cursor) || (target == src)) && 0){
      __sctk_perform_match_for_source_found (cursor,src);
  } else {
    cursor->pair = src;
    sctk_spinlock_lock (&(data->spinlock_matched));
    cursor->next = data->matched;
    data->matched = cursor;
    sctk_spinlock_unlock (&(data->spinlock_matched));
  }
}

static inline void
__sctk_perform_matched_messages(sctk_task_ptp_data_t * restrict data){
  while((data->matched != NULL) || (data->busy == 1)){
    sctk_thread_ptp_message_t *restrict cursor;
    sctk_spinlock_lock (&(data->spinlock_matched));

    cursor = (sctk_thread_ptp_message_t *)data->matched;
    if(cursor != NULL){
      data->matched = cursor->next;
      sctk_spinlock_unlock (&(data->spinlock_matched));
      __sctk_perform_match_for_source_found (cursor,(sctk_thread_ptp_message_t *)cursor->pair);
    } else {
      sctk_spinlock_unlock (&(data->spinlock_matched));
    }

    while((data->matched == NULL) && (data->busy == 1)){
      YIELD
    }
  }
}

static inline int
__sctk_perform_match_for_source (sctk_task_ptp_data_t * restrict data,
				 sctk_per_communicator_ptp_data_t *
				 communicator,sctk_thread_ptp_message_t *restrict target)
{
  int res = 0;
  sctk_thread_ptp_message_t *restrict cursor;
  sctk_thread_ptp_message_t *restrict cursor_any_wait;
  sctk_thread_ptp_message_t *restrict src;
  sctk_thread_ptp_message_t *restrict new_list = NULL;
  sctk_thread_ptp_message_t *restrict new_list_tail = NULL;
  sctk_thread_ptp_message_t *restrict new_any_source_list = NULL;
  sctk_thread_ptp_message_t *restrict new_any_source_list_tail = NULL;

  if ((((data->sctk_recv_list.head != NULL)
       || (communicator->sctk_any_source_list.head != NULL))
       && (data->sctk_send_list.head != NULL)) && (data->busy == 0))
    {
      sctk_spinlock_lock (&(data->spinlock));
      data->busy = 1;

      sctk_thread_ptp_message_list_lock (&(data->sctk_recv_list));
      sctk_thread_ptp_message_list_lock (&
					 (communicator->
					  sctk_any_source_list));
      sctk_thread_ptp_message_list_lock (&(data->sctk_send_list));

      cursor = (sctk_thread_ptp_message_t *) (data->sctk_recv_list.head);
      cursor_any_wait =
	(sctk_thread_ptp_message_t *) (communicator->sctk_any_source_list.
				       head);

      data->sctk_recv_list.head = NULL;
      data->sctk_recv_list.tail = NULL;

      communicator->sctk_any_source_list.head = NULL;
      communicator->sctk_any_source_list.tail = NULL;

      while ((cursor != NULL) || (cursor_any_wait != NULL))
	{
	  sctk_thread_ptp_message_t *restrict tmp;
	  int is_any_wait = 0;

	  if (cursor_any_wait != NULL)
	    {
	      if (cursor == NULL)
		{
		  is_any_wait = 1;
		}
	      else
		{
		  if (cursor_any_wait->header.rank_recv <
		      cursor->header.rank_recv)
		    {
		      is_any_wait = 1;
		    }
		}
	    }

	  if (is_any_wait)
	    {
	      tmp = (sctk_thread_ptp_message_t *)cursor_any_wait->next;
	      cursor_any_wait->next = NULL;
	      if (cursor_any_wait->completion_flag == 0)
		{
		  src = sctk_find_matching (cursor_any_wait, data);
		  if (src != NULL)
		    {
		      __sctk_perform_match_for_source_found_register (cursor_any_wait,
								      src,data,target);
		    }
		  else
		    {
		      res++;
		      /*Message not ready */
		      if (new_any_source_list == NULL)
			{
			  new_any_source_list = cursor_any_wait;
			}
		      else
			{
			  new_any_source_list_tail->next = cursor_any_wait;
			}
		      new_any_source_list_tail = cursor_any_wait;
		    }
		}
	      cursor_any_wait = tmp;
	    }
	  else
	    {
	      tmp = (sctk_thread_ptp_message_t *)cursor->next;
	      cursor->next = NULL;
	      if (cursor->completion_flag == 0)
		{
		  src = sctk_find_matching (cursor, data);
		  if (src != NULL)
		    {
		      __sctk_perform_match_for_source_found_register (cursor, src,data,target);
		    }
		  else
		    {
		      res++;
		      /*Message not ready */
		      if (new_list == NULL)
			{
			  new_list = cursor;
			}
		      else
			{
			  new_list_tail->next = cursor;
			}
		      new_list_tail = cursor;
		    }
		}
	      cursor = tmp;
	    }
	}

      assert(data->sctk_recv_list.head == NULL);
      assert(data->sctk_recv_list.tail == NULL);
      assert(communicator->sctk_any_source_list.head == NULL);
      assert(communicator->sctk_any_source_list.tail == NULL);

      data->sctk_recv_list.head = new_list;
      data->sctk_recv_list.tail = new_list_tail;

      communicator->sctk_any_source_list.head = new_any_source_list;
      communicator->sctk_any_source_list.tail = new_any_source_list_tail;

      sctk_thread_ptp_message_list_unlock (&(data->sctk_recv_list));
      sctk_thread_ptp_message_list_unlock (&
					   (communicator->
					    sctk_any_source_list));
      sctk_thread_ptp_message_list_unlock (&(data->sctk_send_list));
      data->busy = 0;
      sctk_spinlock_unlock (&(data->spinlock));
    }

  return res;
}

static inline int
__sctk_perform_match_any_source_wait (sctk_per_communicator_ptp_data_t *
				      communicator)
{
  sctk_thread_ptp_message_t *cursor;
  int res = 0;

  while(sctk_thread_ptp_message_list_trylock (&(communicator->sctk_wait_list))){
    int i;
    sctk_task_ptp_data_t *data;

    YIELD

    for (i = 0; i < communicator->nb_tasks; i++)
      {
	data = &(communicator->tasks[i]);
	__sctk_perform_matched_messages(data);
      }
  }

  cursor = (sctk_thread_ptp_message_t *) (communicator->sctk_wait_list.head);
  while (cursor != NULL)
    {
      if (cursor->completion_flag == 0)
	{
	  if (cursor->data != NULL)
	    {
	      __sctk_perform_matched_messages(cursor->data);
	      if ((cursor->data->sctk_recv_list.head != NULL) &&
		  (cursor->data->sctk_send_list.head != NULL))
		{
		  __sctk_perform_match_for_source (cursor->data,
						   communicator,cursor);
		}
	      __sctk_perform_matched_messages(cursor->data);
	    }
	  else
	    {
	      int i;
	      sctk_task_ptp_data_t *data;
	      for (i = 0; i < communicator->nb_tasks; i++)
		{
		  data = &(communicator->tasks[i]);
		  __sctk_perform_matched_messages(data);
		  if (data->sctk_send_list.head != NULL)
		    {
		      __sctk_perform_match_for_source (data, communicator,cursor);
		    }
		  __sctk_perform_matched_messages(data);
		}
	    }
	}
      if (cursor->completion_flag == 0)
	{
	  res++;
	}
      cursor = (sctk_thread_ptp_message_t *)cursor->wait_next;
    }
  sctk_thread_ptp_message_list_unlock (&(communicator->sctk_wait_list));
  return res;
}
static inline void
__sctk_free_message_wait (sctk_per_communicator_ptp_data_t *
			  restrict communicator,
			  sctk_ptp_data_t * restrict from)
{
  sctk_thread_ptp_message_t *restrict cursor;
  sctk_thread_ptp_message_t *restrict new_list = NULL;
  sctk_thread_ptp_message_t *restrict new_list_tail = NULL;
  sctk_thread_ptp_message_t *restrict new_free_list = NULL;
  sctk_thread_ptp_message_t *restrict new_free_list_tail = NULL;

  sctk_thread_ptp_message_list_lock (&(communicator->sctk_wait_list));


  cursor = (sctk_thread_ptp_message_t *) (communicator->sctk_wait_list.head);
  communicator->sctk_wait_list.head = NULL;
  communicator->sctk_wait_list.tail = NULL;
  while (cursor != NULL)
    {
      sctk_thread_ptp_message_t *restrict tmp = NULL;
      tmp = (sctk_thread_ptp_message_t *)cursor->wait_next;
      cursor->wait_next = NULL;
      if (cursor->completion_flag == 0)
	{
	  if (new_list == NULL)
	    {
	      new_list = cursor;
	      new_list_tail = cursor;
	    }
	  else
	    {
	      new_list_tail->wait_next = cursor;
	      new_list_tail = cursor;
	    }
	}
      else
	{
	  sctk_nodebug ("free recv %p %d", cursor, cursor->completion_flag);
	  if (cursor->header.request != NULL && cursor->completion_flag != 2)
	    {
	      *(cursor->header.req_msg_size) = cursor->header.msg_size;
	      *(cursor->header.request) = cursor->completion_flag;
	    }
	  if (cursor->completion_flag == 2)
	    {
	      sctk_free (cursor);
	    }
	  else if (cursor->allocator_p != NULL)
	    {
	      if (new_free_list == NULL)
		{
		  new_free_list = cursor;
		  new_free_list_tail = cursor;
		}
	      else
		{
		  new_free_list_tail->wait_next = cursor;
		  new_free_list_tail = cursor;
		}
	    }
	}
      cursor = tmp;
    }
  assert(communicator->sctk_wait_list.head == NULL);
  assert(communicator->sctk_wait_list.tail == NULL);
  communicator->sctk_wait_list.head = new_list;
  communicator->sctk_wait_list.tail = new_list_tail;
  sctk_thread_ptp_message_list_unlock (&(communicator->sctk_wait_list));
/*   if (new_free_list_tail != NULL) */
/*     { */
/*       sctk_spinlock_lock (&(from->free_list.lock)); */
/*       new_free_list_tail->wait_next = from->free_list.free_list; */
/*       from->free_list.free_list = new_free_list; */
/*       sctk_spinlock_unlock (&(from->free_list.lock)); */
/*     } */

  new_list = NULL;
  new_list_tail = NULL;
/*   new_free_list = NULL; */
/*   new_free_list_tail = NULL; */

  sctk_thread_ptp_message_list_lock (&(communicator->sctk_wait_send_list));
  cursor =
    (sctk_thread_ptp_message_t *) (communicator->sctk_wait_send_list.head);

  communicator->sctk_wait_send_list.head = NULL;
  communicator->sctk_wait_send_list.tail = NULL;

  while (cursor != NULL)
    {
      sctk_thread_ptp_message_t *restrict tmp = NULL;
      tmp = (sctk_thread_ptp_message_t *)cursor->wait_next;
      cursor->wait_next = NULL;
      if ((cursor->completion_flag == 0) || (cursor->completion_flag == 3))
	{
	  if (new_list == NULL)
	    {
	      new_list = cursor;
	      new_list_tail = cursor;
	    }
	  else
	    {
	      new_list_tail->wait_next = cursor;
	      new_list_tail = cursor;
	    }
	}
      else
	{
	  sctk_nodebug ("free send %p %d", cursor, cursor->completion_flag);
	  if (cursor->header.request != NULL && cursor->completion_flag != 2)
	    {
	      *(cursor->header.req_msg_size) = cursor->header.msg_size;
	      *(cursor->header.request) = cursor->completion_flag;
	    }
	  if (cursor->completion_flag == 2)
	    {
	      sctk_free (cursor);
	    }
	  else if (cursor-> allocator_p!= NULL)
	    {
	      if (new_free_list == NULL)
		{
		  new_free_list = cursor;
		  new_free_list_tail = cursor;
		}
	      else
		{
		  new_free_list_tail->wait_next = cursor;
		  new_free_list_tail = cursor;
		}
	    }
	}
      cursor = tmp;
    }
  assert(communicator->sctk_wait_send_list.head == NULL);
  assert(communicator->sctk_wait_send_list.tail == NULL);
  communicator->sctk_wait_send_list.head = new_list;
  communicator->sctk_wait_send_list.tail = new_list_tail;
  sctk_thread_ptp_message_list_unlock (&(communicator->sctk_wait_send_list));
  if (new_free_list_tail != NULL)
    {
      sctk_spinlock_lock (&(from->free_list.lock));
      new_free_list_tail->wait_next = from->free_list.free_list;
      from->free_list.free_list = new_free_list;
      sctk_spinlock_unlock (&(from->free_list.lock));
    }
}

static inline void
__sctk_probe_for_source (sctk_task_ptp_data_t * data,
			 sctk_thread_message_header_t * msg, int *state)
{
  *state = 0;
  if (data->sctk_send_list.head != NULL)
    {
      sctk_thread_ptp_message_list_lock (&(data->sctk_send_list));
      if (data->sctk_send_list.head != NULL)
	{
	  *state = 1;
	  *msg = data->sctk_send_list.head->header;
	}
      sctk_thread_ptp_message_list_unlock (&(data->sctk_send_list));
    }
}

static inline void
__sctk_probe_for_source_tag (sctk_task_ptp_data_t * data,
			     sctk_thread_message_header_t * msg, int *state)
{
  int message_tag;
  volatile sctk_thread_ptp_message_t *tmp;
  sctk_spinlock_lock (&(data->spinlock));

  message_tag = msg->message_tag;
  *state = 0;
  sctk_thread_ptp_message_list_lock (&(data->sctk_send_list));
  tmp = data->sctk_send_list.head;
  while (tmp != NULL)
    {
      if (tmp->header.message_tag == message_tag)
	{
	  *state = 1;
	  *msg = tmp->header;
	  break;
	}
      tmp = tmp->next;
    }
  sctk_thread_ptp_message_list_unlock (&(data->sctk_send_list));

  sctk_spinlock_unlock (&(data->spinlock));
}

static inline void
__sctk_probe_any_source (sctk_per_communicator_ptp_data_t * communicator,
			 sctk_thread_message_header_t * msg, int *state)
{
  int i;
  int nb_task;
  int found;
  nb_task = communicator->nb_tasks;
  for (i = 0; i < nb_task; i++)
    {
      __sctk_probe_for_source (&(communicator->tasks[i]), msg, &found);
      if (found == 1)
	{
	  *state = 1;
	  return;
	}
    }
  *state = 0;
}

static inline int
sctk_place_message_send (sctk_ptp_data_t * dest,
			 sctk_thread_ptp_message_t * msg)
{
  sctk_task_ptp_data_t *data;
  int res = 0;

  sctk_nodebug
    ("TRY TO PLACE in comm %d in task %d with tag %d form %d is usable %d",
     msg->header.communicator, msg->header.destination,
     msg->header.message_tag, msg->header.source, dest->is_usable);
  if (dest->is_usable == 0)
    return 1;

  if (msg->header.communicator == (sctk_communicator_t)-1 ||
      msg->header.destination == -1 ||
      msg->header.message_tag == -1 || msg->header.source == -1)
    {
      sctk_nodebug ("PLACE in comm %d in task %d with tag %d form %d",
		    msg->header.communicator, msg->header.destination,
		    msg->header.message_tag, msg->header.source);
    }

  data =
    &(dest->communicators[msg->header.communicator].
      tasks[msg->header.source]);
  msg->data = NULL;

  sctk_spinlock_lock (&(data->spinlock));

  if (data->is_usable)
    {
      msg->data = data;

      sctk_thread_ptp_message_list_lock (&(data->sctk_send_list));
      msg->header.rank = data->rank_send;
      data->rank_send++;

      if (data->sctk_send_list.head != NULL)
	{
	  data->sctk_send_list.tail->next = msg;
	}
      else
	{
	  data->sctk_send_list.head = msg;
	}
      data->sctk_send_list.tail = msg;
      sctk_thread_ptp_message_list_unlock (&(data->sctk_send_list));
    }
  else
    {
      res = 1;
    }

  sctk_spinlock_unlock (&(data->spinlock));

  sctk_nodebug ("Message send in %p %d return %d", data, data->is_usable,
		res);

  return res;
}

void
sctk_check_messages (int destination, int source,
		     sctk_communicator_t communicator)
{
  sctk_ptp_data_t *dest;
  sctk_task_ptp_data_t *data;
  sctk_per_communicator_ptp_data_t *ptp_communicator;

  dest = &(sctk_ptp_list[destination]);

  if (dest->is_usable == 0)
    return;

  data = &(dest->communicators[communicator].tasks[source]);
  ptp_communicator = &(dest->communicators[communicator]);

  sctk_thread_ptp_message_list_lock (&(ptp_communicator->sctk_wait_list));
  __sctk_perform_match_for_source (data, ptp_communicator,NULL);
  sctk_thread_ptp_message_list_unlock (&(ptp_communicator->sctk_wait_list));
}

static inline int
sctk_place_message_recv (sctk_ptp_data_t * dest,
			 sctk_thread_ptp_message_t * msg,
			 sctk_per_communicator_ptp_data_t * communicator)
{
  sctk_task_ptp_data_t *data;
/*   assume(dest->is_usable != 0); */

  if (msg->header.source == -1)
    {
      sctk_thread_ptp_message_list_lock (&
					 (communicator->
					  sctk_any_source_list));
      if (communicator->sctk_any_source_list.head != NULL)
	{
	  communicator->sctk_any_source_list.tail->next = msg;
	}
      else
	{
	  communicator->sctk_any_source_list.head = msg;
	}
      communicator->sctk_any_source_list.tail = msg;
      msg->header.rank_recv = communicator->rank_recv;
      communicator->rank_recv++;
      sctk_thread_ptp_message_list_unlock (&
					   (communicator->
					    sctk_any_source_list));
      msg->data = NULL;
    }
  else
    {
      data =
	&(dest->communicators[msg->header.communicator].
	  tasks[msg->header.source]);

      sctk_thread_ptp_message_list_lock (&(data->sctk_recv_list));
      sctk_thread_ptp_message_list_lock (&
					 (communicator->
					  sctk_any_source_list));
      msg->header.rank_recv = communicator->rank_recv;
      communicator->rank_recv++;
      sctk_thread_ptp_message_list_unlock (&
					   (communicator->
					    sctk_any_source_list));

      msg->header.rank = data->rank_recv;
      data->rank_recv++;

      sctk_nodebug ("Message recv in %p", data);
      msg->data = data;

      if (data->sctk_recv_list.head != NULL)
	{
	  data->sctk_recv_list.tail->next = msg;
	}
      else
	{
	  data->sctk_recv_list.head = msg;
	}
      data->sctk_recv_list.tail = msg;
      sctk_thread_ptp_message_list_unlock (&(data->sctk_recv_list));


    }

  return 0;
}

static inline sctk_thread_ptp_message_t *
__sctk_message_alloc (int old_size, sctk_messages_alloc_thread_data_t * allocator)
{
  static int max_size = 0;
  int size;
  sctk_thread_ptp_message_t *tmp;
  char *ptr;

  size = old_size;

  if (max_size < old_size)
    {
      max_size = old_size;
    }
  if (max_size > old_size)
    {
      old_size = max_size;
    }

  size = sctk_max (old_size, size);

  size = ((size / SCTK_MESSAGE_UNIT_NUMBER) + 1) * SCTK_MESSAGE_UNIT_NUMBER;
  sctk_spinlock_lock(&(allocator->lock));
  tmp = (sctk_thread_ptp_message_t *)
    __sctk_calloc (SCTK_MESSAGE_UNIT_SIZE (size), 1, allocator->alloc);
  sctk_spinlock_unlock(&(allocator->lock));
  ptr = (char *) tmp;
  ptr += sctk_aligned_sizeof (sctk_thread_ptp_message_t);

  tmp->message.begins = (sctk_pack_indexes_t **) ptr;
  ptr += sctk_aligned_size (size * sizeof (sctk_pack_indexes_t *));
  tmp->message.ends = (sctk_pack_indexes_t **) ptr;
  ptr += sctk_aligned_size (size * sizeof (sctk_pack_indexes_t *));

  tmp->message.begins_absolute = (sctk_pack_absolute_indexes_t **) ptr;
  ptr += sctk_aligned_size (size * sizeof (sctk_pack_absolute_indexes_t *));
  tmp->message.ends_absolute = (sctk_pack_absolute_indexes_t **) ptr;
  ptr += sctk_aligned_size (size * sizeof (sctk_pack_absolute_indexes_t *));

  tmp->message.sizes = (size_t *) ptr;
  ptr += sctk_aligned_size (size * sizeof (size_t));
  tmp->message.elem_sizes = (size_t *) ptr;
  ptr += sctk_aligned_size (size * sizeof (size_t));
  tmp->message.adresses = (void **) ptr;

  tmp->message.nb_items_max = size;
  tmp->message.max_count = 0;
  tmp->message.nb_items = 0;
  tmp->message.message_size = 0;

  return tmp;
}


static inline sctk_thread_ptp_message_t *
__sctk_message_realloc (size_t new_size, sctk_thread_ptp_message_t * orig)
{
  sctk_nodebug ("Size %d, cur size %d", new_size, orig->message.nb_items_max);
  if (orig->message.nb_items_max > new_size)
    {
      return orig;
    }
  else
    {
      sctk_thread_ptp_message_t *tmp;
      int size;
      size = orig->message.nb_items_max;

      tmp = __sctk_message_alloc (new_size, orig->allocator_p);

      __sctk_optimized_memcpy (tmp->message.begins, orig->message.begins,
	      sctk_aligned_size (size * sizeof (sctk_pack_indexes_t *)));
      __sctk_optimized_memcpy (tmp->message.ends, orig->message.ends,
	      sctk_aligned_size (size * sizeof (sctk_pack_indexes_t *)));

      __sctk_optimized_memcpy (tmp->message.begins_absolute, orig->message.begins_absolute,
	      sctk_aligned_size (size *
				 sizeof (sctk_pack_absolute_indexes_t *)));
      __sctk_optimized_memcpy (tmp->message.ends_absolute, orig->message.ends_absolute,
	      sctk_aligned_size (size *
				 sizeof (sctk_pack_absolute_indexes_t *)));

      __sctk_optimized_memcpy (tmp->message.sizes, orig->message.sizes,
	      sctk_aligned_size (size * sizeof (size_t)));
      __sctk_optimized_memcpy (tmp->message.elem_sizes, orig->message.elem_sizes,
	      sctk_aligned_size (size * sizeof (size_t)));
      __sctk_optimized_memcpy (tmp->message.adresses, orig->message.adresses,
	      sctk_aligned_size (size * sizeof (void *)));

      tmp->header = orig->header;
      tmp->allocator_p = orig->allocator_p;

      tmp->message.nb_items = orig->message.nb_items;
      tmp->message.message_size = orig->message.message_size;
      tmp->message.max_count = orig->message.max_count;

      tmp->header.myself = orig->header.myself;
      sctk_free (orig);
      return tmp;
    }
}



sctk_thread_ptp_message_t *
sctk_create_header (const int myself)
{
  sctk_thread_ptp_message_t *restrict tmp;
  sctk_messages_alloc_thread_data_t*restrict allocator_p;
  int rank_comm_world;

  rank_comm_world = sctk_get_rank (SCTK_COMM_WORLD, sctk_get_task_rank ());

  sctk_nodebug ("Myself create %d glob %d ", myself, rank_comm_world);
  if (sctk_ptp_list[rank_comm_world].free_list.free_list != NULL){
    sctk_spinlock_lock (&(sctk_ptp_list[rank_comm_world].free_list.lock));
    if (sctk_ptp_list[rank_comm_world].free_list.free_list != NULL)
      {
	tmp = (sctk_thread_ptp_message_t *)(sctk_ptp_list[rank_comm_world].free_list.free_list);
	sctk_ptp_list[rank_comm_world].free_list.free_list = tmp->wait_next;
	sctk_spinlock_unlock (&(sctk_ptp_list[rank_comm_world].free_list.lock));
	sctk_init_thread_ptp_message (tmp, myself);
	return tmp;
      }
    sctk_spinlock_unlock (&(sctk_ptp_list[rank_comm_world].free_list.lock));
  }

  allocator_p = &(sctk_ptp_list[rank_comm_world].allocator);
  sctk_nodebug ("Myself create %d tls %p glob %d", myself, tls,
		rank_comm_world);

  tmp = __sctk_message_alloc (1, allocator_p);
  tmp->allocator_p = allocator_p;
  sctk_init_thread_ptp_message (tmp, myself);
  return tmp;
}

sctk_thread_ptp_message_t
  * sctk_add_adress_in_message (sctk_thread_ptp_message_t * restrict msg,
				void *restrict adr, const size_t size)
{
  register int nb_items;
  nb_items = msg->message.nb_items;
  msg = __sctk_message_realloc (nb_items + 1, msg);

  msg->message.begins[nb_items] = NULL;
  msg->message.ends[nb_items] = NULL;

  msg->message.begins_absolute[nb_items] = NULL;
  msg->message.ends_absolute[nb_items] = NULL;

  msg->message.sizes[nb_items] = size;
  msg->message.elem_sizes[nb_items] = 1;
  msg->message.adresses[nb_items] = adr;

  msg->message.nb_items = nb_items + 1;
  if (msg->message.max_count < 1)
    {
      msg->message.max_count = 1;
    }
  return msg;
}

sctk_thread_ptp_message_t
  * sctk_add_pack_in_message (sctk_thread_ptp_message_t * msg, void *adr,
			      const sctk_count_t nb_items,
			      const size_t elem_size,
			      sctk_pack_indexes_t * begins,
			      sctk_pack_indexes_t * ends)
{
  register int nb_items_in_msg;
  nb_items_in_msg = msg->message.nb_items;
  msg = __sctk_message_realloc (nb_items_in_msg + 1, msg);

  msg->message.begins[nb_items_in_msg] = begins;
  msg->message.ends[nb_items_in_msg] = ends;

  msg->message.begins_absolute[nb_items_in_msg] = NULL;
  msg->message.ends_absolute[nb_items_in_msg] = NULL;

  msg->message.sizes[nb_items_in_msg] = nb_items;
  msg->message.elem_sizes[nb_items_in_msg] = elem_size;
  msg->message.adresses[nb_items_in_msg] = adr;

  msg->message.nb_items = nb_items_in_msg + 1;
  if (msg->message.max_count < (size_t)nb_items)
    {
      msg->message.max_count = nb_items;
    }
  sctk_nodebug ("msg->message.max_count = %d", msg->message.max_count);
  return msg;
}

sctk_thread_ptp_message_t
  * sctk_add_pack_in_message_absolute (sctk_thread_ptp_message_t * msg,
				       void *adr,
				       const sctk_count_t nb_items,
				       const size_t elem_size,
				       sctk_pack_absolute_indexes_t *
				       begins,
				       sctk_pack_absolute_indexes_t * ends)
{
  register int nb_items_in_msg;
  nb_items_in_msg = msg->message.nb_items;
  msg = __sctk_message_realloc (nb_items_in_msg + 1, msg);

  msg->message.begins[nb_items_in_msg] = NULL;
  msg->message.ends[nb_items_in_msg] = NULL;

  msg->message.begins_absolute[nb_items_in_msg] = begins;
  msg->message.ends_absolute[nb_items_in_msg] = ends;

  msg->message.sizes[nb_items_in_msg] = nb_items;
  msg->message.elem_sizes[nb_items_in_msg] = elem_size;
  msg->message.adresses[nb_items_in_msg] = adr;

  msg->message.nb_items = nb_items_in_msg + 1;
  if (msg->message.max_count < (size_t)nb_items)
    {
      msg->message.max_count = nb_items;
    }
  sctk_nodebug ("msg->message.max_count = %d", msg->message.max_count);
  return msg;
}

typedef struct
{
  int val;
  int task;
} sctk_placement_status_t;

static void
sctk_placement_test (sctk_placement_status_t * arg)
{
  if (sctk_ptp_process_localisation[arg->task] != -1)
    arg->val = 1;

}

int
sctk_is_net_message (int dest)
{
  sctk_ptp_data_t *tmp;
  int pos;
  tmp = &(sctk_ptp_list[dest]);
  if (tmp->is_usable == 0)
    return 0;

  pos = sctk_ptp_process_localisation[dest];

  if ((pos != sctk_process_rank) && (pos != -1))
    {
      return 1;
    }
  else
    {
      return 0;
    }
}

void
sctk_send_message (sctk_thread_ptp_message_t * msg)
{
  sctk_ptp_data_t *tmp;
  sctk_per_communicator_ptp_data_t *communicator;
  int is_net = 0;
  sctk_placement_status_t state;

  msg->wait_next = NULL;
  msg->next = NULL;

  sctk_nodebug ("send message %p to %d->%d", msg,
		msg->header.source, msg->header.destination);
  if (msg->net_mesg != NULL)
    {
      is_net = 1;
    }

  tmp = &(sctk_ptp_list[msg->header.destination]);
  while (sctk_place_message_send (tmp, msg) == 1)
    {
      int pos;
      if (sctk_ptp_process_localisation[msg->header.destination] == -1)
	{
	  state.val = 0;
	  state.task = msg->header.destination;
	  sctk_thread_wait_for_value_and_poll (&(state.val), 1,
					       (void (*)(void *))
					       sctk_placement_test, &state);
	}
      pos = sctk_ptp_process_localisation[msg->header.destination];
      if ((pos != sctk_process_rank) && (pos != -1))
	{
	  sctk_nodebug ("Task %d on %d", msg->header.destination, pos);
	  sctk_net_send_ptp_message (msg, pos);
	  break;
	}
      YIELD
    }

  sctk_nodebug ("Task %d is here", msg->header.destination);

  if (is_net == 0)
    {
      /*Network messages are threated during copy */
      sctk_nodebug ("insert %p", msg);
      tmp = &(sctk_ptp_list[msg->header.myself]);
      communicator = &(tmp->communicators[msg->header.communicator]);

      sctk_thread_ptp_message_list_lock (&
					 (communicator->sctk_wait_send_list));
      if (communicator->sctk_wait_send_list.head != NULL)
	{
	  communicator->sctk_wait_send_list.tail->wait_next = msg;
	}
      else
	{
	  communicator->sctk_wait_send_list.head = msg;
	}
      communicator->sctk_wait_send_list.tail = msg;
      sctk_thread_ptp_message_list_unlock (&
					   (communicator->
					    sctk_wait_send_list));
    }
  sctk_nodebug ("send message %p done", msg);
}

void
sctk_recv_message (sctk_thread_ptp_message_t * msg)
{
  sctk_ptp_data_t *tmp;
  sctk_per_communicator_ptp_data_t *communicator;

  msg->wait_next = NULL;
  msg->next = NULL;
  sctk_nodebug ("recv message %p to %d->%d", msg,
		msg->header.source, msg->header.destination);

  tmp = &(sctk_ptp_list[msg->header.myself]);
  communicator = &(tmp->communicators[msg->header.communicator]);

  tmp = &(sctk_ptp_list[msg->header.destination]);

  sctk_place_message_recv (tmp, msg, communicator);

  sctk_thread_ptp_message_list_lock (&(communicator->sctk_wait_list));

  tmp = &(sctk_ptp_list[msg->header.myself]);
  if (communicator->sctk_wait_list.head != NULL)
    {
      communicator->sctk_wait_list.tail->wait_next = msg;
    }
  else
    {
      communicator->sctk_wait_list.head = msg;
    }
  communicator->sctk_wait_list.tail = msg;
  sctk_thread_ptp_message_list_unlock (&(communicator->sctk_wait_list));


  __sctk_perform_match_any_source_wait (communicator);
  __sctk_free_message_wait (communicator, tmp);
}

void
sctk_probe_source_any_tag (int destination, int source,
			   const sctk_communicator_t comm, int *status,
			   sctk_thread_message_header_t * msg)
{
  sctk_ptp_data_t *tmp;
  sctk_task_ptp_data_t *data;
/*   sctk_per_communicator_ptp_data_t * communicator; */

  tmp =
    &(sctk_ptp_list
      [sctk_translate_to_global_rank_remote (destination, comm)]);
  data =
    &(tmp->communicators[comm].
      tasks[sctk_translate_to_global_rank_local (source, comm)]);
/*   communicator = &(tmp->communicators[comm]); */

  __sctk_probe_for_source (data, msg, status);
}

void
sctk_probe_source_tag (int destination, int source,
		       const sctk_communicator_t comm, int *status,
		       sctk_thread_message_header_t * msg)
{
  sctk_ptp_data_t *tmp;
  sctk_task_ptp_data_t *data;
/*   sctk_per_communicator_ptp_data_t * communicator; */

  tmp =
    &(sctk_ptp_list
      [sctk_translate_to_global_rank_remote (destination, comm)]);
  data =
    &(tmp->communicators[comm].
      tasks[sctk_translate_to_global_rank_local (source, comm)]);
/*   communicator = &(tmp->communicators[comm]); */

  __sctk_probe_for_source_tag (data, msg, status);
}

void
sctk_probe_any_source_tag (int destination,
			   const sctk_communicator_t comm, int *status,
			   sctk_thread_message_header_t * msg)
{
  sctk_ptp_data_t *tmp;
  sctk_task_ptp_data_t *data;
  int i;
  sctk_per_communicator_ptp_data_t *communicator;

  tmp =
    &(sctk_ptp_list
      [sctk_translate_to_global_rank_remote (destination, comm)]);
  communicator = &(tmp->communicators[comm]);

  for (i = 0; i < communicator->nb_tasks; i++)
    {
      data =
	&(communicator->tasks[sctk_translate_to_global_rank_local (i, comm)]);
      __sctk_probe_for_source_tag (data, msg, status);
      if (*status == 1)
	{
	  return;
	}
    }
}

void
sctk_probe_any_source_any_tag (int destination,
			       const sctk_communicator_t comm, int *status,
			       sctk_thread_message_header_t * msg)
{
  sctk_ptp_data_t *tmp;
  sctk_per_communicator_ptp_data_t *communicator;

  tmp =
    &(sctk_ptp_list
      [sctk_translate_to_global_rank_remote (destination, comm)]);
  communicator = &(tmp->communicators[comm]);

  __sctk_probe_any_source (communicator, msg, status);
}

void
sctk_register_thread_initial (const int i)
{
  int j, k;
/*   sctk_ptp_data_t* tmp; */
/*   tmp = &(sctk_ptp_list[i]); */
  sctk_ptp_process_localisation[i] = sctk_process_rank;
  sctk_nodebug ("task %d on %d", i, sctk_ptp_process_localisation[i]);
  for (j = 0; j < SCTK_MAX_COMMUNICATOR_NUMBER; j++)
    {
      for (k = 0; k < sctk_ptp_list[i].communicators[j].nb_tasks; k++)
	{
	  sctk_ptp_list[i].communicators[j].tasks[k].is_usable = 1;
	}
    }

  {
    static volatile int done = 0;
    static sctk_spinlock_t lock;
    int THREAD_NUMBER;

    sctk_spinlock_lock(&lock);

    if(done == 0){
      for(j = 0; j < sctk_total_task_number; j++){
	int pos = -1;
	int local_threads;
	int start_thread;
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

	sctk_ptp_process_localisation[j] = pos;
	sctk_nodebug("%d/%d is on %d/%d",j,THREAD_NUMBER,pos,sctk_process_number);
      }
      done = 1;
    }

    sctk_spinlock_unlock(&lock);
  }

}

void
sctk_register_thread (const int i)
{
  int j, k;
  sctk_ptp_process_localisation[i] = sctk_process_rank;
  sctk_nodebug ("task %d on %d", i, sctk_ptp_process_localisation[i]);
  for (j = 0; j < SCTK_MAX_COMMUNICATOR_NUMBER; j++)
    {
      for (k = 0; k < sctk_ptp_list[i].communicators[j].nb_tasks; k++)
	{
	  sctk_ptp_list[i].communicators[j].tasks[k].is_usable = 1;
	}
    }

  sctk_net_send_task_location (i, sctk_process_rank);

}

void
sctk_register_distant_thread (const int i, const int pos)
{
  sctk_nodebug ("set task %d on %d really done", i, pos);
  while (sctk_ptp_process_localisation == NULL)
    {
      YIELD
    }
  sctk_ptp_process_localisation[i] = pos;
}

void
sctk_register_restart_thread (const int i, const int pos)
{
  sctk_ptp_process_localisation[i] = pos;
  sctk_nodebug ("task %d on %d", i, sctk_ptp_process_localisation[i]);
}

void
sctk_unregister_distant_thread (const int i)
{
  sctk_ptp_process_localisation[i] = -1;
}

void
sctk_unregister_thread (const int i)
{
  int j, k;
  sctk_nodebug ("unregister task %d on %d", i,
		sctk_ptp_process_localisation[i]);
  sctk_ptp_process_localisation[i] = -1;
  for (j = 0; j < SCTK_MAX_COMMUNICATOR_NUMBER; j++)
    {
      for (k = 0; k < sctk_ptp_list[i].communicators[j].nb_tasks; k++)
	{
	  sctk_spinlock_lock (&
			      (sctk_ptp_list[i].communicators[j].
			       tasks[k].spinlock));
	  sctk_ptp_list[i].communicators[j].tasks[k].is_usable = 0;
	  if (sctk_ptp_list[i].communicators[j].tasks[k].sctk_send_list.
	      head != NULL)
	    {
	      /*clean dues to migration */
	      sctk_error
		("There's some pending messages from %d to %d on communicator %d",
		 i, j, k);
	      /*            sctk_error("Migration part of ptp communication not implemented"); */
	      not_reachable ();
	    }
	  sctk_spinlock_unlock (&(sctk_ptp_list[i].communicators[j].
				  tasks[k].spinlock));
	}
    }
}

typedef struct
{
  int myself;
  sctk_communicator_t communicator;
  int *res;
} sctk_message_wait_t;

static void
sctk_check_for_communicator_poll (sctk_message_wait_t * restrict arg)
{
  int i;
  sctk_ptp_data_t *restrict tmp;
  sctk_per_communicator_ptp_data_t *restrict communicator;
  int val = 0;
  int res;

  tmp = &(sctk_ptp_list[arg->myself]);
  i = arg->communicator;
  communicator = &(tmp->communicators[i]);

  YIELD

  res = __sctk_perform_match_any_source_wait (communicator);
  __sctk_free_message_wait (communicator, tmp);

  if (arg->res != NULL)
    {
      if (communicator->sctk_wait_send_list.head != NULL)
	val++;
      if (communicator->sctk_wait_list.head != NULL)
	val++;
      sctk_nodebug ("%d message on %d on com %d (recv %d)", val,
		    arg->myself, i, res);

      *(arg->res) = val + res;
    }

}

static void
sctk_check_for_communicator_poll_one (sctk_message_wait_t * restrict arg)
{
  int i;
  sctk_ptp_data_t *restrict tmp;
  sctk_per_communicator_ptp_data_t *restrict communicator;

  YIELD

  tmp = &(sctk_ptp_list[arg->myself]);
  i = arg->communicator;
  communicator = &(tmp->communicators[i]);

  __sctk_perform_match_any_source_wait (communicator);
  __sctk_free_message_wait (communicator, tmp);

}

void
sctk_wait_message (sctk_request_t * msg)
{
  sctk_message_wait_t req;
  req.myself = msg->header.myself;
  req.communicator = msg->header.communicator;
  req.res = NULL;

  sctk_nodebug ("Wait message from %d to %d %p(%p) (%d)", msg->header.source,
		msg->header.destination, msg->msg, msg, msg->completion_flag);

  if (msg->completion_flag == 2)
    {
      sctk_check_for_communicator_poll_one (&req);
    }
  else
    {
      if (msg->completion_flag != 1)
	{
	  sctk_check_for_communicator_poll_one (&req);

	  if (msg->completion_flag != 1)
	    {
	      sctk_thread_wait_for_value_and_poll
		((int *) &(msg->completion_flag), 1,
		 (void (*)(void *)) sctk_check_for_communicator_poll_one,
		 (void *) (&req));
	    }
	}
    }
  sctk_nodebug ("Wait message from %d to %d %p done", msg->header.source,
		msg->header.destination, &(msg->completion_flag));
}

void
sctk_cancel_message (sctk_request_t * msg)
{
  sctk_nodebug ("Cancel message from %d to %d %p (%d)", msg->header.source,
		msg->header.destination, &(msg->completion_flag),
		msg->completion_flag);
  if (msg->completion_flag == 0)
    {
      if (msg->header.myself == msg->header.destination)
	{
	  /* Recv part */
	  msg->completion_flag = 2;
	  if (msg->msg != NULL)
	    {
	      if (msg->msg->completion_flag == 0)
		{
		  msg->msg->completion_flag = 2;
		}
	    }
	}
      else
	{
	  /*Send Part */
	  msg->completion_flag = 2;
	  if (msg->msg != NULL)
	    {
	      if (msg->msg->completion_flag == 0)
		{
		  msg->msg->completion_flag = 3;
		}
	    }
	}
    }
}

void
sctk_perform_messages (const int task, const sctk_communicator_t com)
{
  sctk_message_wait_t req;
  int val = 0;
  req.myself = task;
  req.communicator = com;
  req.res = &val;
  sctk_check_for_communicator_poll (&req);
}

void
sctk_wait_all (const int task, const sctk_communicator_t com)
{
  sctk_message_wait_t req;
  sctk_ptp_data_t *tmp;
  sctk_per_communicator_ptp_data_t *communicator;
  int val = 0;
  req.myself = sctk_translate_to_global_rank_local (task, com);
  req.communicator = com;
  req.res = &val;

  tmp = &(sctk_ptp_list[task]);
  communicator = &(tmp->communicators[com]);
  if (communicator->sctk_wait_send_list.head != NULL)
    val++;
  if (communicator->sctk_wait_list.head != NULL)
    val++;
  sctk_nodebug ("Start %d message on %d %p\n", val, task, &val);

  sctk_check_for_communicator_poll (&req);
  sctk_thread_wait_for_value_and_poll (&val, 0, (void (*)(void *))
				       sctk_check_for_communicator_poll,
				       (void *) (&req));
  sctk_assert (communicator->sctk_wait_send_list.head == NULL);
  sctk_assert (communicator->sctk_wait_list.head == NULL);
  sctk_nodebug ("Start %d message on %d done %p\n", val, task, &val);
}

int sctk_get_ptp_process_localisation(int i)
{
  return sctk_ptp_process_localisation[i];
}

