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


typedef enum
{ sctk_contiguous_message, sctk_splitted_message_core,
  sctk_splitted_message_head, sctk_splitted_message_tail
} sctk_net_ptp_message_type_t;

typedef struct
{
  sctk_thread_ptp_message_t msg;
  size_t size;
  size_t slot_size;
  size_t total_size;
  unsigned long addr;
  volatile int received_done;
  sctk_net_ptp_message_type_t msg_type;
} sctk_net_ptp_header_intern_t;

typedef struct
{
  sctk_net_ptp_header_intern_t head;
  sctk_net_rdma_ack_t header_done;
  sctk_net_rdma_ack_t core_done;
} sctk_net_ptp_header_t;

typedef struct
{
  sctk_net_ptp_header_t recv_header_local[SCTK_NET_MESSAGE_NUMBER];
  volatile int recv_rank;

  sctk_net_ptp_header_t send_header_local[SCTK_NET_MESSAGE_NUMBER];
  volatile int send_rank;

  char recv_buffer_local[SCTK_NET_MESSAGE_BUFFER];
  char send_buffer_local[SCTK_NET_MESSAGE_BUFFER];

  volatile sctk_thread_ptp_message_t *splitted_msg;
  volatile size_t splitted_offset;
  volatile size_t total_size;

  sctk_thread_mutex_t lock;
  char* real_remote;
} sctk_net_ptp_slot_t;

static sctk_net_ptp_slot_t **sctk_net_headers_ptp;

static inline void* sctk_convert_to_real_address_func(void* ptr, sctk_net_ptp_slot_t* start, int i){
  unsigned long offset;
  offset = (unsigned long)ptr - (unsigned long)start;
  return (char*)(sctk_net_headers_ptp[i]->real_remote) + offset;
}

#define sctk_convert_to_real_address(a,b,c) sctk_convert_to_real_address_func((void*)(a),(void*)(b),c)

/*
  WARNING No limit on memory consumption
*/
static void
sctk_net_ptp_poll_func (void *arg)
{
  int i;

  for (i = 0; i < sctk_process_number; i++)
    {
      int j;
      int k;

      for (k = 0; k < SCTK_NET_MESSAGE_NUMBER; k++)
	{
	  if(sctk_net_headers_ptp[i]){
	    j = sctk_net_headers_ptp[i]->recv_rank;
	    if ((sctk_net_headers_ptp[i]->recv_header_local[j].head.received_done == 1) &&
		sctk_net_rdma_test (&
				    (sctk_net_headers_ptp[i]->recv_header_local[j].
				     header_done)) == 1
		&&
		sctk_net_rdma_test (&
				    (sctk_net_headers_ptp[i]->recv_header_local[j].
				     core_done)) == 1)
	      {
		sctk_thread_ptp_message_t *msg;
		char *buffer;

		sctk_net_headers_ptp[i]->recv_header_local[j].head.received_done = 0;
		sctk_net_rdma_wait (&
				    (sctk_net_headers_ptp[i]->recv_header_local[j].
				     header_done));
		sctk_net_rdma_wait (&
				    (sctk_net_headers_ptp[i]->recv_header_local[j].
				     core_done));

		assume (sctk_net_headers_ptp[i]->recv_header_local[j].head.size <=
			SCTK_NET_MESSAGE_SPLIT_THRESHOLD);

		switch (sctk_net_headers_ptp[i]->recv_header_local[j].head.msg_type)
		  {
		  case sctk_contiguous_message:
		    {
		      sctk_nodebug
			("Message contiguous from %d in rank %d size %lu ptr %lu",
			 i, j, sctk_net_headers_ptp[i]->recv_header_local[j].head.size,
			 sctk_net_headers_ptp[i]->recv_header_local[j].head.addr);

		      sctk_nodebug ("RECVED %s",
				    sctk_net_headers_ptp[i]->recv_buffer_local +
				    sctk_net_headers_ptp[i]->recv_header_local[j].head.
				    addr);

		      buffer =
			sctk_malloc (sizeof (sctk_thread_ptp_message_t) +
				     sctk_net_headers_ptp[i]->recv_header_local[j].head.
				     size);

		      msg = (sctk_thread_ptp_message_t *) buffer;
		      memcpy (msg,
			      &(sctk_net_headers_ptp[i]->recv_header_local[j].head.msg),
			      sizeof (sctk_thread_ptp_message_t));
		      buffer += sizeof (sctk_thread_ptp_message_t);
		      msg->net_mesg = buffer;

		      memcpy (buffer, sctk_net_headers_ptp[i]->recv_buffer_local +
			      sctk_net_headers_ptp[i]->recv_header_local[j].head.addr,
			      sctk_net_headers_ptp[i]->recv_header_local[j].head.size);

		      sctk_send_message (msg);
		      sctk_net_rdma_write ((int *)
					   &(sctk_net_headers_ptp[i]->recv_header_local[j].
					     head.received_done),
					   (int *)sctk_convert_to_real_address(
					   &(sctk_net_headers_ptp
					     [sctk_process_rank]->send_header_local[j].
					     head.received_done),sctk_net_headers_ptp
					   [sctk_process_rank],i), sizeof (int),
					   i, NULL, NULL);
		      break;
		    }
		  case sctk_splitted_message_head:
		    {
		      sctk_nodebug
			("Message head part from %d in rank %d size %lu ptr %lu total size %lu",
			 i, j, sctk_net_headers_ptp[i]->recv_header_local[j].head.size,
			 sctk_net_headers_ptp[i]->recv_header_local[j].head.addr,
			 sctk_net_headers_ptp[i]->recv_header_local[j].head.total_size);

		      assume (sctk_net_headers_ptp[i]->splitted_msg == NULL);
		      assume (sctk_net_headers_ptp[i]->splitted_offset == 0);
		      assume (sctk_net_headers_ptp[i]->total_size == 0);

		      sctk_net_headers_ptp[i]->total_size =
			sctk_net_headers_ptp[i]->recv_header_local[j].head.total_size;

		      sctk_nodebug ("RECVED %s",
				    sctk_net_headers_ptp[i]->recv_buffer_local +
				    sctk_net_headers_ptp[i]->recv_header_local[j].head.
				    addr);

		      /*
			We add sctk_net_headers_ptp[i]->recv_header_local[j].head.size in order to avoid bffer overflow in
			the copy phase
		      */
		      buffer =
			sctk_malloc (sizeof (sctk_thread_ptp_message_t) +
				     sctk_net_headers_ptp[i]->recv_header_local[j].head.
				     total_size +
				     SCTK_NET_MESSAGE_SPLIT_THRESHOLD);

		      msg = (sctk_thread_ptp_message_t *) buffer;
		      memcpy (msg,
			      &(sctk_net_headers_ptp[i]->recv_header_local[j].head.msg),
			      sizeof (sctk_thread_ptp_message_t));
		      buffer += sizeof (sctk_thread_ptp_message_t);
		      msg->net_mesg = buffer;
		      sctk_net_headers_ptp[i]->splitted_msg = msg;

		      sctk_nodebug ("Use buffer %p message %p", buffer, msg);
		      memcpy (buffer, sctk_net_headers_ptp[i]->recv_buffer_local +
			      sctk_net_headers_ptp[i]->recv_header_local[j].head.addr,
			      sctk_net_headers_ptp[i]->recv_header_local[j].head.size);
		      sctk_net_headers_ptp[i]->splitted_offset +=
			sctk_net_headers_ptp[i]->recv_header_local[j].head.size;

		      sctk_net_rdma_write ((int *)
					   &(sctk_net_headers_ptp[i]->recv_header_local[j].
					     head.received_done),
					   (int *)sctk_convert_to_real_address(
					   &(sctk_net_headers_ptp
					     [sctk_process_rank]->send_header_local[j].
					     head.received_done),sctk_net_headers_ptp
					   [sctk_process_rank],i), sizeof (int),
					   i, NULL, NULL);
		      sctk_nodebug ("Use buffer %p message %p", buffer, msg);
		      break;
		    }
		  case sctk_splitted_message_core:
		    {
		      sctk_nodebug
			("Message split core from %d in rank %d size %lu ptr %lu offset %lu, max %lu",
			 i, j, sctk_net_headers_ptp[i]->recv_header_local[j].head.size,
			 sctk_net_headers_ptp[i]->recv_header_local[j].head.addr,
			 sctk_net_headers_ptp[i]->splitted_offset,
			 sctk_net_headers_ptp[i]->recv_header_local[j].head.total_size);

		      assume (sctk_net_headers_ptp[i]->splitted_msg != NULL);
		      assume (sctk_net_headers_ptp[i]->splitted_offset != 0);
		      assume (sctk_net_headers_ptp[i]->total_size != 0);
		      assume (sctk_net_headers_ptp[i]->recv_header_local[j].head.
			      total_size == sctk_net_headers_ptp[i]->total_size);

		      msg =
			(sctk_thread_ptp_message_t *) sctk_net_headers_ptp[i]->
			splitted_msg;
		      buffer = msg->net_mesg;

		      sctk_nodebug ("Use buffer %p message %p", buffer, msg);

		      memcpy (buffer + sctk_net_headers_ptp[i]->splitted_offset,
			      sctk_net_headers_ptp[i]->recv_buffer_local +
			      sctk_net_headers_ptp[i]->recv_header_local[j].head.addr,
			      sctk_net_headers_ptp[i]->recv_header_local[j].head.size);
		      sctk_net_headers_ptp[i]->splitted_offset +=
			sctk_net_headers_ptp[i]->recv_header_local[j].head.size;

		      sctk_nodebug
			("Receive following part of a splitted message done");
		      sctk_net_rdma_write ((int *)
					   &(sctk_net_headers_ptp[i]->recv_header_local[j].
					     head.received_done),
					   (int *)sctk_convert_to_real_address(
					   &(sctk_net_headers_ptp
					     [sctk_process_rank]->send_header_local[j].
					     head.received_done),sctk_net_headers_ptp
					   [sctk_process_rank],i), sizeof (int),
					   i, NULL, NULL);

		      sctk_nodebug
			("Receive following part of a splitted message done and sended if needed");
		      break;
		    }
		  case sctk_splitted_message_tail:
		    {
		      int to_send = 0;
		      sctk_nodebug
			("Message split tail from %d in rank %d size %lu ptr %lu offset %lu, max %lu",
			 i, j, sctk_net_headers_ptp[i]->recv_header_local[j].head.size,
			 sctk_net_headers_ptp[i]->recv_header_local[j].head.addr,
			 sctk_net_headers_ptp[i]->splitted_offset,
			 sctk_net_headers_ptp[i]->recv_header_local[j].head.total_size);

		      assume (sctk_net_headers_ptp[i]->splitted_msg != NULL);
		      assume (sctk_net_headers_ptp[i]->splitted_offset != 0);
		      assume (sctk_net_headers_ptp[i]->total_size != 0);
		      assume (sctk_net_headers_ptp[i]->recv_header_local[j].head.
			      total_size == sctk_net_headers_ptp[i]->total_size);

		      msg =
			(sctk_thread_ptp_message_t *) sctk_net_headers_ptp[i]->
			splitted_msg;
		      buffer = msg->net_mesg;

		      sctk_nodebug
			("Use buffer %p message %p (offset %p) size %lu",
			 buffer + sctk_net_headers_ptp[i]->splitted_offset, msg,
			 sctk_net_headers_ptp[i]->splitted_offset,
			 sctk_net_headers_ptp[i]->recv_header_local[j].head.size);

		      memcpy (buffer + sctk_net_headers_ptp[i]->splitted_offset,
			      sctk_net_headers_ptp[i]->recv_buffer_local +
			      sctk_net_headers_ptp[i]->recv_header_local[j].head.addr,
			      sctk_net_headers_ptp[i]->recv_header_local[j].head.size);
		      sctk_net_headers_ptp[i]->splitted_offset +=
			sctk_net_headers_ptp[i]->recv_header_local[j].head.size;

		      sctk_nodebug
			("Receive following part of a splitted message done");

		      assume (sctk_net_headers_ptp[i]->splitted_offset <=
			      sctk_net_headers_ptp[i]->recv_header_local[j].head.
			      total_size + SCTK_NET_MESSAGE_SPLIT_THRESHOLD);

		      if (sctk_net_headers_ptp[i]->splitted_offset >=
			  sctk_net_headers_ptp[i]->recv_header_local[j].head.total_size)
			{
			  sctk_net_headers_ptp[i]->splitted_offset = 0;
			  sctk_net_headers_ptp[i]->splitted_msg = NULL;
			  sctk_net_headers_ptp[i]->total_size = 0;

			  to_send = 1;
			  sctk_nodebug ("Reinit %d", i);
			}


		      sctk_net_rdma_write ((int *)
					   &(sctk_net_headers_ptp[i]->recv_header_local[j].
					     head.received_done),
					   (int *)sctk_convert_to_real_address(
									       &(sctk_net_headers_ptp
										 [sctk_process_rank]->send_header_local[j].
										 head.received_done),sctk_net_headers_ptp
									       [sctk_process_rank],i), sizeof (int),
					   i, NULL, NULL);

		      if (to_send)
			{
			  sctk_nodebug ("Splitted message try to send");
			  sctk_send_message (msg);
			  sctk_nodebug ("Splitted message sended");
			}
		      sctk_nodebug
			("Receive following part of a splitted message done and sended if needed");
		      break;
		    }
		  default:
		    not_reachable ();
		  }
		sctk_net_headers_ptp[i]->recv_rank =
		  (sctk_net_headers_ptp[i]->recv_rank + 1) % SCTK_NET_MESSAGE_NUMBER;
	      }
	    else
	      {
		break;
	      }
	  }
	}
    }
}

static size_t sctk_ptp_buffer_size;
static void
sctk_net_preinit_ptp (int use_global_isoalloc)
{
  sctk_ptp_buffer_size = sctk_process_number * sizeof (sctk_net_ptp_slot_t);
//  if(use_global_isoalloc){
    int i;
    sctk_net_headers_ptp =
      sctk_malloc (sctk_process_number * sizeof (sctk_net_ptp_slot_t*));

    for (i = 0; i < sctk_process_number; i++)
      {
	sctk_net_headers_ptp[i] = sctk_malloc (sizeof (sctk_net_ptp_slot_t));
      }
//  } else{
//    sctk_net_headers_ptp = sctk_malloc (sctk_process_number * sizeof (sctk_net_ptp_slot_t*));
//    sctk_net_headers_ptp[sctk_process_rank] =  sctk_iso_malloc (sizeof (sctk_net_ptp_slot_t));
//  }
}

static inline void sctk_net_init_ptp_per_process(int i, sctk_net_ptp_slot_t* tmp,void* real_remote){
  sctk_thread_mutex_init (&(tmp->lock), NULL);
  tmp->send_header_local[0].head.size = SCTK_NET_MESSAGE_BUFFER;
  tmp->send_header_local[0].head.slot_size =
    SCTK_NET_MESSAGE_BUFFER;
  tmp->send_header_local[0].head.addr = 0;
  tmp->splitted_msg = NULL;
  tmp->real_remote = real_remote;
  sctk_net_headers_ptp[i] = tmp;
}

static void
sctk_net_init_ptp (int use_global_isoalloc)
{
  if(use_global_isoalloc){
    int i;
    for (i = 0; i < sctk_process_number; i++)
      {
	  sctk_thread_mutex_init (&(sctk_net_headers_ptp[i]->lock), NULL);
	sctk_net_headers_ptp[i]->send_header_local[0].head.size = SCTK_NET_MESSAGE_BUFFER;
	sctk_net_headers_ptp[i]->send_header_local[0].head.slot_size =
	  SCTK_NET_MESSAGE_BUFFER;
	sctk_net_headers_ptp[i]->send_header_local[0].head.addr = 0;
	sctk_net_headers_ptp[i]->splitted_msg = NULL;
      }
  } else {
    sctk_net_init_ptp_per_process(sctk_process_rank,sctk_net_headers_ptp[sctk_process_rank],sctk_net_headers_ptp[sctk_process_rank]);
  }
}

static inline sctk_net_ptp_header_t *
sctk_net_get_new_header (sctk_net_ptp_slot_t * slot, size_t size, int *r_rank, int process)
{
  sctk_net_ptp_header_t *tmp;
  int rank;
  SCTK_PROFIL_START(sctk_net_get_new_header);

  sctk_connection_check(process);
  slot = sctk_net_headers_ptp[process];

  assume(slot != NULL);

  sctk_thread_mutex_lock (&(slot->lock));

  rank = slot->send_rank;
  *r_rank = rank;
  slot->send_rank = (slot->send_rank + 1) % SCTK_NET_MESSAGE_NUMBER;
  tmp = &(slot->send_header_local[rank]);

  sctk_thread_wait_for_value ((int *) &(tmp->head.received_done), 0);

  tmp->head.received_done = 1;

  if (size <= tmp->head.slot_size)
    {
      if (slot->send_rank != 0)
	{
	  size_t remain;
	  remain = tmp->head.slot_size - size;
	  tmp->head.slot_size = size;

	  sctk_thread_wait_for_value ((int *)
				      &(slot->send_header_local[slot->send_rank].
					head.received_done), 0);

	  slot->send_header_local[slot->send_rank].head.slot_size += remain;
	  slot->send_header_local[slot->send_rank].head.addr =
	    tmp->head.addr + size;


	  sctk_nodebug ("ret %lu %lu ; next %lu %lu", tmp->head.slot_size,
			tmp->head.addr,
			slot->send_header_local[slot->send_rank].head.slot_size,
			slot->send_header_local[slot->send_rank].head.addr);
	}
    }
  else
    {
      size_t remain;
      int i;

      /*The remain part of the buffer stay a the end of the tabs */

      if (tmp->head.addr + size > SCTK_NET_MESSAGE_BUFFER)
	{
	  size_t r_size = 0;
	  tmp->head.addr = 0;
	  tmp->head.received_done = 0;
	  for (i = 0; i < SCTK_NET_MESSAGE_NUMBER; i++)
	    {
	      sctk_thread_wait_for_value ((int *)
					  &(slot->send_header_local[i].head.
					    received_done), 0);
	      r_size += slot->send_header_local[i].head.slot_size;
	      slot->send_header_local[i].head.slot_size = 0;
	      slot->send_header_local[i].head.addr = 0;
	      if (tmp->head.slot_size >= size)
		{
		  break;
		}
	    }
	  tmp->head.received_done = 1;
	  tmp->head.slot_size = r_size;
	}
      else
	{
	  for (i = rank + 1; i < SCTK_NET_MESSAGE_NUMBER; i++)
	    {
	      sctk_thread_wait_for_value ((int *)
					  &(slot->send_header_local[i].head.
					    received_done), 0);
	      tmp->head.slot_size += slot->send_header_local[i].head.slot_size;
	      slot->send_header_local[i].head.slot_size = 0;
	      if (tmp->head.slot_size >= size)
		{
		  break;
		}
	    }
	}

      if (slot->send_rank != 0)
	{
	  remain = tmp->head.slot_size - size;
	  tmp->head.slot_size = size;

	  sctk_thread_wait_for_value ((int *)
				      &(slot->send_header_local[slot->send_rank].
					head.received_done), 0);


	  slot->send_header_local[slot->send_rank].head.slot_size += remain;
	  slot->send_header_local[slot->send_rank].head.addr =
	    tmp->head.addr + size;


	  sctk_nodebug ("ret %lu %lu ; next %lu %lu", tmp->head.slot_size,
			tmp->head.addr,
			slot->send_header_local[slot->send_rank].head.slot_size,
			slot->send_header_local[slot->send_rank].head.addr);
	}
    }
  tmp->head.size = size;
  assume (tmp->head.slot_size >= size);
  assume (tmp->head.size + tmp->head.addr <= SCTK_NET_MESSAGE_BUFFER);
  assert (slot->send_header_local[slot->send_rank].head.addr <=
	  SCTK_NET_MESSAGE_BUFFER);
  sctk_thread_mutex_unlock (&(slot->lock));
  SCTK_PROFIL_END(sctk_net_get_new_header);
  return tmp;
}

static inline sctk_net_ptp_header_t *
sctk_net_send_ptp_message_driver_split_header (sctk_net_ptp_header_t * header,
					       size_t total_size,
					       size_t sended,
					       int dest_process,
					       int *rank,
					       size_t * done_size,
					       char **buffer)
{
  size_t needed;
  sctk_net_ptp_message_type_t msg_type;
  SCTK_PROFIL_START(sctk_net_send_ptp_message_driver_split_header);

  if (header == NULL)
    {
      msg_type = sctk_splitted_message_core;
      needed = SCTK_NET_MESSAGE_SPLIT_THRESHOLD;
      if (needed > (total_size - sended))
	{
	  needed = total_size - sended;
	}

      sctk_nodebug ("Ask new header of size %lu", needed);
      header =
	sctk_net_get_new_header (sctk_net_headers_ptp[dest_process], needed, rank,dest_process);

      if (header->head.size >= total_size - sended)
	{
	  msg_type = sctk_splitted_message_tail;
	}

      header->head.msg_type = msg_type;
      header->head.total_size = total_size;
      *buffer =
	((char *) (sctk_net_headers_ptp[dest_process]->
		   send_buffer_local)) + header->head.addr;
      *done_size = 0;
    }
  SCTK_PROFIL_END(sctk_net_send_ptp_message_driver_split_header);
  return header;
}

static inline sctk_net_ptp_header_t *
sctk_net_send_ptp_message_driver_send_header (sctk_net_ptp_header_t * header,
					      size_t done_size, int rank,
					      int dest_process,
					      size_t * sended)
{
  SCTK_PROFIL_START(sctk_net_send_ptp_message_driver_send_header);
  /*Send the slot if full slot */
  assume (done_size <= header->head.size);
  if (done_size >= header->head.size)
    {
      sctk_net_rdma_write (&(header->head),
			   sctk_convert_to_real_address(&(sctk_net_headers_ptp
			     [sctk_process_rank]->
							  recv_header_local[rank].head),sctk_net_headers_ptp
							[sctk_process_rank],dest_process),
			   sizeof
			   (sctk_net_ptp_header_intern_t),
			   dest_process, NULL,
			   sctk_convert_to_real_address(&(sctk_net_headers_ptp
			     [sctk_process_rank]->
							  recv_header_local[rank].header_done),sctk_net_headers_ptp
							[sctk_process_rank],dest_process));

      sctk_net_rdma_write (((char
			     *) (sctk_net_headers_ptp
				 [dest_process]->
				 send_buffer_local)) +
			   header->head.addr,
			   sctk_convert_to_real_address(((char
			     *) (sctk_net_headers_ptp
				 [sctk_process_rank]->
				 recv_buffer_local)) +
							header->head.addr,sctk_net_headers_ptp
				 [sctk_process_rank], dest_process),
			   done_size, dest_process,
			   NULL,
			   sctk_convert_to_real_address(&(sctk_net_headers_ptp
			     [sctk_process_rank]->
							  recv_header_local[rank].core_done),sctk_net_headers_ptp
			     [sctk_process_rank],dest_process));
      *sended = *sended + done_size;
      header = NULL;
      sctk_nodebug ("Send full header of size %lu", done_size);
    }
  SCTK_PROFIL_END(sctk_net_send_ptp_message_driver_send_header);
  return header;
}

static void
sctk_net_send_ptp_message_driver (sctk_thread_ptp_message_t * msg,
				  int dest_process)
{
  size_t total_size;
  SCTK_PROFIL_START(sctk_net_send_ptp_message_driver);

  sctk_nodebug ("New message");
  total_size = sctk_net_determine_message_size (msg);
  sctk_nodebug ("Total size %lu", total_size);

  /*
     WARNING Handel large message for better performances
   */
  if (total_size <= SCTK_NET_MESSAGE_SPLIT_THRESHOLD)
    {
      /*Send with 2 copy method */
      sctk_net_ptp_header_t *header;
      int rank;
      SCTK_PROFIL_START(sctk_net_send_ptp_message_driver_inf);

      header =
	sctk_net_get_new_header (sctk_net_headers_ptp[dest_process],
				 total_size, &rank,dest_process);
      header->head.msg_type = sctk_contiguous_message;

      memcpy (&(header->head.msg), msg, sizeof (sctk_thread_ptp_message_t));
      sctk_net_copy_in_buffer (msg,
			       ((char *) (sctk_net_headers_ptp[dest_process]->
					  send_buffer_local)) + header->head.addr);
      msg->completion_flag = 1;

      sctk_net_rdma_write (&(header->head),
			   sctk_convert_to_real_address(&(sctk_net_headers_ptp[sctk_process_rank]->
							  recv_header_local[rank].head),sctk_net_headers_ptp[sctk_process_rank],
			   dest_process),
			   sizeof (sctk_net_ptp_header_intern_t),
			   dest_process, NULL,
			   sctk_convert_to_real_address(&(sctk_net_headers_ptp[sctk_process_rank]->
							  recv_header_local[rank].header_done),sctk_net_headers_ptp[sctk_process_rank],dest_process));

      sctk_net_rdma_write (((char *) (sctk_net_headers_ptp[dest_process]->
				      send_buffer_local)) + header->head.addr,
			   sctk_convert_to_real_address(((char *) (sctk_net_headers_ptp[sctk_process_rank]->
								   recv_buffer_local)) + header->head.addr,sctk_net_headers_ptp[sctk_process_rank],
			   dest_process),
			   total_size, dest_process, NULL,
			   sctk_convert_to_real_address(&(sctk_net_headers_ptp[sctk_process_rank]->
							  recv_header_local[rank].core_done),sctk_net_headers_ptp[sctk_process_rank],dest_process));

      SCTK_PROFIL_END(sctk_net_send_ptp_message_driver_inf);
    }
  else
    {
      sctk_net_ptp_header_t *header;
      int rank;
      size_t i;
      size_t j;
      size_t size;
      size_t done_size;
      char *buffer;
      size_t sended = 0;

      static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
      SCTK_PROFIL_START(sctk_net_send_ptp_message_driver_sup);

      /*
         WARNING May be optimize to increase scalability
       */

      sctk_thread_mutex_lock (&lock);

      /*CASE sctk_splitted_message */

      header = sctk_net_get_new_header (sctk_net_headers_ptp[dest_process],
					SCTK_NET_MESSAGE_SPLIT_THRESHOLD,
					&rank,dest_process);
      header->head.msg_type = sctk_splitted_message_head;
      header->head.total_size = total_size;

      buffer =
	((char *) (sctk_net_headers_ptp[dest_process]->send_buffer_local)) +
	header->head.addr;
      done_size = 0;

      memcpy (&(header->head.msg), msg, sizeof (sctk_thread_ptp_message_t));

      sctk_nodebug ("Write from %p", buffer);
      for (i = 0; i < msg->message.nb_items; i++)
	{
	  if (msg->message.begins_absolute[i] != NULL)
	    {
	      for (j = 0; j < msg->message.sizes[i]; j++)
		{
		  /*Prepare the copy slot */
		  header =
		    sctk_net_send_ptp_message_driver_split_header (header,
								   total_size,
								   sended,
								   dest_process,
								   &rank,
								   &done_size,
								   &buffer);
		  size =
		    (msg->message.ends_absolute[i][j] -
		     msg->message.begins_absolute[i][j] +
		     1) * msg->message.elem_sizes[i];
		  if (done_size + size > header->head.size)
		    {
		      size_t copy_done = 0;
		      size_t tmp_size;
		      do
			{
			  header =
			    sctk_net_send_ptp_message_driver_split_header
			    (header, total_size, sended, dest_process, &rank,
			     &done_size, &buffer);
			  sctk_nodebug
			    ("Size %lu done_size %lu header->head.size %lu absolute",
			     size, copy_done, header->head.size);

			  /*Determine max copy size in this step */
			  tmp_size = header->head.size - done_size;
			  if (copy_done + tmp_size > size)
			    {
			      tmp_size = size - copy_done;
			    }
			  sctk_nodebug ("Size for this step %lu", tmp_size);

			  /*Perform the copy of message part */
			  memcpy (buffer,
				  ((char *) (msg->message.adresses[i])) +
				  msg->message.begins_absolute[i][j] *
				  msg->message.elem_sizes[i] + copy_done,
				  tmp_size);

			  copy_done += tmp_size;
			  done_size += tmp_size;
			  buffer += tmp_size;

			  header =
			    sctk_net_send_ptp_message_driver_send_header
			    (header, done_size, rank, dest_process, &sended);
			}
		      while (size > copy_done);
		      assume (size == copy_done);
		    }
		  else
		    {
		      memcpy (buffer, ((char *) (msg->message.adresses[i])) +
			      msg->message.begins_absolute[i][j] *
			      msg->message.elem_sizes[i], size);
		      buffer += size;
		      done_size += size;
		    }
		}
	    }
	  else
	    {
	      if (msg->message.begins[i] == NULL)
		{
		  size = msg->message.sizes[i];
		  sctk_nodebug
		    ("Size %lu done_size %lu header->head.size %lu", size,
		     done_size, header->head.size);
		  /*Prepare the copy slot */
		  header =
		    sctk_net_send_ptp_message_driver_split_header (header,
								   total_size,
								   sended,
								   dest_process,
								   &rank,
								   &done_size,
								   &buffer);
		  if (done_size + size >= header->head.size)
		    {
		      size_t copy_done = 0;
		      size_t tmp_size;
		      do
			{
			  header =
			    sctk_net_send_ptp_message_driver_split_header
			    (header, total_size, sended, dest_process, &rank,
			     &done_size, &buffer);
			  sctk_nodebug
			    ("Size %lu done_size %lu header->head.size %lu",
			     size, copy_done, header->head.size);

			  /*Determine max copy size in this step */
			  tmp_size = header->head.size - done_size;
			  if (copy_done + tmp_size > size)
			    {
			      tmp_size = size - copy_done;
			    }
			  sctk_nodebug ("Size for this step %lu", tmp_size);

			  /*Perform the copy of message part */
			  memcpy (buffer,
				  ((char *) (msg->message.adresses[i])) +
				  copy_done, tmp_size);
			  copy_done += tmp_size;
			  done_size += tmp_size;
			  buffer += tmp_size;

			  header =
			    sctk_net_send_ptp_message_driver_send_header
			    (header, done_size, rank, dest_process, &sended);
			}
		      while (size > copy_done);
		      assume (size == copy_done);
		    }
		  else
		    {
		      memcpy (buffer, msg->message.adresses[i], size);
		      buffer += size;
		      done_size += size;
		    }
		}
	      else
		{
		  for (j = 0; j < msg->message.sizes[i]; j++)
		    {
		      /*Prepare the copy slot */
		      header =
			sctk_net_send_ptp_message_driver_split_header (header,
								       total_size,
								       sended,
								       dest_process,
								       &rank,
								       &done_size,
								       &buffer);
		      size =
			(msg->message.ends[i][j] - msg->message.begins[i][j] +
			 1) * msg->message.elem_sizes[i];
		      if (done_size + size > header->head.size)
			{
			  size_t copy_done = 0;
			  size_t tmp_size;
			  do
			    {
			      header =
				sctk_net_send_ptp_message_driver_split_header
				(header, total_size, sended, dest_process,
				 &rank, &done_size, &buffer);
			      sctk_nodebug
				("Size %lu done_size %lu header->head.size %lu",
				 size, copy_done, header->head.size);

			      /*Determine max copy size in this step */
			      tmp_size = header->head.size - done_size;
			      if (copy_done + tmp_size > size)
				{
				  tmp_size = size - copy_done;
				}
			      sctk_nodebug ("Size for this step %lu",
					    tmp_size);

			      /*Perform the copy of message part */
			      memcpy (buffer,
				      (char *) (msg->message.adresses[i]) +
				      msg->message.begins[i][j] *
				      msg->message.elem_sizes[i] +
				      copy_done, tmp_size);

			      copy_done += tmp_size;
			      done_size += tmp_size;
			      buffer += tmp_size;

			      header =
				sctk_net_send_ptp_message_driver_send_header
				(header, done_size, rank, dest_process,
				 &sended);
			    }
			  while (size > copy_done);
			  assume (size == copy_done);
			}
		      else
			{
			  memcpy (buffer,
				  (char *) (msg->message.adresses[i]) +
				  msg->message.begins[i][j] *
				  msg->message.elem_sizes[i], size);
			  buffer += size;
			  done_size += size;
			}
		    }
		}
	    }
	}
      msg->completion_flag = 1;
      {
      SCTK_PROFIL_START(sctk_net_send_ptp_message_driver_sup_write);
      if (header != NULL)
	{
	  assume (done_size <= header->head.size);
	  sctk_net_rdma_write (&(header->head),
			       sctk_convert_to_real_address(&(sctk_net_headers_ptp[sctk_process_rank]->
							      recv_header_local[rank].head),sctk_net_headers_ptp[sctk_process_rank],
			   dest_process),
			       sizeof (sctk_net_ptp_header_intern_t),
			       dest_process, NULL,
			       sctk_convert_to_real_address(&(sctk_net_headers_ptp[sctk_process_rank]->
							      recv_header_local[rank].header_done),sctk_net_headers_ptp[sctk_process_rank],dest_process));

	  sctk_net_rdma_write (((char *) (sctk_net_headers_ptp[dest_process]->
					  send_buffer_local)) + header->head.addr,
			       sctk_convert_to_real_address(((char *) (sctk_net_headers_ptp[sctk_process_rank]->
								       recv_buffer_local)) + header->head.addr,sctk_net_headers_ptp[sctk_process_rank],
			   dest_process),
			       done_size, dest_process, NULL,
			       sctk_convert_to_real_address(&(sctk_net_headers_ptp[sctk_process_rank]->
							      recv_header_local[rank].core_done),sctk_net_headers_ptp[sctk_process_rank],dest_process));
	  sended += header->head.size;
	  header = NULL;
	  sctk_nodebug ("Send full header of size %lu end", done_size);
	}
      SCTK_PROFIL_END(sctk_net_send_ptp_message_driver_sup_write);
      }
      sctk_thread_mutex_unlock (&lock);
      SCTK_PROFIL_END(sctk_net_send_ptp_message_driver_sup);
    }
  sctk_nodebug ("New message done");
  SCTK_PROFIL_END(sctk_net_send_ptp_message_driver);
}

static void
sctk_net_copy_message_func_driver (sctk_thread_ptp_message_t * restrict dest,
				   sctk_thread_ptp_message_t * restrict src)
{
  not_available ();
}

static void
sctk_net_free_func_driver (sctk_thread_ptp_message_t * item)
{
  sctk_free (item);
}
