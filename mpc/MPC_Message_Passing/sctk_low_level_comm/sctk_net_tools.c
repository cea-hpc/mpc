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

#include "sctk_inter_thread_comm.h"
#include "sctk_low_level_comm.h"
#include "sctk_debug.h"
#include "sctk_net_tools.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

  void sctk_net_read_in_fd (sctk_thread_ptp_message_t * msg,
			     int fd)
  {
TODO("Deal with partial reception")
    switch(msg->tail.message_type){
    case sctk_message_contiguous: {
      size_t size;

      size = msg->body.header.msg_size;

      sctk_nodebug("MSG SEND |%s|", (char*)msg->tail.message.contiguous.addr);
      sctk_safe_read(fd,msg->tail.message.contiguous.addr,size);
      break;
    }
    case sctk_message_network: {
      size_t size;
      void* body;

      size = msg->body.header.msg_size;
      body = (char*)msg + sizeof(sctk_thread_ptp_message_t);

      sctk_safe_read(fd,body,size);
      break;
    }
    case sctk_message_pack: {
      size_t i;
      size_t j;
      size_t size;
      for (i = 0; i < msg->tail.message.pack.count; i++)
	for (j = 0; j < msg->tail.message.pack.list.std[i].count; j++)
	  {
	    size = (msg->tail.message.pack.list.std[i].ends[j] -
		    msg->tail.message.pack.list.std[i].begins[j] +
		    1) * msg->tail.message.pack.list.std[i].elem_size;
	    sctk_safe_read(fd,((char *) (msg->tail.message.pack.list.std[i].addr)) +
			    msg->tail.message.pack.list.std[i].begins[j] *
			    msg->tail.message.pack.list.std[i].elem_size,size);
	  }
      break;
    }
  case sctk_message_pack_absolute: {
    size_t i;
    size_t j;
    size_t size;
    for (i = 0; i < msg->tail.message.pack.count; i++)
      for (j = 0; j < msg->tail.message.pack.list.absolute[i].count; j++)
	{
	  size = (msg->tail.message.pack.list.absolute[i].ends[j] -
		  msg->tail.message.pack.list.absolute[i].begins[j] +
		  1) * msg->tail.message.pack.list.absolute[i].elem_size;
	  sctk_safe_read(fd,((char *) (msg->tail.message.pack.list.absolute[i].addr)) +
			  msg->tail.message.pack.list.absolute[i].begins[j] *
			  msg->tail.message.pack.list.absolute[i].elem_size,size);
	}
      break;
    }
    default: not_reachable();
    }
  }

  void sctk_net_write_in_fd (sctk_thread_ptp_message_t * msg,
			     int fd)
  {
    switch(msg->tail.message_type){
    case sctk_message_contiguous: {
      size_t size;

      size = msg->body.header.msg_size;

      sctk_nodebug("MSG SEND |%s|", (char*)msg->tail.message.contiguous.addr);
      sctk_safe_write(fd,msg->tail.message.contiguous.addr,size);
      break;
    }
    case sctk_message_network: {
      size_t size;
      void* body;

      size = msg->body.header.msg_size;
      body = (char*)msg + sizeof(sctk_thread_ptp_message_t);

      sctk_safe_write(fd,body,size);
      break;
    }
    case sctk_message_pack: {
      size_t i;
      size_t j;
      size_t size;
      for (i = 0; i < msg->tail.message.pack.count; i++)
	for (j = 0; j < msg->tail.message.pack.list.std[i].count; j++)
	  {
	    size = (msg->tail.message.pack.list.std[i].ends[j] -
		    msg->tail.message.pack.list.std[i].begins[j] +
		    1) * msg->tail.message.pack.list.std[i].elem_size;
	    sctk_safe_write(fd,((char *) (msg->tail.message.pack.list.std[i].addr)) +
			    msg->tail.message.pack.list.std[i].begins[j] *
			    msg->tail.message.pack.list.std[i].elem_size,size);
	  }
      break;
    }
  case sctk_message_pack_absolute: {
    size_t i;
    size_t j;
    size_t size;
    for (i = 0; i < msg->tail.message.pack.count; i++)
      for (j = 0; j < msg->tail.message.pack.list.absolute[i].count; j++)
	{
	  size = (msg->tail.message.pack.list.absolute[i].ends[j] -
		  msg->tail.message.pack.list.absolute[i].begins[j] +
		  1) * msg->tail.message.pack.list.absolute[i].elem_size;
	  sctk_safe_write(fd,((char *) (msg->tail.message.pack.list.absolute[i].addr)) +
			  msg->tail.message.pack.list.absolute[i].begins[j] *
			  msg->tail.message.pack.list.absolute[i].elem_size,size);
	}
      break;
    }
    default: not_reachable();
    }
  }

  void sctk_net_copy_in_buffer (sctk_thread_ptp_message_t * msg,
				char *buffer)
  {
    switch(msg->tail.message_type){
    case sctk_message_contiguous: {
      size_t size;

      size = msg->body.header.msg_size;
    sctk_nodebug("SEND size %lu %lu", size,
        adler32(0, (unsigned char*) msg->tail.message.contiguous.addr, size));

      sctk_nodebug("MSG SEND |%s|", (char*)msg->tail.message.contiguous.addr);
      memcpy(buffer,msg->tail.message.contiguous.addr,size);
      buffer += size;
      break;
    }
    case sctk_message_network: {
      size_t size;
      void* body;

      size = msg->body.header.msg_size;
      body = (char*)msg + sizeof(sctk_thread_ptp_message_t);

      memcpy(buffer,body,size);
      buffer += size;
      break;
    }
    case sctk_message_pack: {
      size_t i;
      size_t j;
      size_t size;
      for (i = 0; i < msg->tail.message.pack.count; i++)
	for (j = 0; j < msg->tail.message.pack.list.std[i].count; j++)
	  {
	    size = (msg->tail.message.pack.list.std[i].ends[j] -
		    msg->tail.message.pack.list.std[i].begins[j] +
		    1) * msg->tail.message.pack.list.std[i].elem_size;
	    memcpy(buffer,((char *) (msg->tail.message.pack.list.std[i].addr)) +
			    msg->tail.message.pack.list.std[i].begins[j] *
			    msg->tail.message.pack.list.std[i].elem_size,size);
	    buffer += size;
	  }
      break;
    }
  case sctk_message_pack_absolute: {
    size_t i;
    size_t j;
    size_t size;
    for (i = 0; i < msg->tail.message.pack.count; i++)
      for (j = 0; j < msg->tail.message.pack.list.absolute[i].count; j++)
	{
	  size = (msg->tail.message.pack.list.absolute[i].ends[j] -
		  msg->tail.message.pack.list.absolute[i].begins[j] +
		  1) * msg->tail.message.pack.list.absolute[i].elem_size;
	  memcpy(buffer,((char *) (msg->tail.message.pack.list.absolute[i].addr)) +
			  msg->tail.message.pack.list.absolute[i].begins[j] *
			  msg->tail.message.pack.list.absolute[i].elem_size,size);
	  buffer += size;
	}
      break;
    }
    default: not_reachable();
    }
  }


  void* sctk_net_if_one_msg_in_buffer (sctk_thread_ptp_message_t * msg)
  {
    not_implemented();
/*     size_t i; */
/*     size_t j; */
/*     int nb_msg = 0; */
/*     void* ptr = NULL; */

/*     for (i = 0; i < msg->message.nb_items; i++) */
/*     { */
/*       if (msg->message.begins_absolute[i] != NULL) */
/*       { */
/*         for (j = 0; j < msg->message.sizes[i]; j++) */
/*         { */
/*           ptr =((char *) (msg->message.adresses[i])) + */
/*               msg->message.begins_absolute[i][j] * */
/*               msg->message.elem_sizes[i]; */

/*           nb_msg++; */
/*           if (nb_msg >= 2) */
/*             return 0; */
/*         } */
/*       } */
/*       else */
/*       { */
/*         if (msg->message.begins[i] == NULL) */
/*         { */
/*           ptr = msg->message.adresses[i]; */
/*           nb_msg++; */
/*           if (nb_msg >= 2) */
/*             return 0; */
/*         } */
/*         else */
/*         { */
/*           for (j = 0; j < msg->message.sizes[i]; j++) */
/*           { */
/*             ptr = (char *) (msg->message.adresses[i]) + */
/*                 msg->message.begins[i][j] * */
/*                 msg->message.elem_sizes[i]; */

/*             nb_msg++; */
/*             if (nb_msg >= 2) */
/*               return 0; */
/*           } */
/*         } */
/*       } */
/*     } */
/*     return ptr; */
    return NULL;
  }



  size_t sctk_net_determine_message_size (sctk_thread_ptp_message_t *
						 msg)
  {
    return msg->body.header.msg_size;
  }

static  inline void
amd64_cpy_nt ( volatile void *dst, volatile void *src, size_t n ) {
	memcpy ( ( void * ) dst, ( void * ) src, n );
}

static inline size_t
copy_frag ( char *msg,
                    size_t size,
                    char *buffer,
                    size_t curr_copy,
                    size_t max_copy,
                    size_t tot_size,
                    int* go_on ) {
  not_implemented();
  return 0;
}


int sctk_net_copy_frag_msg (
    const sctk_thread_ptp_message_t * msg,
    char *buffer,
    const size_t curr_copy,
    const size_t max_copy ) {
  size_t tmp_size;

  switch(msg->tail.message_type){
    case sctk_message_contiguous:
      {
        size_t size;
        size = msg->body.header.msg_size;

        if ( (size - curr_copy) > max_copy)
          tmp_size = max_copy;
        else tmp_size = (size - curr_copy);

        memcpy((char*) buffer + curr_copy,
            (char*) msg->tail.message.contiguous.addr + curr_copy,
            tmp_size);
        break;
      }
    case sctk_message_network:
      {
        size_t size;
        void* body;
        not_implemented();

        size = msg->body.header.msg_size;
        body = (char*)msg + sizeof(sctk_thread_ptp_message_t);

        memcpy(buffer,body,size);
        buffer += size;
        break;
      }
    case sctk_message_pack:
      {
        size_t i;
        size_t j;
        size_t size;
        not_implemented();
        for (i = 0; i < msg->tail.message.pack.count; i++)
          for (j = 0; j < msg->tail.message.pack.list.std[i].count; j++)
          {
            size = (msg->tail.message.pack.list.std[i].ends[j] -
                msg->tail.message.pack.list.std[i].begins[j] +
                1) * msg->tail.message.pack.list.std[i].elem_size;
            memcpy(buffer,((char *) (msg->tail.message.pack.list.std[i].addr)) +
                msg->tail.message.pack.list.std[i].begins[j] *
                msg->tail.message.pack.list.std[i].elem_size,size);
            buffer += size;
          }
        break;
      }
    case sctk_message_pack_absolute:
      {
        size_t i;
        size_t j;
        size_t size;
        not_implemented();
        for (i = 0; i < msg->tail.message.pack.count; i++)
          for (j = 0; j < msg->tail.message.pack.list.absolute[i].count; j++)
          {
            size = (msg->tail.message.pack.list.absolute[i].ends[j] -
                msg->tail.message.pack.list.absolute[i].begins[j] +
                1) * msg->tail.message.pack.list.absolute[i].elem_size;
            memcpy(buffer,((char *) (msg->tail.message.pack.list.absolute[i].addr)) +
                msg->tail.message.pack.list.absolute[i].begins[j] *
                msg->tail.message.pack.list.absolute[i].elem_size,size);
            buffer += size;
          }
        break;
      }
    default: not_reachable();
  }

/* 	size_t i; */
/* 	size_t j; */
/* 	size_t size; */
/* 	size_t tot_size = 0; */
/* 	int go_on; */

/* 	sctk_nodebug ( "Write from %p", buffer ); */
/* 	sctk_nodebug ( "Curr_copy (%u), max_copy (%u)", curr_copy, max_copy ); */

/* 	for ( i = 0; i < msg->message.nb_items; i++ ) { */
/* 		if ( msg->message.begins_absolute[i] != NULL ) { */
/* 			for ( j = 0; j < msg->message.sizes[i]; j++ ) { */
/* 				size = ( msg->message.ends_absolute[i][j] - */
/* 				         msg->message.begins_absolute[i][j] + */
/* 				         1 ) * msg->message.elem_sizes[i]; */

/* 				sctk_nodebug ( "size 0: %d", size ); */

/* 				tot_size = copy_frag ( */
/* 				            ( ( char * ) ( msg->message.adresses[i] ) ) + */
/* 				            msg->message.begins_absolute[i][j] * */
/* 				            msg->message.elem_sizes[i], */
/* 				            size, */
/* 				            buffer, */
/* 				            curr_copy, */
/* 				            max_copy, */
/* 				            tot_size, */
/* 				            &go_on ); */

/* 				sctk_nodebug ( "Tot size : %ld", tot_size ); */

/* 				if ( go_on == 0 ) { */
/* 					sctk_nodebug ( "Tot size : %ld", tot_size ); */
/* 					return tot_size; */
/* 				} */
/* 			} */
/* 		} else { */
/* 			if ( msg->message.begins[i] == NULL ) { */
/* 				sctk_nodebug ( "size 1: %d", msg->message.sizes[i] ); */

/* 				tot_size = copy_frag ( */
/* 				            msg->message.adresses[i], */
/* 				            msg->message.sizes[i], */
/* 				            buffer, */
/* 				            curr_copy, */
/* 				            max_copy, */
/* 				            tot_size, */
/* 				            &go_on ); */

/* 				sctk_nodebug ( "Tot size : %ld", tot_size ); */

/* 				sctk_nodebug ( "Curr copy : %d", curr_copy ); */

/* 				if ( go_on == 0 ) { */
/* 					sctk_nodebug ( "Tot size : %ld", tot_size ); */
/* 					return tot_size; */
/* 				} */
/* 			} else { */
/* 				for ( j = 0; j < msg->message.sizes[i]; j++ ) { */
/* 					size = */
/* 					 ( msg->message.ends[i][j] - msg->message.begins[i][j] + */
/* 					   1 ) * msg->message.elem_sizes[i]; */

/* 					sctk_nodebug ( "size 2: %d", size ); */

/* 					tot_size = copy_frag ( */
/* 					            ( char * ) ( msg->message.adresses[i] ) + */
/* 					            msg->message.begins[i][j] * */
/* 					            msg->message.elem_sizes[i], */
/* 					            size, */
/* 					            buffer, */
/* 					            curr_copy, */
/* 					            max_copy, */
/* 					            tot_size, */
/* 					            &go_on ); */

/* 					sctk_nodebug ( "Tot size : %ld", tot_size ); */

/* 					if ( go_on == 0 ) { */
/* 						sctk_nodebug ( "Tot size : %ld", tot_size ); */
/* 						return tot_size; */
/* 					} */
/* 				} */
/* 			} */
/* 		} */
/* 	} */

/* 	sctk_nodebug ( "End of function (curr_copy %d) (max %d) (tot_size %d)", curr_copy, max_copy, tot_size ); */
/* 	return tot_size; */
  not_implemented();
  return 0;
}

void sctk_net_message_copy(sctk_message_to_copy_t* tmp){
  sctk_thread_ptp_message_t* send;
  sctk_thread_ptp_message_t* recv;
  char* body;

  send = tmp->msg_send;
  recv = tmp->msg_recv;

  body = (char*)send + sizeof(sctk_thread_ptp_message_t);

  send->body.completion_flag = NULL;

  sctk_nodebug("MSG |%s|", (char*)body);

  switch(recv->tail.message_type){
  case sctk_message_contiguous: {
    size_t size;
    size = send->body.header.msg_size;
    if(size > recv->tail.message.contiguous.size){
      size = recv->tail.message.contiguous.size;
    }

    memcpy(recv->tail.message.contiguous.addr,body,
	   size);

    sctk_message_completion_and_free(send,recv);
    break;
  }
  case sctk_message_pack: {
    size_t i;
    size_t j;
    size_t size;
    for (i = 0; i < recv->tail.message.pack.count; i++)
      for (j = 0; j < recv->tail.message.pack.list.std[i].count; j++)
	{
	  size = (recv->tail.message.pack.list.std[i].ends[j] -
		  recv->tail.message.pack.list.std[i].begins[j] +
		  1) * recv->tail.message.pack.list.std[i].elem_size;
	  memcpy(((char *) (recv->tail.message.pack.list.std[i].addr)) +
		 recv->tail.message.pack.list.std[i].begins[j] *
		 recv->tail.message.pack.list.std[i].elem_size,body,size);
	  body += size;
	}
    sctk_message_completion_and_free(send,recv);
    break;
  }
  case sctk_message_pack_absolute: {
    size_t i;
    size_t j;
    size_t size;
    for (i = 0; i < recv->tail.message.pack.count; i++)
      for (j = 0; j < recv->tail.message.pack.list.absolute[i].count; j++)
	{
	  size = (recv->tail.message.pack.list.absolute[i].ends[j] -
		  recv->tail.message.pack.list.absolute[i].begins[j] +
		  1) * recv->tail.message.pack.list.absolute[i].elem_size;
	  memcpy(((char *) (recv->tail.message.pack.list.absolute[i].addr)) +
		 recv->tail.message.pack.list.absolute[i].begins[j] *
		 recv->tail.message.pack.list.absolute[i].elem_size,body,size);
	  body += size;
	}
    sctk_message_completion_and_free(send,recv);
    break;
  }
  default: not_reachable();
  }
}

void sctk_net_message_copy_from_buffer(char* body,
  sctk_message_to_copy_t* tmp, char free_headers) {
  sctk_thread_ptp_message_t* send;
  sctk_thread_ptp_message_t* recv;

  sctk_nodebug("MSG RECV |%s|", (char*)body);
  send = tmp->msg_send;
  recv = tmp->msg_recv;

  switch(recv->tail.message_type){
  case sctk_message_contiguous: {
    size_t size;
    size = send->body.header.msg_size;
    if(size > recv->tail.message.contiguous.size){
      size = recv->tail.message.contiguous.size;
    }

    memcpy(recv->tail.message.contiguous.addr,body,
	   size);
    sctk_nodebug("RECV size %lu-%lu %lu %p", size, recv->tail.message.contiguous.size,
        adler32(0, (unsigned char*) recv->tail.message.contiguous.addr, size), recv);

    if(free_headers) sctk_message_completion_and_free(send,recv);
    break;
  }
  case sctk_message_pack: {
    size_t i;
    size_t j;
    size_t size;
    for (i = 0; i < recv->tail.message.pack.count; i++)
      for (j = 0; j < recv->tail.message.pack.list.std[i].count; j++)
	{
	  size = (recv->tail.message.pack.list.std[i].ends[j] -
		  recv->tail.message.pack.list.std[i].begins[j] +
		  1) * recv->tail.message.pack.list.std[i].elem_size;
	  memcpy(((char *) (recv->tail.message.pack.list.std[i].addr)) +
		 recv->tail.message.pack.list.std[i].begins[j] *
		 recv->tail.message.pack.list.std[i].elem_size,body,size);
	  body += size;
	}
    if(free_headers) sctk_message_completion_and_free(send,recv);
    break;
  }
  case sctk_message_pack_absolute: {
    size_t i;
    size_t j;
    size_t size;
    for (i = 0; i < recv->tail.message.pack.count; i++)
      for (j = 0; j < recv->tail.message.pack.list.absolute[i].count; j++)
	{
	  size = (recv->tail.message.pack.list.absolute[i].ends[j] -
		  recv->tail.message.pack.list.absolute[i].begins[j] +
		  1) * recv->tail.message.pack.list.absolute[i].elem_size;
	  memcpy(((char *) (recv->tail.message.pack.list.absolute[i].addr)) +
		 recv->tail.message.pack.list.absolute[i].begins[j] *
		 recv->tail.message.pack.list.absolute[i].elem_size,body,size);
	  body += size;
	}
    if(free_headers) sctk_message_completion_and_free(send,recv);
    break;
  }
  default: not_reachable();
  }
}

/* void sctk_safe_write(int fd, char* buf,size_t size){ */
/*   size_t done = 0;  */
/*   int res; */
/*   do{ */
/*     res = write(fd,buf + done ,size - done); */
/*     if(res < 0){ */
/*       perror("Write error"); */
/*       sctk_abort(); */
/*     } */
/*     done += res; */
/*   }while(done < size); */
/* } */

/* void sctk_safe_read(int fd, char* buf,size_t size){ */
/*   size_t done = 0;  */
/*   int res; */
/*   do{ */
/*     res = read(fd,buf + done,size - done); */
/*     if(res < 0){ */
/*       perror("Read error"); */
/*       sctk_abort(); */
/*     } */
/*     done += res; */
/*   }while(done < size); */
/* } */
