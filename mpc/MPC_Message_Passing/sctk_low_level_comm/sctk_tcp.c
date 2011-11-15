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


#include <sctk_debug.h>
#include <sctk_net_tools.h>
#include <sctk_tcp_toolkit.h>


/************ INTER_THEAD_COMM HOOOKS ****************/
static void* sctk_tcp_thread(sctk_route_table_t* tmp){
  int fd;
  fd = tmp->data.tcp.fd;

  sctk_nodebug("Rail %d from %d launched",tmp->rail->rail_number,
	     tmp->key.destination);

  while(1){
    sctk_thread_ptp_message_t * msg;
    void* body;
    size_t size;

    sctk_safe_read(fd,(char*)&size,sizeof(size_t));

    size = size - sizeof(sctk_thread_ptp_message_body_t) + 
      sizeof(sctk_thread_ptp_message_t);
    msg = sctk_malloc(size);
    body = (char*)msg + sizeof(sctk_thread_ptp_message_t);

    /* Recv header*/
    sctk_nodebug("Read %d",sizeof(sctk_thread_ptp_message_body_t));
    sctk_safe_read(fd,(char*)msg,sizeof(sctk_thread_ptp_message_body_t));

    msg->body.completion_flag = NULL;
    msg->tail.message_type = sctk_message_network;
    
    /* Recv body*/
    size = size - sizeof(sctk_thread_ptp_message_t);
    sctk_safe_read(fd,(char*)body,size);

    sctk_rebuild_header(msg);
    sctk_reinit_header(msg,sctk_free,sctk_net_message_copy);

    sctk_nodebug("MSG RECV|%s|", (char*)body);    

    sctk_nodebug("Msg recved");
    tmp->rail->send_message_from_network(msg);
  }
  return NULL;
}

static void 
sctk_network_send_message_tcp (sctk_thread_ptp_message_t * msg,sctk_rail_info_t* rail){
  sctk_route_table_t* tmp;
  size_t size;
  int fd;

  sctk_nodebug("send message through rail %d",rail->rail_number);

  tmp = sctk_get_route(msg->sctk_msg_get_glob_destination,rail);

  sctk_spinlock_lock(&(tmp->data.tcp.lock));

  fd = tmp->data.tcp.fd;

  size = msg->body.header.msg_size + sizeof(sctk_thread_ptp_message_body_t);

  sctk_safe_write(fd,(char*)&size,sizeof(size_t));

  sctk_safe_write(fd,(char*)msg,sizeof(sctk_thread_ptp_message_body_t));

  sctk_net_write_in_fd(msg,fd);
  sctk_spinlock_unlock(&(tmp->data.tcp.lock));

  sctk_complete_and_free_message(msg);
}

static void  
sctk_network_notify_recv_message_tcp (sctk_thread_ptp_message_t * msg,sctk_rail_info_t* rail){
}

static void 
sctk_network_notify_matching_message_tcp (sctk_thread_ptp_message_t * msg,sctk_rail_info_t* rail){
}

static void 
sctk_network_notify_perform_message_tcp (int remote,sctk_rail_info_t* rail){
}

static void 
sctk_network_notify_idle_message_tcp (sctk_rail_info_t* rail){
}

static void 
sctk_network_notify_any_source_message_tcp (sctk_rail_info_t* rail){
}

/************ INIT ****************/
void sctk_network_init_tcp(sctk_rail_info_t* rail,int sctk_use_tcp_o_ib){
  rail->send_message = sctk_network_send_message_tcp;
  rail->notify_recv_message = sctk_network_notify_recv_message_tcp;
  rail->notify_matching_message = sctk_network_notify_matching_message_tcp;
  rail->notify_perform_message = sctk_network_notify_perform_message_tcp;
  rail->notify_idle_message = sctk_network_notify_idle_message_tcp;
  rail->notify_any_source_message = sctk_network_notify_any_source_message_tcp;
  rail->network_name = "TCP (ring)";

  sctk_network_init_tcp_all(rail,sctk_use_tcp_o_ib,sctk_tcp_thread,sctk_route_ring);
}
