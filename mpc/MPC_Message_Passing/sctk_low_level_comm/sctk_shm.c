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
/* #   - PERACHE Marc     marc.perache@cea.fr                             # */
/* #   - DIDELOT Sylvain  sdidelot@exascale-computing.eu                    # */
/* #                                                                      # */
/* ######################################################################## */

#include "sctk_low_level_comm.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk_bootstrap.h"
#include "sctk_rpc.h"
#include "sctk_shm.h"
#include "sctk_mpcrun_client.h"

#ifdef MPC_USE_SHM

#include <stdio.h>
#include <sys/mman.h>		/* for mmap */
#include <sys/types.h>
#include <fcntl.h>		/* for O_* contants */
#include "sctk_net_tools.h"
#include "sctk_tcp.h"

/* pointer to the memory structure */
extern struct sctk_shm_mem_struct_s *sctk_shm_mem_struct;
/* mutex between pool and thread PTP receiver. Thread receiver is a rescue polling function when
 * the receving daemon is blocked on a sending function;*/
static sctk_thread_mutex_t ptp_received_lock;
/* structure where all pointers to functions are saved  */
sctk_net_driver_pointers_functions_t* module_pointers;

/* Accessor to translation table */
int* sctk_shm_get_local_to_global_process_translation_table() {
	return sctk_shm_get_memstruct_base()->shm_local_to_global_translation_table;
}
int* sctk_shm_get_global_to_local_process_translation_table() {
	return sctk_shm_get_memstruct_base()->shm_global_to_local_translation_table;
}

/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_fill_translation_table
 *  Description: Grab local rank and local size from TCP server.
 *  Alors fill local tables
 * ==================================================================
 */
static void sctk_shm_fill_translation_tables() {
    sctk_nodebug ( "Init local vars| %p", sctk_shm_mem_struct->shm_local_process_translation_table );

    /* fill translation table */
    sctk_spinlock_lock ( & ( sctk_shm_mem_struct->translation_table_lock ) );

    sctk_nodebug ( "Local : %d", sctk_local_process_rank );

    sctk_shm_mem_struct->shm_local_to_global_translation_table[sctk_local_process_rank] = sctk_process_rank;
    sctk_shm_mem_struct->shm_global_to_local_translation_table[sctk_process_rank] = sctk_local_process_rank;

    sctk_spinlock_unlock ( & ( sctk_shm_mem_struct->translation_table_lock ) );
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_is_shm_com
 *  Description: Check if a collective communication is a pure SHM
 *  communication
 * ==================================================================
 */
static int sctk_shm_is_shm_com ( sctk_collective_communications_t* com ) {
  int i;
  int com_type;

  if ( com->involved_nb != sctk_local_process_number )
    return -1;

  /* lockup inter-node */
  for ( i=0; i < com->involved_nb; ++i ) {
    com_type = sctk_shm_translate_global_to_local ( com->involved_list[i], NULL );

    if ( com_type == SCTK_SHM_INTER_NODE_COMM ) {
      return -1;
    }
  }
  return 0;
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sckt_shm_thread_poll
 *  Description:  Function which checks if a new message is available
 *  in a queue. If it is, a flasg is up. Used for PTP, RMDA and RPC msg.
 * ==================================================================
 */
static void
sctk_shm_thread_poll ( struct sctk_shm_fastmsg_queue_s *__queue ) {
  if ( __queue->count_msg_received > 0 ) {
    sctk_nodebug ( "nb msg : %d", __queue->count_msg_received );
    sctk_nodebug ( "New message received in queue" );
    __queue->flag_new_msg_received = 1;
  }
}


/*-----------------------------------------------------------
 *                          RDMA
 *----------------------------------------------------------*/

/*
 * ===  FUNCTION  ===================================================
 *         Name:  sckt_shm_thread_rdma
 *  Description:  Thread which polls a new rdma msg.
 * ==================================================================
 */
static void *
sctk_shm_thread_rdma ( void *arg ) {
  int result;
  struct sctk_shm_rpc_s rpc;
  struct sctk_shm_rdma_read_s rdma_read;
  struct sctk_shm_rdma_write_s rdma_write;
  size_t size;
  struct sctk_shm_ret_get_ptp_s ret;
  struct sctk_shm_rpc_slot_s *rpc_slot = NULL;

  /* malloc pointer to pointer */
  ret.selected_slot_content = sctk_malloc ( sizeof ( void * ) );

  sctk_nodebug ( "RDMA polling thread launched" );

  while ( 1 ) {
    sctk_thread_wait_for_value_and_poll ( ( int * )
        &
        ( sctk_shm_mem_struct->shm_ptp_queue
          [sctk_local_process_rank]->shm_fastmsg_queue
          [SCTK_SHM_RDMA_FAST]->flag_new_msg_received ),
        1,
        ( void ( * ) ( void * ) )
        sctk_shm_thread_poll,
        sctk_shm_mem_struct->shm_ptp_queue
        [sctk_local_process_rank]->shm_fastmsg_queue
        [SCTK_SHM_RDMA_FAST] );

    sctk_nodebug ( "NEW RDMA message received" );

    result = sctk_shm_get_ptp_msg ( sctk_local_process_rank,
        &rpc,
        sizeof ( struct sctk_shm_rpc_s ),
        &size, SCTK_SHM_RDMA_FAST, &ret );

    if ( result != -1 ) {
      rpc_slot = ret.selected_slot;
      sctk_nodebug ( "Result : %d", ret.msg_type );

      switch ( ret.msg_type ) {

        case SCTK_SHM_SLOT_TYPE_PTP_RDMA_READ:
          sctk_nodebug
            ( "BEGIN sctk_shm_thread_rdma (read) (slot %p) content (%p)",
              rpc_slot, rpc_slot->msg_content );

          /* copy the rdma_read request */
          memcpy ( &rdma_read, rpc_slot->msg_content, sizeof ( struct sctk_shm_rdma_read_s ) );

          if ( rdma_read.rdma_size <= SCTK_SHM_FASTMSG_RDMA_MAXLEN ) {
            rpc_slot->shm_msg_type = SCTK_SHM_RDMA_FAST;

            memcpy ( rpc_slot->msg_content,
                rdma_read.rdma_addr, rdma_read.rdma_size );

            rpc_slot->is_rpc_acked = 1;
          } else {
            not_implemented();

            //TODO "BEWARE : part of code not tested. May contain bugs"
            rpc_slot->shm_msg_type = SCTK_SHM_RDMA_BIG;

            sctk_shm_put_dma_bigmsg ( rdma_read.src_rank,
                rdma_read.rdma_addr,
                rdma_read.rdma_size,
                & ( rpc_slot->is_rpc_acked ) );
          }

          sctk_nodebug ( "END sctk_shm_thread_rdma" );
          break;

        case SCTK_SHM_SLOT_TYPE_PTP_RDMA_WRITE:
          sctk_nodebug ( "BEGIN sctk_shm_thread_rdma (write)" );

          /* copy the rdma_write request*/
          memcpy ( &rdma_write, rpc_slot->msg_content, sizeof ( struct sctk_shm_rdma_write_s ) );

          memcpy ( rdma_write.rdma_addr,
              & ( rdma_write.rdma_msg ), rdma_write.rdma_size );

          rpc_slot->is_rpc_acked = 1;
      }
    } else {
      sctk_thread_yield ();
    }

    sctk_shm_mem_struct->
      shm_ptp_queue[sctk_local_process_rank]->shm_fastmsg_queue
      [SCTK_SHM_RDMA_FAST]->flag_new_msg_received = 0;

  }
  return NULL;
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_rdma read
 *  Description:  Read the content of an address in another
 *                process.
 * ==================================================================
 */
static void
sctk_shm_rdma_read ( unsigned int __process,
    void *__remote_src, void *__local_dest, size_t __size ) {
  struct sctk_shm_rpc_slot_s *rpc_slot = NULL;
  struct sctk_shm_rdma_read_s rdma;

  /* forge RDMA msg */
  rdma.rdma_addr = __remote_src;
  rdma.rdma_size = __size;
  rdma.src_rank = sctk_local_process_rank;

  while ( ( rpc_slot = sctk_shm_put_fastmsg ( __process,
          &rdma,
          sizeof ( struct
            sctk_shm_rdma_read_s ),
          SCTK_SHM_SLOT_TYPE_PTP_RDMA_READ ) )
      == NULL ) {
    sctk_thread_yield ();
  }

  sctk_nodebug ( "BEGIN sctk_shm_rdma_read" );
  /* wait rdma ack */
  sctk_thread_wait_for_value ( ( int * ) & ( rpc_slot->is_rpc_acked ), 1 );

  if ( rpc_slot->shm_msg_type == SCTK_SHM_RDMA_FAST ) {
    memcpy ( __local_dest, rpc_slot->msg_content, __size );
  } else {

    //TODO "BEWARE : part of code not tested. May contain bugs"
    sctk_shm_get_dma_bigmsg ( sctk_local_process_number,
        __local_dest,
        __size );

    not_implemented();
  }

  /* reset the ack flag */
  rpc_slot->is_rpc_acked = 0;
  sctk_nodebug ( "END sctk_shm_rdma_read" );

}



/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_rdma_write
 *  Description:  Write the content of a value into  another process.

#include "sctk_shm_mem_struct_funcs.h"
 *                BEWARE : there is no implementation for messages bigger than
 *                the size of the slot. In the case of this module, the size of
 *                the variable is always sizeof(int)
 * ==================================================================
 */
static void
sctk_shm_rdma_write ( unsigned int __process,
    void *__remote_src, int __local_dest, size_t __size ) {
  struct sctk_shm_rpc_slot_s *rpc_slot;
  struct sctk_shm_rdma_write_s rdma;

  /* forge RDMA msg */
  rdma.rdma_addr = __remote_src;
  rdma.rdma_size = __size;
  rdma.rdma_msg = __local_dest;

  while ( ( rpc_slot = sctk_shm_put_fastmsg ( __process,
          &rdma,
          sizeof ( struct
            sctk_shm_rdma_write_s ),
          SCTK_SHM_SLOT_TYPE_PTP_RDMA_WRITE ) )
      == NULL ) {
    sctk_thread_yield ();
  }

  sctk_nodebug ( "BEGIN sctk_shm_rdma_write" );
  /* wait rdma ack */
  sctk_thread_wait_for_value ( ( int * ) & ( rpc_slot->is_rpc_acked ), 1 );

  /* reset the ack flag */
  rpc_slot->is_rpc_acked = 0;
  sctk_nodebug ( "END sctk_shm_rdma_write" );
}



/*-----------------------------------------------------------
 *                          RPC
 *----------------------------------------------------------*/

/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_net_rpc_driver
 *  Description:  Send a RPC to a process
 * ==================================================================
 */
static void
sctk_net_rpc_driver ( void ( *func ) ( void * ), int destination, void *arg,
    size_t arg_size ) {
  struct sctk_shm_rpc_s msg;
  struct sctk_shm_rpc_slot_s *rpc_slot;

  sctk_nodebug ( "LOCAL RANK : %d", destination );
  static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
  sctk_thread_mutex_lock ( &lock );


  if ( arg_size > SCTK_SHM_RPC_ARGS_MAXLEN ){
    sctk_error("%d > %d",arg_size,SCTK_SHM_RPC_ARGS_MAXLEN);
    not_implemented ();
  }

  msg.shm_rpc_function = func;
  memcpy ( msg.shm_rpc_args, arg, arg_size );
  msg.shm_rpc_size_args = arg_size;

  /* send RPC */
  while ( ( rpc_slot = sctk_shm_put_fastmsg ( destination,
          &msg,
          sizeof ( struct sctk_shm_rpc_s ),
          SCTK_SHM_SLOT_TYPE_PTP_RPC |
          SCTK_SHM_SLOT_TYPE_FASTMSG ) ) ==
      NULL ) {
    sctk_thread_yield ();
  }
  sctk_nodebug ( "BEGIN - net_rpc_driver to %d (slot %p)", destination,
      rpc_slot );

  sctk_thread_wait_for_value ( ( int * ) & ( rpc_slot->is_rpc_acked ), 1 );
  rpc_slot->is_rpc_acked = 0;
  sctk_thread_mutex_unlock ( &lock );
  sctk_nodebug ( "END - net_rpc_driver" );
}




/*
 * ===  FUNCTION  ===================================================
 *         Name:  sckt_shm_thread_rpc
 *  Description:  Function which polls new RPC msg.
 * ==================================================================
 */
static void *
sctk_shm_thread_rpc ( void *arg ) {
  /*RPC deamon */
  int result;
  struct sctk_shm_rpc_slot_s *rpc_slot = NULL;
  struct sctk_shm_rpc_s rpc;
  sctk_update_communicator_t *args;
  size_t size;
  struct sctk_shm_ret_get_ptp_s ret;

  /* malloc pointer to pointer */
  ret.selected_slot_content = sctk_malloc ( sizeof ( void * ) );

  sctk_nodebug ( "RPC polling thread launched" );

  while ( 1 ) {

    sctk_thread_wait_for_value_and_poll ( ( int * )
        &
        ( sctk_shm_mem_struct->shm_ptp_queue
          [sctk_local_process_rank]->shm_fastmsg_queue
          [SCTK_SHM_RPC_FAST]->flag_new_msg_received ),
        1,
        ( void ( * ) ( void * ) )
        sctk_shm_thread_poll,
        sctk_shm_mem_struct->shm_ptp_queue
        [sctk_local_process_rank]->shm_fastmsg_queue
        [SCTK_SHM_RPC_FAST] );

    sctk_nodebug ( "New message!" );

    result = sctk_shm_get_ptp_msg ( sctk_local_process_rank,
        &rpc,
        sizeof ( struct sctk_shm_rpc_s ),
        &size, SCTK_SHM_RPC_FAST, &ret );

    if ( result != -1 ) {
      rpc_slot = ret.selected_slot;

      sctk_nodebug ( "BEGIN sctk_net_adm_poll_shm (slot %p)", rpc_slot );

      args = sctk_malloc ( SCTK_SHM_RPC_ARGS_MAXLEN );
      memcpy ( args, rpc.shm_rpc_args, rpc.shm_rpc_size_args );

      sctk_nodebug ( "Arg size : %d - %d", rpc.shm_rpc_size_args,
          * ( ( int * ) args ) );

      /* execute the RPC */
      sctk_rpc_execute ( rpc.shm_rpc_function, args );
      rpc_slot->is_rpc_acked = 1;
      sctk_nodebug ( "END sctk_net_adm_poll_shm" );

    }
    sctk_shm_mem_struct->
      shm_ptp_queue[sctk_local_process_rank]->shm_fastmsg_queue
      [SCTK_SHM_RPC_FAST]->flag_new_msg_received = 0;

    rpc_slot = NULL;
  }
  return NULL;
}



/*-----------------------------------------------------------
 *                          RPC
 *----------------------------------------------------------*/


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_net_rpc_send_driver
 *  Description:  NOT USED
 * ==================================================================
 */
static void
sctk_net_rpc_send_driver ( void *dest, void *src, size_t arg_size, int process,
    int *ack ) {
  DBG_S ( 0 );
  not_reachable ();
  assume ( dest );
  assume ( src );
  assume ( arg_size );
  assume ( process );
  assume ( ack );
  DBG_E ( 0 );
}



/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_net_rpc_retrive_driver
 *  Description:  Function which send more argument to another process.
 *
 *  dest :        pointer to the local address, where the data must be
 *                stored
 *  src :         pointer to the remote address, where the data must be
 *                grab
 * ==================================================================
 */
static void
sctk_net_rpc_retrive_driver ( void *dest, void *src, size_t arg_size,
    int process, int *ack ) {
  sctk_nodebug
    ( "BEGIN - net_rpc_retrive_driver (dest : %p - src : %p - process %d -  size : %d)",
      ( int * ) dest, ( int * ) src, sctk_local_process_rank, arg_size );

  sctk_shm_rdma_read ( process, src, dest, arg_size );

  /* we acquite the msg */
  sctk_shm_rdma_write ( process, ack, 1, sizeof ( int ) );
  DBG_E ( 0 );
}


/*-----------------------------------------------------------
 *                      PTP
 *----------------------------------------------------------*/

/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_net_ptp_poll_shm_thread
 *  Description:  function which loops to check if new DATA msg are
 *                available
 * ==================================================================
 */
static void *
sctk_net_ptp_poll_shm_thread ( void *arg ) {
  /* PTP deamon */
  void *result;
  sctk_thread_ptp_message_t *msg;
  char *buffer;

  while ( 1 ) {
    if ( sctk_thread_mutex_trylock ( &ptp_received_lock ) == 0 ) {
      result = sctk_shm_get_ptp_fastmsg ( sctk_local_process_rank, 0 );

      /* if a message is received */
      if ( result != NULL ) {
        sctk_nodebug
          ( "THREAD - BEGIN sctk_ne_ptp_poll_shm - New message PTP received. Content (%p)",
            result );

        msg = ( sctk_thread_ptp_message_t * ) result;
        buffer = ( ( char * ) msg + sizeof ( sctk_thread_ptp_message_t ) );
        msg->net_mesg = buffer;

        sctk_send_message ( msg );

        sctk_nodebug ( "RANK %d - PTP received from", sctk_local_process_rank );
      }

      sctk_thread_mutex_unlock ( &ptp_received_lock );
    }
    usleep ( 20 );
  }
  return NULL;
}



/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_net_ptp_poll_shm
 *  Description:  Funtion which polls a new msg
 * ==================================================================
 */
static void
sctk_net_ptp_poll_shm ( void *arg ) {
  /* PTP deamon */
  void *result = NULL;
  sctk_thread_ptp_message_t *msg;
  char *buffer;

  if ( sctk_thread_mutex_trylock ( &ptp_received_lock ) == 0 ) {
    result = sctk_shm_get_ptp_fastmsg ( sctk_local_process_rank, 1 );

    /* if a message is received */
    if ( result != NULL ) {
      sctk_nodebug
        ( "POLL - BEGIN sctk_ne_ptp_poll_shm - New message PTP received. Content (%p)",
          result );

      msg = ( sctk_thread_ptp_message_t * ) result;
      buffer = ( ( char * ) msg + sizeof ( sctk_thread_ptp_message_t ) );
      msg->net_mesg = buffer;

      sctk_send_message ( msg );

      sctk_nodebug ( "RANK %d - PTP received from", sctk_local_process_rank );
    }
    sctk_thread_mutex_unlock ( &ptp_received_lock );
  }

}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_net_send_ptp_message_driver
 *  Description:  function which sends a ptp message.
 * ==================================================================
 */
static void
sctk_net_send_ptp_message_driver ( sctk_thread_ptp_message_t * msg,
    int dest_process ) {
  size_t msg_size;

  msg_size = sctk_net_determine_message_size ( msg );

  sctk_nodebug ( "BEGIN Sending PTP message to process %d (size %d)",
      sctk_local_process_rank, sizeof ( sctk_thread_ptp_message_t ) + msg_size );

  module_pointers->net_ptp_poll        = NULL;	/* PTP */
  sctk_shm_put_ptp_msg ( dest_process,
      msg, sizeof ( sctk_thread_ptp_message_t ), msg_size );
  module_pointers->net_ptp_poll        = sctk_net_ptp_poll_shm;	/* PTP */

  /* msg has been read, we acquite */
  msg->completion_flag = 1;

  DBG_E(0);
}



/*
 * ===  FUNCTION  ===================================================
 *         Name:  sckt_net_copy_messag_func_driver
 *  Description:  NOT USED
 * ==================================================================
 */
static void
sctk_net_copy_message_func_driver ( sctk_thread_ptp_message_t * restrict dest,
    sctk_thread_ptp_message_t * restrict src ) {
  not_reachable ();
  assume ( dest );
  assume ( src );
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_net_free_func_friver
 *  Description:  free a message when it's read
 * ==================================================================
 */
static void
sctk_net_free_func_driver ( sctk_thread_ptp_message_t * item ) {
  DBG_S(0);
  sctk_free ( item );
  DBG_E(0);
}

/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_init_printinfos
 *  Description:  print informations about the SHM
 * ==================================================================
 */
static void
sctk_shm_init_printinfos () {
  size_t size_allocated_mem = sctk_shm_get_allocated_mem_size ();
  int i;

  fprintf ( stderr,
      "\n# ---------------------------NODE %d | QUEUE INFORMATIONS---------------------------\n", sctk_node_rank );
  fprintf ( stderr, "# Initialization finished by rank %d\
      \n# \tFile mapped into memory. shm_mem_map=%p\n# \n", sctk_local_process_rank, sctk_shm_get_memstruct_base () );
  fprintf ( stderr,
      "# - Total allowed SHM size                                           : %d ko\n",
      SCTK_SHM_LEN / ( 1024 ) );
  fprintf ( stderr,
      "# - Global size allocated in the SHM                                 : %ld ko\n",
      size_allocated_mem / ( 1024 ) );
  fprintf ( stderr,
      "# - Average Size allocated in the SHM per process                    : %ld ko\n",
      size_allocated_mem / ( 1024 * sctk_local_process_number ) );
  fprintf ( stderr,
      "# - Size allocated for fast messages queue per process               : %ld ko\n",
      ( SCTK_SHM_FASTMSG_QUEUE_LEN *
        sizeof ( struct sctk_shm_fastmsg_slot_s ) ) / ( 1024 ) );
  fprintf ( stderr,
      "# - Size allocated for fast broadcast messages queue per process     : %ld ko\n",
      ( SCTK_SHM_FASTMSG_BROADCAST_QUEUE_LEN *
        sizeof ( struct sctk_shm_fastmsg_slot_s ) ) / ( 1024 ) );
  fprintf ( stderr,
      "# - Size allocated for big messages queue per process                : %d ko\n",
      ( SCTK_SHM_BIGMSG_QUEUE_LEN * SCTK_SHM_BIGMSG_DATA_MAXLEN ) /
      ( 1024 ) );
  fprintf ( stderr,
      "# - Size allocated for big rpc messages queue                        : %d ko\n",
      ( SCTK_SHM_BIGMSG_RPC_QUEUE_LEN *
        SCTK_SHM_BIGMSG_RPC_MAXLEN ) / ( 1024 ) );
  fprintf ( stderr,
      "# - Size allocated for the big broadcast messages queue (COM_WORLD)  : %d ko\n",
      ( SCTK_SHM_BIGMSG_BROADCAST_QUEUE_LEN *
        SCTK_SHM_BIGMSG_BROADCAST_MAXLEN ) / ( 1024 ) );
  fprintf ( stderr,
      "# - Size allocated for the big reduce messages queue (COM_WORLD)     : %d ko\n",
      ( SCTK_SHM_BIGMSG_REDUCE_QUEUE_LEN *
        SCTK_SHM_BIGMSG_REDUCE_MAXLEN ) / ( 1024 ) );
  fprintf ( stderr,
      "# - Size allocated for rpc messages queue per process                : %ld ko\n",
      ( SCTK_SHM_RPC_QUEUE_LEN *
        sizeof ( struct sctk_shm_rpc_slot_s ) ) / ( 1024 ) );
  fprintf ( stderr,
      "# --------------------------------------------------------------------------------\n\n" );
  fprintf ( stderr,
      "\n# ------------------------NODE %d | TRANSLATION INFORMATIONS------------------------\n", sctk_node_rank );

  for ( i = 0; i < sctk_local_process_number; ++i ) {
    fprintf ( stderr,
        "# Local process rank : %d   ->   Global process rank : %d\n",
        i,
        sctk_shm_mem_struct->shm_local_to_global_translation_table[i] );
  }
  fprintf ( stderr,
      "# --------------------------------------------------------------------------------\n\n" );
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_init
 *  Description:  initialization of the SHM.
 *                Function which creates a file descriptor that is deleted
 *                when all processes have join the SHM (for safety)
 *
 *                / ! \ CARE
 *                We are not sure that the sctk_malloc will return the same
 *                address for each processes. It can failed but we don't
 *                have any other solution...
 * ==================================================================
 */
static int
sctk_shm_init ( int init ) {
  void *tmp_ptr_shm_current = NULL;
  int shm_fd;
  unsigned long page_size;
  void* ptr;
  char* local_host;
  char* shm_key;
  char* shm_filename;

  //assume(sctk_bootstrap_get_max_key_len() >= SHM_FILENAME_SIZE);

  shm_key = sctk_malloc(sctk_bootstrap_get_max_key_len());
  assume(shm_key);
  shm_filename = sctk_malloc(sctk_bootstrap_get_max_key_len());
  assume(shm_filename);

  local_host = sctk_mpcrun_client_get_hostname();
  sprintf(shm_key, "SHM.%s", local_host);

  sctk_nodebug ( "sctk_local_process_rank = %d", sctk_local_process_rank );
  sctk_nodebug ( "sctk_local_process_number = %d", sctk_local_process_number );

  /* get the size of system paged */
  page_size = getpagesize ();
  /* malloc the shared memory segment */
  ptr = sctk_memalign(page_size, SCTK_SHM_LEN);
  assume(ptr);

  /* process with rank 0 opens the shared memory
   * other processes wait until rank 0 finishes
   * */
  if ( sctk_local_process_rank == init ) {

    sctk_mpcrun_client_forge_shm_filename ( shm_filename );
    sctk_nodebug ( "SHM filename generated: %s", shm_filename );

    sctk_bootstrap_register(shm_key, shm_filename, SHM_FILENAME_SIZE);

    /* open shared memory */
    shm_fd =
      shm_open ( shm_filename, O_CREAT | O_EXCL | O_RDWR, S_IRWXU | S_IRWXG );
    if (shm_fd < 0)
    {
      sctk_error("SHM file cannot be created : filename already exists in /dev/shm");
      sctk_abort();
    }

    /* truncate the shared memory */
    assume (ftruncate ( shm_fd, SCTK_SHM_LEN) == 0);

    tmp_ptr_shm_current = ( void * )
      mmap ( ptr, SCTK_SHM_LEN, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, shm_fd, 0 );
    sctk_nodebug("Pointer : %p", tmp_ptr_shm_current);
    assume ( tmp_ptr_shm_current );

    /* create the structure mapped in the shared memory segment */
    sctk_shm_init_mem_struct ( tmp_ptr_shm_current, ptr );
  }

  sctk_nodebug ( "Waiting" );
  sctk_bootstrap_barrier();

  sctk_nodebug ( "Barrier 1 for process %d", sctk_local_process_rank );

  if ( sctk_local_process_rank != init ) {
    int shm_fd;
    struct mmap_infos_s *mem_init;

    sctk_bootstrap_get(shm_key, shm_filename, SHM_FILENAME_SIZE);
    sctk_nodebug ( "SHM filename got: %s", shm_filename );

    sctk_nodebug ( "Filename : %s", shm_filename );

    /* clients open the shared memory */
    shm_fd = shm_open ( shm_filename, O_RDWR, S_IRWXU | S_IRWXG );
    assume( shm_fd >= 0 );

    assume (ftruncate(shm_fd, SCTK_SHM_LEN) == 0);

    tmp_ptr_shm_current = ( void * ) mmap ( ptr,
        SCTK_SHM_LEN, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, shm_fd, 0 );
    assume (tmp_ptr_shm_current);

    mem_init = tmp_ptr_shm_current;

    /* check if the address from the init is the same in the current process
     * CARE : MPC can crash during the init if the malloc doesn't return the same
     * address in each process
     * TODO Change this implementation when new memory allocation will be developped */
    assume(ptr == mem_init->malloc_base);

    sctk_shm_mem_struct = mem_init->mem_base;
    ptr_shm_current = mem_init->malloc_ptr;
  }

  sctk_nodebug ( "Barrier 2 (pre) for process %d", sctk_local_process_rank );
  sctk_bootstrap_barrier();
  sctk_nodebug ( "Barrier 2 for process %d", sctk_local_process_rank );

  /* remove the shm file when all processes have
   * opened the shared memory */
  if ( sctk_local_process_rank == init ) {

    /* unlink the SHM segment */
    assume ( shm_unlink ( shm_filename ) == 0 );
  }

  sctk_nodebug ( "Init done by process %d", sctk_local_process_rank );
  sctk_nodebug ( "Pointers : %p", sctk_shm_mem_struct );
  return 0;
}



/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_barrier
 *  Description:  Perform a barrier
 * ==================================================================
 */
#warning "Have to propagate reinitialize for migration support"
static inline void
sctk_shm_barrier ( sctk_collective_communications_t * com,
    sctk_virtual_processor_t * my_vp ) {
  struct sctk_shm_barrier_slot_s *barrier_slot;
  struct sctk_shm_com_list_s *com_list;
  int i;

  /* select the right com list */
  com_list = sctk_shm_select_right_com_list ( com );
  sctk_assert ( com_list != NULL );

  barrier_slot = & ( com_list->shm_barrier_slot );

  sctk_nodebug ( "BEGIN sctk_shm_barrier. Involved processes : %d. Com (%d)",
      com_list->nb_process_involved, com->id );

  if ( com_list->nb_process_involved != 1 ) {

    sctk_spinlock_lock ( & ( barrier_slot->spin_total ) );
    barrier_slot->total++;

    sctk_nodebug ( "involved : %d, total : %d",
        com_list->nb_process_involved, barrier_slot->total );

    if ( com_list->nb_process_involved == barrier_slot->total ) {
      sctk_spinlock_unlock ( & ( barrier_slot->spin_total ) );

      barrier_slot->rank[sctk_local_process_rank] = 1;

      sctk_nodebug ( "RANK %d - Barrier init step 1", sctk_local_process_rank );

      /* wait for all processes */
      for ( i = 0; i < com_list->nb_process_involved; ++i ) {
        int proc = com_list->involved_process_list[i];
        sctk_nodebug("Wait for process %d", proc);
        sctk_thread_wait_for_value ( ( int * ) & ( barrier_slot->rank[proc] ), 1 );
        sctk_nodebug("Ok for rank %d", i);
      }

      memset ( barrier_slot->done, 0, com_list->nb_process_involved );

      barrier_slot->rank[sctk_local_process_rank] = 0;
      barrier_slot->lock = 1;

      sctk_nodebug ( "RANK %d - Barrier init step 2", sctk_local_process_rank );
      for ( i = 0; i < com_list->nb_process_involved; ++i ) {
        int proc = com_list->involved_process_list[i];
        sctk_thread_wait_for_value ( ( int * ) & ( barrier_slot->rank[proc] ), 0 );
        sctk_nodebug ( "Ok for rank %d", i );
      }

      sctk_spinlock_lock ( & ( barrier_slot->spin_total ) );
      barrier_slot->total = 0;
      sctk_spinlock_unlock ( & ( barrier_slot->spin_total ) );

      sctk_nodebug ( "RANK %d - Barrier init step 3", sctk_local_process_rank );
      barrier_slot->lock = 0;
    } else {
      sctk_spinlock_unlock ( & ( barrier_slot->spin_total ) );

      sctk_nodebug ( "RANK %d - Barrier other step 1", sctk_local_process_rank );
      barrier_slot->rank[sctk_local_process_rank] = 1;

      sctk_thread_wait_for_value ( ( int * ) & ( barrier_slot->lock ), 1 );

      barrier_slot->rank[sctk_local_process_rank] = 0;

      sctk_nodebug ( "RANK %d - Barrier other step 2", sctk_local_process_rank );

      sctk_thread_wait_for_value ( ( int * ) & ( barrier_slot->lock ), 0 );

      sctk_nodebug ( "RANK %d - Barrier other step 3", sctk_local_process_rank );
    }
  }
  DBG_E(0);
}



/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_broadcast
 *  Description:  Perform a broadcast
 * ==================================================================
 */
#warning "Have to propagate reinitialize for migration support"
static inline void
sctk_shm_broadcast ( sctk_collective_communications_t * com,
    sctk_virtual_processor_t * my_vp,
    const size_t elem_size, const size_t nb_elem ) {
  int size;
  struct sctk_shm_broadcast_slot_s *broadcast_slot;
  struct sctk_shm_com_list_s *com_list;

  com_list = sctk_shm_select_right_com_list ( com );
  sctk_assert ( com_list );

  sctk_nodebug ( "BEGIN broadcast" );
  sctk_nodebug ( "int : %s, out : %p", my_vp->data.data_in,
      my_vp->data.data_out );

  size = elem_size * nb_elem;
  broadcast_slot = & ( com_list->shm_broadcast_slot );

  if ( my_vp->data.data_in != NULL ) {

    sctk_nodebug ( "trying to send a broadcast msg" );

    sctk_shm_put_broadcast_msg ( my_vp->data.data_in, size, com_list, 0 );
    sctk_nodebug ( "broadcast msg sent" );

    if ( my_vp->data.data_out != NULL ) {
      memcpy ( my_vp->data.data_out, my_vp->data.data_in, size );
    }
  } else {
    sctk_nodebug ( "Trying to GET a fast broadcast msg (ready : %d)",
        broadcast_slot->shm_is_msg_ready );

    /* wait until the broadcast msg is ready */
    sctk_thread_wait_for_value ( ( int * ) & ( broadcast_slot->shm_is_msg_ready ),
        1 );

    sctk_shm_get_broadcast_msg ( sctk_local_process_rank,
        my_vp->data.data_out, size, com_list );

    sctk_nodebug ( "MESSAGE GET", broadcast_slot->shm_is_msg_ready );
  }

  sctk_shm_barrier ( com, my_vp );
  broadcast_slot->shm_is_msg_ready = 0;
  sctk_shm_barrier ( com, my_vp );

  DBG_E( 0 );
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_allreduce
 *  Description:  Perform a reduction operation
 * ==================================================================
 */
#warning "Have to propagate reinitialize for migration support"
static inline void
sctk_shm_allreduce ( sctk_collective_communications_t * com,
    sctk_virtual_processor_t * my_vp,
    const size_t elem_size,
    const size_t nb_elem,
    void ( *func ) ( const void *, void *, size_t,
      sctk_datatype_t ),
    const sctk_datatype_t data_type ) {

  int size;
  void *incoming_buff = NULL;
  int i;
  struct sctk_shm_com_list_s *com_list;

  com_list = sctk_shm_select_right_com_list ( com );
  sctk_assert ( com_list != NULL );

  size = elem_size * nb_elem;

  sctk_nodebug ( "int : %d, out : %d, nb tasks involved (%d)",
      ( ( int * ) my_vp->data.data_in ) [0],
      ( ( int * ) my_vp->data.data_out ) [0],
      com_list->nb_process_involved );

  /* send a collective message */
  if ( sctk_local_process_rank == 0 ) {
    int task_list[SCTK_SHM_MAX_NB_LOCAL_PROCESSES];
    int nb_task_done = com_list->nb_process_involved - 1;

    int index;

    for ( index = 0; index < com_list->nb_process_involved; ++index ) {
      task_list[index] = com_list->involved_task_list[index];
    }

    incoming_buff = sctk_malloc ( size );

    sctk_nodebug ( "Nb process inv %d", nb_task_done );

    while ( nb_task_done != 0 ) {
      for ( index = 0; index < ( com_list->nb_process_involved - 1 );
          ++index ) {
        sctk_nodebug ( "Try to get at index %d : %d", index,
            com_list->shm_reduce_slot.
            shm_is_msg_ready[index] );

        while ( com_list->shm_reduce_slot.shm_is_msg_ready[index] == -1 ) {
          sctk_thread_yield ();
        }

        sctk_nodebug ( "Get msg" );

        sctk_get_reduce_msg ( com_list->shm_reduce_slot.
            shm_is_msg_ready[index],
            com_list->shm_reduce_slot.msg_size[index],
            incoming_buff, com_list );

        sctk_nodebug ( "Input received : %d",
            ( ( int * ) incoming_buff ) [0] );

        /* function execution */
        func ( incoming_buff, my_vp->data.data_out, nb_elem, data_type );
        sctk_nodebug ( "data_out %d", ( ( int * ) my_vp->data.data_out ) [0] );

        --nb_task_done;
      }
    }
    /* Final broadcast phase */
    sctk_nodebug ( "Broadcast result : %d (size %d)",
        ( ( int * ) my_vp->data.data_out ) [0], size );

    sctk_shm_put_broadcast_msg ( my_vp->data.data_out, size, com_list, 1 );
    sctk_nodebug ( "OK" );
  } else {
    sctk_nodebug ( "Input : %p", my_vp->data.data_in );

    sctk_shm_put_reduce_msg ( 0,
        my_vp->data.data_out, size, com_list );

    sctk_nodebug ( "Reduce sent (%d) - Waiting....",
        ( ( int * ) my_vp->data.data_in ) [0] );

    sctk_thread_wait_for_value ( &
        ( com_list->
          shm_reduce_slot.shm_is_broadcast_ready ),
        1 );

    sctk_nodebug ( "End waiting" );

    sctk_shm_get_broadcast_msg ( sctk_local_process_rank,
        my_vp->data.data_out, size, com_list );

    sctk_nodebug ( "Result received from broadcast: %d",
        ( ( int * ) my_vp->data.data_out ) [0] );
  }
  sctk_shm_barrier ( com, my_vp );

  if ( sctk_local_process_rank == 0 ) {
    for ( i = 0; i < SCTK_SHM_MAX_NB_LOCAL_PROCESSES; ++i ) {
      /* initialize reduce slot */
      com_list->shm_reduce_slot.shm_is_msg_ready[i] = -1;
    }
    com_list->shm_reduce_slot.shm_is_broadcast_ready = 0;
    com_list->shm_broadcast_slot.shm_is_msg_ready = 0;
  }
  sctk_nodebug ( "Process %d waiting for final barrier", sctk_local_process_rank );

  sctk_free ( incoming_buff );
  sctk_shm_barrier ( com, my_vp );
  sctk_nodebug ( "End of reduce" );
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_net_collective_op_driver
 *  Description:  when a collective message is received
 * ==================================================================
 */
static void
sctk_net_collective_op_driver ( sctk_collective_communications_t * com,
    sctk_virtual_processor_t * my_vp,
    const size_t elem_size,
    const size_t nb_elem,
    void ( *func ) ( const void *, void *, size_t,
      sctk_datatype_t ),
    const sctk_datatype_t data_type ) {
  sctk_nodebug ( "Nb tasks involved for the barrier: %d\n", com->involved_nb );

  sctk_nodebug ( "begin collective" );
  if ( nb_elem == 0 ) {
    sctk_nodebug ( "begin collective barrier" );
    sctk_shm_barrier ( com, my_vp );
    sctk_nodebug ( "end collective barrier" );
    return;
  } else {
    if ( func == NULL ) {
      sctk_nodebug ( "begin collective broadcast" );
      sctk_shm_broadcast ( com, my_vp, elem_size, nb_elem );
      sctk_nodebug ( "end collective broadcast" );
      return;
    } else {
      sctk_nodebug ( "begin collective reduce" );
      sctk_shm_allreduce ( com, my_vp, elem_size, nb_elem, func,
          data_type );
      sctk_nodebug ( "end collective reduce" );
      return;
    }
  }
  sctk_nodebug ( "end collective" );
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_net_init_driver_shm
 *  Description:  Initialization of the SHM MPC module
 * ==================================================================
 */
void
sctk_net_init_driver_shm ( int *argc, char ***argv ) {
  sctk_thread_t pidt_rdma;
  sctk_thread_attr_t attr_rdma;

  sctk_thread_t pidt_rpc;
  sctk_thread_attr_t attr_rpc;

  sctk_thread_t pidt_ptp;
  sctk_thread_attr_t attr_ptp;

  /* thread for RDMA */
  sctk_thread_attr_init ( &attr_rdma );
  sctk_thread_attr_setscope ( &attr_rdma, SCTK_THREAD_SCOPE_SYSTEM );
  sctk_user_thread_create ( &pidt_rdma, &attr_rdma, sctk_shm_thread_rdma,
      NULL );

  /* thread for RPC */
  sctk_thread_attr_init ( &attr_rpc );
  sctk_thread_attr_setscope ( &attr_rpc, SCTK_THREAD_SCOPE_SYSTEM );
  sctk_user_thread_create ( &pidt_rpc, &attr_rpc, sctk_shm_thread_rpc, NULL );

  sctk_thread_mutex_init ( & ( ptp_received_lock ), NULL );
  /* thread for PTP */
  sctk_thread_attr_init ( &attr_ptp );
  sctk_thread_attr_setscope ( &attr_ptp, SCTK_THREAD_SCOPE_SYSTEM );
  sctk_user_thread_create ( &pidt_ptp, &attr_ptp, sctk_net_ptp_poll_shm_thread,
      NULL );

  sctk_nodebug ( "SHM_init done | process : %d", sctk_process_rank );
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_net_preini_driver_shm
 *  Description:  Preinitialization of the SHM module.
 *                Thread library isn't initialized at this point
 * ==================================================================
 */
void
sctk_net_preinit_driver_shm ( sctk_net_driver_pointers_functions_t* pointers ) {
  int init = 0;			/* process which creates the SHM */

  /* save all pointers to functions */
  pointers->rpc_driver          = sctk_net_rpc_driver;
  pointers->rpc_driver_retrive  = sctk_net_rpc_retrive_driver;
  pointers->rpc_driver_send     = sctk_net_rpc_send_driver;
  pointers->net_send_ptp_message= sctk_net_send_ptp_message_driver;
  pointers->net_copy_message    = sctk_net_copy_message_func_driver;
  pointers->net_free            = sctk_net_free_func_driver;

  pointers->collective          = sctk_net_collective_op_driver;

  pointers->net_adm_poll        = NULL;	/* RPC */
  pointers->net_ptp_poll        = sctk_net_ptp_poll_shm;	/* PTP */

  pointers->net_new_comm        = sctk_shm_init_new_com;
  pointers->net_free_comm       = sctk_shm_free_com;

  module_pointers = pointers;

  /* initialization of the struture into the SHM */
  sctk_shm_init ( init );
  sctk_shm_init_collective_structs ( init );
  sctk_shm_init_world_com ( init );
  /* build translation tables for each process*/
  sctk_shm_fill_translation_tables();


#if SCTK_HYBRID_DEBUG == 1
  /* print informations about the SHM (size of queues,
   * size allocated, etc... */
  sctk_bootstrap_barrier();
  if ( sctk_process_rank == init ) {
    sctk_shm_init_printinfos ();
  }
  sctk_bootstrap_barrier();
#endif

  sctk_nodebug ( "SHM - Module pre-initialized" );
}

/* if SHM module not recquired */
#else
void
sctk_net_init_driver_shm ( int *argc, char ***argv ) {
  assume ( argc != NULL );
  assume ( argv != NULL );
  not_available ();
}


void
sctk_net_preinit_driver_shm ( void ) {
  not_available ();
}


#endif
