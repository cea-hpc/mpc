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

#include "sctk_math.h"

#define SCTK_MAX_PARALLEL_COLLECTIVES 4

typedef struct
{
  volatile int reinit;
  volatile int root_buffer;
  volatile int coll_nb;
  double buffer[collective_buffer_size];
} sctk_collective_msg_intern_msg_t;

typedef struct
{
  sctk_collective_msg_intern_msg_t msg;
  sctk_net_rdma_ack_t done;
} sctk_collective_msg_intern_t;

typedef sctk_collective_msg_intern_t
  sctk_collective_msg_t[collective_op_steps];

typedef struct
{
  sctk_collective_msg_t coll_recv[SCTK_MAX_PARALLEL_COLLECTIVES];
  sctk_collective_msg_intern_t coll_send[SCTK_MAX_PARALLEL_COLLECTIVES];
  volatile int coll_nb;
} sctk_collective_t;

static sctk_collective_t *sctk_collective;
static size_t sctk_collective_buffer_size;
static void
sctk_net_preinit_collectives (int use_global_isoalloc)
{
  assume ((int) ceil (log2 (sctk_process_number)) < collective_op_steps);
  sctk_collective_buffer_size = SCTK_MAX_COMMUNICATOR_NUMBER *
    sizeof (sctk_collective_t);
  if(use_global_isoalloc){
    sctk_collective =
      sctk_iso_malloc (sctk_collective_buffer_size);
  } else{
    sctk_collective =
      sctk_iso_malloc (sctk_collective_buffer_size);
  }
}

static inline void
sctk_net_barrier_op_driver (sctk_collective_communications_t * com,
			    sctk_virtual_processor_t * my_vp)
{
  int step_number;
  int step_current;
  int acc;
  sctk_collective_msg_intern_t *coll_recv;
  sctk_collective_msg_intern_t *coll_send;
  int com_id;
  int coll_nb;

  int nexts[collective_op_steps];
  int process_number;

  step_number = (int) ceil (log2 (com->involved_nb));
  step_current = 0;
  com_id = com->id;
  coll_nb = sctk_collective[com_id].coll_nb;

  sctk_nodebug ("Com id : %d - involved : %d, begin collective barrier %d %d",
    com->involved_nb,
    com_id,
		coll_nb, coll_nb % SCTK_MAX_PARALLEL_COLLECTIVES);

  coll_recv =
    (sctk_collective[com_id].
     coll_recv[coll_nb % SCTK_MAX_PARALLEL_COLLECTIVES]);
  coll_send =
    &(sctk_collective[com_id].
      coll_send[coll_nb % SCTK_MAX_PARALLEL_COLLECTIVES]);
  sctk_nodebug ("recv %p send %p", coll_recv, coll_send);
  sctk_collective[com_id].coll_nb++;

  coll_send->msg.reinit = com->reinitialize;
  coll_send->msg.coll_nb = coll_nb;

  acc = 1;

  if (com->involved_nb == sctk_process_number)
    {
      for (step_current = 0; step_current < step_number; step_current++)
	{
	  int next;
	  next = (sctk_process_rank + acc) % sctk_process_number;
	  nexts[step_current] = next;
	  acc *= 2;
	}
    }
  else
    {
      int tmp_next;
      int j;
      int rank;
      int next;

      process_number = com->involved_nb;

      rank = 0;
      for (j = 0; j < sctk_process_rank; j++)
	{
	  if (com->involved_list[j])
	    {
	      rank++;
	    }
	}

      for (step_current = 0; step_current < step_number; step_current++)
	{
	  tmp_next = (rank + acc) % process_number;
	  next = 0;
	  while (!com->involved_list[next])
	    next++;
	  for (j = 0; j < tmp_next; j++)
	    {
	      next++;
	      while (!com->involved_list[next])
		next++;
	    }
	  nexts[step_current] = next;
	  acc *= 2;
	}
    }

  acc = 1;

  for (step_current = 0; step_current < step_number; step_current++)
    {
      int next;
      next = nexts[step_current];

      sctk_nodebug ("INIT for RDMA in collective %d -> %d",
		    sctk_process_rank, next);
      assume (sctk_net_rdma_write
	      (coll_send, &(coll_recv[step_current]),
	       sizeof (sctk_collective_msg_intern_msg_t), next,
	       &(coll_send->done), &(coll_recv[step_current].done)) == 0);
      sctk_nodebug ("Wait for RDMA in collective step %d barrier %d %p",
		    step_current, sctk_collective[com_id].coll_nb - 1,
		    &(coll_send->done));
      sctk_net_rdma_wait (&(coll_send->done));
      sctk_nodebug
	("Wait for received RDMA in collective step %d barrier %d %p",
	 step_current, sctk_collective[com_id].coll_nb - 1,
	 &(coll_recv[step_current].done));


      sctk_net_rdma_wait (&(coll_recv[step_current].done));

      sctk_nodebug("1 : %d - 2 : %d", coll_recv[step_current].msg.coll_nb, coll_nb);

      assume (coll_recv[step_current].msg.coll_nb == coll_nb);
      {
	int local_val;
	int new_val;
	local_val = coll_send->msg.reinit;
	new_val = coll_recv[step_current].msg.reinit;
	coll_send->msg.reinit = local_val | new_val;
      }
      acc *= 2;
      sctk_nodebug ("End step %d barrier %d", step_current,
		    sctk_collective[com_id].coll_nb - 1);
    }


  com->reinitialize = coll_send->msg.reinit;
  /*   memset (coll_recv, 0, sizeof (sctk_collective_msg_t)); */
  sctk_nodebug ("end collective barrier %d",
		sctk_collective[com_id].coll_nb - 1);
}

static inline void
sctk_net_broadcast_op_driver (sctk_collective_communications_t * com,
			      sctk_virtual_processor_t * my_vp,
			      const size_t elem_size,
			      const size_t nb_elem,
			      const sctk_datatype_t data_type,
			      void *data_in, void *data_out)
{
  if (elem_size * nb_elem <= collective_buffer_size*sizeof(double))
    {
      int step_number;
      int step_current;
      int acc;
      sctk_collective_msg_intern_t *coll_recv;
      sctk_collective_msg_intern_t *coll_send;
      int com_id;
      int old_root_buffer = 0;
      int coll_nb;

      int nexts[collective_op_steps];
      int process_number;

      step_number = (int) ceil (log2 (com->involved_nb));
      step_current = 0;
      com_id = com->id;
      coll_nb = sctk_collective[com_id].coll_nb;
      sctk_nodebug ("begin collective broadcast %d", coll_nb);
      coll_recv =
	(sctk_collective[com_id].
	 coll_recv[coll_nb % SCTK_MAX_PARALLEL_COLLECTIVES]);
      coll_send =
	&(sctk_collective[com_id].
	  coll_send[coll_nb % SCTK_MAX_PARALLEL_COLLECTIVES]);
      sctk_collective[com_id].coll_nb++;

      coll_send->msg.reinit = com->reinitialize;
      coll_send->msg.root_buffer = 0;
      coll_send->msg.coll_nb = coll_nb;

      acc = 1;
      if (com->involved_nb == sctk_process_number)
	{
	  for (step_current = 0; step_current < step_number; step_current++)
	    {
	      int next;
	      next = (sctk_process_rank + acc) % sctk_process_number;
	      nexts[step_current] = next;
	      acc *= 2;
	    }
	}
      else
	{
	  int tmp_next;
	  int j;
	  int rank;
	  int next;

	  process_number = com->involved_nb;

	  rank = 0;
	  for (j = 0; j < sctk_process_rank; j++)
	    {
	      if (com->involved_list[j])
		{
		  rank++;
		}
	    }

	  for (step_current = 0; step_current < step_number; step_current++)
	    {
	      tmp_next = (rank + acc) % process_number;
	      next = 0;
	      while (!com->involved_list[next])
		next++;
	      for (j = 0; j < tmp_next; j++)
		{
		  next++;
		  while (!com->involved_list[next])
		    next++;
		}
	      nexts[step_current] = next;
	      acc *= 2;
	    }
	}

      acc = 1;

      if (data_in)
	{
	  coll_send->msg.root_buffer = 1;
	  old_root_buffer = 1;
	  memcpy (coll_send->msg.buffer, data_in, elem_size * nb_elem);
	}

      for (step_current = 0; step_current < step_number; step_current++)
	{
	  int next;
	  next = nexts[step_current];

	  sctk_nodebug ("INIT for RDMA in collective %d -> %d",
			sctk_process_rank, next);
	  assume (sctk_net_rdma_write
		  (coll_send, &(coll_recv[step_current]),
		   sizeof (sctk_collective_msg_intern_msg_t), next,
		   &(coll_send->done), &(coll_recv[step_current].done)) == 0);
	  sctk_nodebug ("Wait for RDMA in collective");
	  sctk_net_rdma_wait (&(coll_send->done));
	  sctk_nodebug ("Wait for received RDMA in collective");
	  sctk_net_rdma_wait (&(coll_recv[step_current].done));

      sctk_nodebug("(2) 1 : %d - 2 : %d", coll_recv[step_current].msg.coll_nb, coll_nb);
	  assume (coll_recv[step_current].msg.coll_nb == coll_nb);
	  {
	    int local_val;
	    int new_val;

	    local_val = coll_send->msg.reinit;
	    new_val = coll_recv[step_current].msg.reinit;
	    coll_send->msg.reinit = local_val | new_val;

	    local_val = coll_send->msg.root_buffer;
	    new_val = coll_recv[step_current].msg.root_buffer;
	    coll_send->msg.root_buffer = local_val | new_val;
	  }

	  if (coll_send->msg.root_buffer && old_root_buffer == 0)
	    {
	      sctk_nodebug ("Receive message");
	      memcpy (coll_send->msg.buffer,
		      coll_recv[step_current].msg.buffer,
		      elem_size * nb_elem);
	      old_root_buffer = 1;
	    }

	  acc *= 2;
	  sctk_nodebug ("End step %d", step_current);
	}

      if (data_out)
	{
	  memcpy (data_out, coll_send->msg.buffer, elem_size * nb_elem);
	}

      com->reinitialize = coll_send->msg.reinit;
      /*   memset (coll_recv, 0, sizeof (sctk_collective_msg_t)); */
    }
  else
    {
      size_t total_mem;
      size_t done_mem = 0;
      size_t cur_size;

      cur_size = collective_buffer_size*sizeof(double);
      total_mem = elem_size * nb_elem;

      if (data_out == NULL)
	{
	  while (done_mem < total_mem)
	    {
	      sctk_net_broadcast_op_driver (com, my_vp, cur_size, 1,
					    data_type,
					    ((char *) data_in) + done_mem,
					    NULL);
	      done_mem += cur_size;
	      if (done_mem + cur_size > total_mem)
		{
		  cur_size = total_mem - done_mem;
		}
	    }
	}
      else
	{
	  while (done_mem < total_mem)
	    {
	      sctk_net_broadcast_op_driver (com, my_vp, cur_size, 1,
					    data_type, NULL,
					    ((char *) data_out) + done_mem);
	      done_mem += cur_size;
	      if (done_mem + cur_size > total_mem)
		{
		  cur_size = total_mem - done_mem;
		}
	    }
	}
    }
  sctk_nodebug ("end collective broadcast");
}

static inline void
sctk_net_reduce_op_driver_intern (sctk_collective_communications_t * com,
				  sctk_virtual_processor_t * my_vp,
				  const size_t elem_size,
				  const size_t nb_elem,
				  void (*func) (const void *, void *, size_t,
						sctk_datatype_t),
				  const sctk_datatype_t data_type,
				  size_t offset)
{
  if (elem_size * nb_elem <= collective_buffer_size*sizeof(double))
    {
      int step_number;
      int step_current;
      int acc;
      sctk_collective_msg_intern_t *coll_recv;
      sctk_collective_msg_intern_t *coll_send;
      int com_id;
      int coll_nb;

      int nexts[collective_op_steps];
      int rank;
      int process_number;

      step_number = (int) ceil (log2 (com->involved_nb));
      step_current = 0;
      com_id = com->id;
      coll_nb = sctk_collective[com_id].coll_nb;
      sctk_nodebug ("begin collective reduce %d", coll_nb);
      coll_recv =
	(sctk_collective[com_id].
	 coll_recv[coll_nb % SCTK_MAX_PARALLEL_COLLECTIVES]);
      coll_send =
	&(sctk_collective[com_id].
	  coll_send[coll_nb % SCTK_MAX_PARALLEL_COLLECTIVES]);
      sctk_collective[com_id].coll_nb++;

      coll_send->msg.reinit = com->reinitialize;
      coll_send->msg.coll_nb = coll_nb;

      acc = 1;
      process_number = com->involved_nb;
      if (com->involved_nb == sctk_process_number)
	{
	  int next;

	  rank = sctk_process_rank;
	  for (step_current = 0; step_current < step_number; step_current++)
	    {
	      if (rank % (acc * 2) == 0)
		{
		  next = rank + acc;
		  nexts[step_current] = next;
		}
	      else
		{
		  next = rank - acc;
		  nexts[step_current] = next;
		  break;
		}
	      acc *= 2;
	    }
	}
      else
	{
	  int j;
	  int next;
	  int tmp_next;
	  rank = 0;
	  for (j = 0; j < sctk_process_rank; j++)
	    {
	      if (com->involved_list[j])
		{
		  rank++;
		}
	    }
	  for (step_current = 0; step_current < step_number; step_current++)
	    {
	      if (rank % (acc * 2) == 0)
		{
		  tmp_next = rank + acc;
		  if (tmp_next < process_number)
		    {
		      next = 0;
		      while (!com->involved_list[next])
			next++;
		      for (j = 0; j < tmp_next; j++)
			{
			  next++;
			  while (!com->involved_list[next])
			    next++;
			}
		    }
		  else
		    {
		      next = sctk_process_number;
		    }
		  nexts[step_current] = next;
		}
	      else
		{
		  tmp_next = rank - acc;
		  next = 0;
		  while (!com->involved_list[next])
		    next++;
		  for (j = 0; j < tmp_next; j++)
		    {
		      next++;
		      while (!com->involved_list[next])
			next++;
		    }
		  nexts[step_current] = next;
		  break;
		}
	      acc *= 2;
	    }
	}
      acc = 1;

      {
	int next;

	memcpy (coll_send->msg.buffer,
		(char *) (my_vp->data.data_out) + offset,
		elem_size * nb_elem);

	/*descente */
	for (step_current = 0; step_current < step_number; step_current++)
	  {
	    if (rank % (acc * 2) == 0)
	      {
		next = nexts[step_current];
		if (next < sctk_process_number)
		  {
		    /*Recv from next */
		    sctk_net_rdma_wait (&(coll_recv[step_current].done));
		    assume (coll_recv[step_current].msg.coll_nb == coll_nb);

		    func (coll_recv[step_current].msg.buffer,
			  coll_send->msg.buffer, nb_elem, data_type);
		    {
		      int local_val;
		      int new_val;
		      local_val = coll_send->msg.reinit;
		      new_val = coll_recv[step_current].msg.reinit;
		      coll_send->msg.reinit = local_val | new_val;
		    }
		  }
	      }
	    else
	      {
		next = nexts[step_current];
		/*send to next */

		assume (sctk_net_rdma_write
			(coll_send, &(coll_recv[step_current]),
			 sizeof (sctk_collective_msg_intern_msg_t), next,
			 &(coll_send->done),
			 &(coll_recv[step_current].done)) == 0);
		sctk_net_rdma_wait (&(coll_send->done));
		break;
	      }


	    acc *= 2;
	    sctk_nodebug ("End step %d", step_current);
	  }

	com->reinitialize = coll_send->msg.reinit;
	if (rank == 0)
	  {
	    memcpy ((char *) (my_vp->data.data_out) + offset,
		    coll_send->msg.buffer, elem_size * nb_elem);
	    sctk_net_broadcast_op_driver (com, my_vp, elem_size, nb_elem,
					  data_type,
					  (char *) (my_vp->data.data_out) +
					  offset, NULL);
	  }
	else
	  {
	    sctk_net_broadcast_op_driver (com, my_vp, elem_size, nb_elem,
					  data_type, NULL,
					  (char *) (my_vp->data.data_out) +
					  offset);
	  }
      }
    }
  else
    {
      if (elem_size < collective_buffer_size*sizeof(double))
	{
	  size_t check = 0;
	  size_t i;
	  size_t slot;
	  size_t remain;
	  slot = (collective_buffer_size*sizeof(double))/elem_size;
	  remain = (collective_buffer_size*sizeof(double))%elem_size;
	  for (i = 0; i < nb_elem; i+=slot)
	    {
	      sctk_net_reduce_op_driver_intern (com, my_vp, elem_size, slot,
						func, data_type,
						check);
	      check+=slot;
	    }
	  sctk_net_reduce_op_driver_intern (com, my_vp, elem_size, remain,
					    func, data_type,
					    check*elem_size);
	  check+=remain;
	  assume(check == nb_elem);
	}
      else
	{
	  not_available ();
	}
    }

  sctk_nodebug ("end collective reduce");
}

static inline void
sctk_net_reduce_op_driver (sctk_collective_communications_t * com,
			   sctk_virtual_processor_t * my_vp,
			   const size_t elem_size,
			   const size_t nb_elem,
			   void (*func) (const void *, void *, size_t,
					 sctk_datatype_t),
			   const sctk_datatype_t data_type)
{
  sctk_net_reduce_op_driver_intern (com, my_vp, elem_size, nb_elem, func,
				    data_type, 0);
}

static void
sctk_net_collective_op_driver (sctk_collective_communications_t * com,
			       sctk_virtual_processor_t * my_vp,
			       const size_t elem_size,
			       const size_t nb_elem,
			       void (*func) (const void *, void *, size_t,
					     sctk_datatype_t),
			       const sctk_datatype_t data_type)
{
  sctk_nodebug ("begin collective");
  if (nb_elem == 0)
    {
      sctk_nodebug ("begin collective barrier");
      sctk_net_barrier_op_driver (com, my_vp);
      sctk_nodebug ("end collective barrier");
    }
  else
    {
      if (func == NULL)
	{
	  sctk_nodebug ("begin collective broadcast");
	  sctk_net_broadcast_op_driver (com, my_vp, elem_size, nb_elem,
					data_type, my_vp->data.data_in,
					my_vp->data.data_out);
	  sctk_nodebug ("end collective broadcast");
	}
      else
	{
	  sctk_nodebug ("begin collective reduce");
	  sctk_net_reduce_op_driver (com, my_vp, elem_size, nb_elem, func,
				     data_type);
	  sctk_nodebug ("end collective reduce");
	}
    }
  sctk_nodebug ("end collective");
}
