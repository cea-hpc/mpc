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
/* #   - DIDELOT Sylvain  sdidelot@exascale-computing.eu                 # */
/* #                                                                      # */
/* ######################################################################## */
#include "sctk_shm.h"

/* pointer to the structure mapped into memory */
struct sctk_shm_mem_struct_s *sctk_shm_mem_struct;

/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_move_first_flag
 *  Description:  Function which moves the first flag of a queue according
 *                to the value of "nb_slots".
 *
 *  __queue : pointer to the queue
 *  nb_slots : nb slots to move
 * ==================================================================
 */
static inline int
sctk_shm_move_first_flag ( void *queue, const int nb_slots ) {
    struct sctk_shm_generic_queue_s *gen_queue = queue;
    int i;

    i = ( gen_queue->ptr_msg_first_slot + nb_slots ) % gen_queue->queue_size;
    gen_queue->ptr_msg_first_slot = i;
    return i;
}

/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_move_last_flag
 *  Description:  Function which moves the last flag of a queue according
 *                to the value of "nb_slots".
 * ==================================================================
 */
static inline int
sctk_shm_move_last_flag ( void *queue, const int nb_slots ) {
    struct sctk_shm_generic_queue_s *gen_queue = queue;
    int i;

    i = ( gen_queue->ptr_msg_last_slot + nb_slots ) % gen_queue->queue_size;
    gen_queue->ptr_msg_last_slot = i;
    return i;
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_put_fastmsg
 *  Description:  Function which adds a msg to the fast msg queue
 *
 *  __dest : dest of the msg
 *  __ payload : msg to send
 *  __size_header : size of the msg's header
 *  __size_payload : size of the msg's payload
 *  ptr_to_slot_queue_type : msg's type (BIG of FAST)
 * ==================================================================
 */
static inline void *
sctk_shm_put_ptp_fastmsg ( const unsigned int __dest,
                           void *__payload,
                           const size_t __size_header,
                           const size_t __size_payload, const int ptr_to_slot_queue_type ) {

    struct sctk_shm_fastmsg_queue_s *fastmsg_queue;
    struct sctk_shm_fastmsg_slot_s *selected_slot;
    int slot_number;

    fastmsg_queue =
        sctk_shm_mem_struct->
        shm_ptp_queue[__dest]->shm_fastmsg_queue[SCTK_SHM_DATA_FAST];
    sctk_assert ( fastmsg_queue != NULL );

    /* spinlock the queue for writing */
    sctk_spinlock_lock ( & ( fastmsg_queue->lock_write ) );

    /* return if the queue is full we leave the function */
    if ( fastmsg_queue->ptr_msg_first_slot ==
            fastmsg_queue->ptr_msg_last_slot ) {
        sctk_spinlock_unlock ( & ( fastmsg_queue->lock_write ) );
        return NULL;
    }

    /* point to correct slot number */
    slot_number = fastmsg_queue->ptr_msg_last_slot;
    /* point to correct slot */
    selected_slot = fastmsg_queue->msg_slot[slot_number];
    selected_slot->msg_size = ( __size_header + __size_payload );

    switch ( ptr_to_slot_queue_type ) {
        /**************************************************
         *                  PTP DATA MESSAGE
         *************************************************/
    case ( SCTK_SHM_SLOT_TYPE_PTP_MESSAGE | SCTK_SHM_SLOT_TYPE_BIGMSG ) :

        sctk_nodebug ( "BIGMSG is being added to the queue" );
        /* update the pointer to a big msg slot */
        selected_slot->shm_msg_type =
            ( SCTK_SHM_SLOT_TYPE_PTP_MESSAGE | SCTK_SHM_SLOT_TYPE_BIGMSG );
        /* The slot is ready to be read */
        selected_slot->is_msg_ready_to_read = 1;
        /* move to the next slot */
        sctk_shm_move_last_flag ( fastmsg_queue, 1 );
        break;

    case ( SCTK_SHM_SLOT_TYPE_PTP_MESSAGE | SCTK_SHM_SLOT_TYPE_FASTMSG ) :
        sctk_nodebug ( "Fastmsg is being added to the queue | size=%d",
                       __size_header + __size_payload );
        /* this is a fast msg slot */
        selected_slot->shm_msg_type =
            ( SCTK_SHM_SLOT_TYPE_PTP_MESSAGE | SCTK_SHM_SLOT_TYPE_FASTMSG );
        /* move last slot pointer */
        sctk_shm_move_last_flag ( fastmsg_queue, 1 );
        /* copy the header of the msg */
        memcpy ( selected_slot->msg_content, __payload, __size_header );
        /* copy the content of the msg in one shot to the slot */
        sctk_net_copy_in_buffer ( __payload, selected_slot->msg_content +
                                  __size_header );
        /* the msg is ready to read */
        selected_slot->is_msg_ready_to_read = 1;
        break;
    }
    sctk_nodebug ( "Message added !" );
    /* release writer spinlock */
    sctk_spinlock_unlock ( & ( fastmsg_queue->lock_write ) );

    return selected_slot;
}



/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_put_ptp_bigmsg
 *  Description:  Function which adds a ptp msg to the big queue
 * ==================================================================
 */
static inline void
sctk_shm_put_ptp_bigmsg ( const unsigned int __dest,
                          const void *__payload,
                          const size_t __size_header, const size_t __size_payload ) {
    struct sctk_shm_bigmsg_queue_s *bigmsg_queue;
    char *selected_slot;
    int nb_available_slots = 0;
    int first_allocation = 1;
    struct sctk_shm_bigmsg_header_s *header;
    size_t total_size;
    int size_tocopy;
    int current_slot;
    int copied_size = 0;
    int tmp_copied_size;

    /* select the correct queue according the msg type */
    bigmsg_queue =
        sctk_shm_mem_struct->shm_ptp_queue[__dest]->shm_bigmsg_queue;
    sctk_assert ( bigmsg_queue != NULL );

    /* lock all other writers which want access to the queue */
    sctk_spinlock_lock_yield ( & ( bigmsg_queue->lock_write ) );
    /* compute the total size of the msg. Composed of slot's header size, msg's header and
     * msg's payload */
    total_size =  __size_payload;
    /* select right slot */
    selected_slot = bigmsg_queue->msg_slot[bigmsg_queue->ptr_msg_last_slot];
    current_slot = bigmsg_queue->ptr_msg_last_slot;

    /* while it remains slots to copy */
    while ( total_size > 0 ) {
        do {
            sctk_spinlock_lock ( & ( bigmsg_queue->lock_read ) );

            /* compute available slots according to the position of first and last
             * flags.*/
            if ( current_slot > ( bigmsg_queue->ptr_msg_first_slot ) ) {
                nb_available_slots = SCTK_SHM_BIGMSG_QUEUE_LEN - current_slot;
            } else if ( current_slot == bigmsg_queue->ptr_msg_first_slot ) {
                /* no slots available for this configuration */
                nb_available_slots = 0;
            } else {
                /*  -1 because all the time, there is a slot free in the queue (between first
                 *  and last flag */
                nb_available_slots = bigmsg_queue->ptr_msg_first_slot - current_slot - 1;
            }

            sctk_spinlock_unlock ( & ( bigmsg_queue->lock_read ) );

            /*  if no slots available -> thread yield to hand over to another thread */
            if ( nb_available_slots == 0 ) {
                sctk_thread_yield ();
            }
        } while ( nb_available_slots <= 0 );

        header = ( struct sctk_shm_bigmsg_header_s * ) selected_slot;

        /* if this is the first allocation, so we write warm the destination that a
        * new bigmsg is available */
        if ( first_allocation == 1 ) {
            first_allocation = 0;

            /* set the size to copy for the current msg */
            size_tocopy = SCTK_SHM_BIGMSG_DATA_MAXLEN -
                          sizeof ( struct sctk_shm_bigmsg_header_s ) - __size_header;

            sctk_nodebug ( "PUT TO %p (%d) size (%d) process (%d)",
                           selected_slot, current_slot, size_tocopy, __dest );

            /* copy the header of the msg */
            memcpy ( ( char * ) selected_slot + sizeof ( struct sctk_shm_bigmsg_header_s ),
                     __payload, __size_header );

            /*  copy the msg content */
            tmp_copied_size = sctk_shm_opt_net_copy_in_buffer ( __payload,
                              ( char * ) selected_slot + sizeof ( struct sctk_shm_bigmsg_header_s )
                              + __size_header, copied_size, size_tocopy );

            while ( sctk_shm_put_ptp_fastmsg ( __dest,
                                               NULL,
                                               __size_header,
                                               __size_payload,
                                               SCTK_SHM_SLOT_TYPE_PTP_MESSAGE
                                               |
                                               SCTK_SHM_SLOT_TYPE_BIGMSG ) == NULL ) {
                sctk_thread_yield ();
            }

            /* build header */
            header->size = ( tmp_copied_size - copied_size ) + __size_header;

            sctk_nodebug ( "Slot added" );
        } else {
            size_tocopy = SCTK_SHM_BIGMSG_DATA_MAXLEN - sizeof ( struct sctk_shm_bigmsg_header_s );

            /*  copy the msg content */
            tmp_copied_size = sctk_shm_opt_net_copy_in_buffer ( __payload,
                              ( char * ) selected_slot + sizeof ( struct sctk_shm_bigmsg_header_s ),
                              copied_size, size_tocopy );

            /* build header */
            header->size = ( tmp_copied_size - copied_size );
        }

        total_size -= ( tmp_copied_size - copied_size );

        sctk_nodebug ( "tot_copied : %d | tot_size : %d | copied_size : %d | msg size : %d",
                       tmp_copied_size - copied_size, total_size, copied_size,
                       __size_header + __size_payload );

        header->nb_slots_allocated = 1;

        if ( total_size > 0 )
            header->remaining = 1;
        else
            header->remaining = 0;

        sctk_nodebug ( "Header size : %d | remaining : %d | total_size : %d",
                       sizeof ( struct sctk_shm_bigmsg_header_s ), header->remaining, total_size );

        /* move the first flag of the queue */
        sctk_shm_move_last_flag ( bigmsg_queue, 1 );

        copied_size = tmp_copied_size;

        /*  pointer to the curent position in the msg */
        current_slot =
            ( current_slot + 1 ) % SCTK_SHM_BIGMSG_QUEUE_LEN;

        selected_slot = bigmsg_queue->msg_slot[current_slot];

        sctk_nodebug ( "PUT first slot : %d - last slot : %d",
                       bigmsg_queue->ptr_msg_first_slot,
                       bigmsg_queue->ptr_msg_last_slot );

    }
    /* release the spinlock -> other processes can write */
    sctk_spinlock_unlock ( & ( bigmsg_queue->lock_write ) );

    sctk_nodebug ( "Ptp Bigmsg added successfully" );
    return;
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_get_ptp_bigmsg
 *  Description:  Function which gets a msg from a bigmsg queue.
 *
 *  __rank : rank of the procress where grabbing the msg
 *  __msg_content : pointer to the address where saving the msg's content
 *
 * ==================================================================
 */
static inline void
sctk_shm_get_ptp_bigmsg ( const unsigned int __rank, void *__msg_content ) {
    struct sctk_shm_bigmsg_queue_s *bigmsg_queue;
    struct sctk_shm_bigmsg_slot_s *selected_slot;
    char *current_position = __msg_content;
    struct sctk_shm_bigmsg_header_s *header;
    int remaining;

    bigmsg_queue =
        sctk_shm_mem_struct->shm_ptp_queue[__rank]->shm_bigmsg_queue;
    current_position = __msg_content;

    do {
        sctk_nodebug ( "Trying GET First (%d) last (%d) queue (%p)",
                       bigmsg_queue->next_slots[bigmsg_queue->
                                                ptr_msg_first_slot],
                       bigmsg_queue->ptr_msg_last_slot, bigmsg_queue );

        /* waiting while the queue of big messages is empty. Cannot exit the function
        * because the msg must read at once */
        while ( bigmsg_queue->next_slots[bigmsg_queue->ptr_msg_first_slot] ==
                bigmsg_queue->ptr_msg_last_slot ) {
            sctk_thread_yield ();
        }

        selected_slot = bigmsg_queue->
                        msg_slot[bigmsg_queue->next_slots[bigmsg_queue->ptr_msg_first_slot]];

        /* grab the header of the slot */
        header = ( struct sctk_shm_bigmsg_header_s * ) selected_slot;

        sctk_nodebug
        ( "Nb slots extracted : %d from %p (slot %p) (remaining : %d) | slot nb %d | size %d",
          header->nb_slots_allocated,
          ( char * ) selected_slot +
          sizeof ( struct sctk_shm_bigmsg_header_s ), selected_slot,
          header->remaining,
          bigmsg_queue->next_slots[bigmsg_queue->ptr_msg_first_slot],
          header->size
        );

        /* read the content of the msg */
        memcpy ( current_position,
                 ( char * ) selected_slot +
                 sizeof ( struct sctk_shm_bigmsg_header_s ), header->size );

        /* inc the current position in the msg */
        current_position += header->size;

        /* update first slot */
        remaining = header->remaining;
        sctk_spinlock_lock ( & ( bigmsg_queue->lock_read ) );
        bigmsg_queue->ptr_msg_first_slot =
            ( bigmsg_queue->ptr_msg_first_slot + header->nb_slots_allocated ) % bigmsg_queue->queue_size;
        sctk_spinlock_unlock ( & ( bigmsg_queue->lock_read ) );

        sctk_nodebug ( "Bigmsg extracted, remain : %d", remaining );
    } while ( remaining == 1 );

    return;
}

/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_put_reduce_bigmsg
 *  Description:  Add a message into the bigmsg reduce queue.
 *
 *  __shm_msg : Msg to write
 *  __size : Size of the msg
 *  __com_list : Pointer to the communicator
 * ==================================================================
 */
static inline void
sctk_shm_put_reduce_bigmsg ( const void *__shm_msg,
                             const size_t __size,
                             struct sctk_shm_com_list_s *__com_list ) {
    struct sctk_shm_bigmsg_queue_s *bigmsg_queue = NULL;
    struct sctk_shm_bigmsg_reduce_slot_s *selected_slot = NULL;

    int selected_slot_number;
    const char *msg_tocopy = __shm_msg;
    int current_size = __size;
    int size_tocopy;
    int msg_remaining;
    int first_allocation = 0;
    int i;

    DBG_S ( 0 );
    bigmsg_queue = __com_list->reduce_bigmsg_queue;

    sctk_nodebug ( "Pointer : %p", __com_list );
    /* lock all writers on the queue */
    sctk_spinlock_lock ( & ( bigmsg_queue->lock_write ) );

    /* while there are slots to write */
    while ( current_size > 0 ) {
        selected_slot_number = bigmsg_queue->ptr_msg_last_slot;

        /* point to the correct slot */
        selected_slot =
            bigmsg_queue->msg_slot[selected_slot_number];

        sctk_nodebug ( "com : %d, ici %d, next %d, first %d last %d",
                       __com_list->com_id, selected_slot_number,
                       bigmsg_queue->next_slots[selected_slot_number],
                       bigmsg_queue->ptr_msg_first_slot,
                       bigmsg_queue->ptr_msg_last_slot );

        /* keeps the lock until a slot is freed */
        while ( selected_slot_number == bigmsg_queue->ptr_msg_first_slot ) {
            sctk_thread_yield ();
        }

        /* if it remains data to copy */
        if ( current_size > SCTK_SHM_BIGMSG_REDUCE_MAXLEN ) {
            size_tocopy = SCTK_SHM_BIGMSG_REDUCE_MAXLEN;
            msg_remaining = 1;
            current_size -= SCTK_SHM_BIGMSG_REDUCE_MAXLEN;
        } else {		/* if there is no more data to copy */
            size_tocopy = current_size;
            msg_remaining = 0;
            current_size = 0;
        }

        memcpy ( selected_slot->msg_content, msg_tocopy, size_tocopy );

        /* updates variables */
        sctk_nodebug ( "PUT msg_remaining(%p) : %d",
                       & ( selected_slot->msg_remaining ), msg_remaining );

        selected_slot->msg_remaining = msg_remaining;
        selected_slot->msg_size = size_tocopy;

        /* indicates that the slot is ready to be read */
        sctk_shm_move_last_flag ( bigmsg_queue, 1 );

        /* warn the process that a new msg is available */
        if ( !first_allocation ) {
            first_allocation = 1;
            sctk_nodebug ( "Selected slot : %d", selected_slot_number );

            __com_list->shm_reduce_slot.shm_msg_type =
                SCTK_SHM_SLOT_TYPE_PTP_MESSAGE | SCTK_SHM_SLOT_TYPE_BIGMSG;

            /* set the msg ready */
            for ( i = 0; i < SCTK_SHM_MAX_NB_LOCAL_PROCESSES; ++i ) {
                if ( __com_list->shm_reduce_slot.shm_is_msg_ready[i] == -1 ) {
                    __com_list->shm_reduce_slot.shm_is_msg_ready[i] = sctk_local_process_rank;
                    __com_list->shm_reduce_slot.msg_size[i] = __size;
                    sctk_nodebug ( "Process %d in index %d", sctk_local_process_rank, i );
                    break;
                }
            }
        }

        sctk_nodebug ( "SLOT %d write !!", selected_slot_number );
        msg_tocopy += size_tocopy;
    }
    /* release the spinlock -> other processes can write */
    sctk_spinlock_unlock ( & ( bigmsg_queue->lock_write ) );

    DBG_E ( 0 );
    return;
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  shm_get_reduce_bigmsg
 *  Description:  get a reduce msg from the bigmsg queue.
 *
 *  __msg_content : pointer to the variable where save the msg
 *  __com_list : pointer to communicator
 * ==================================================================
 */
static inline void
sctk_shm_get_reduce_bigmsg ( void *__msg_content,
                             struct sctk_shm_com_list_s *__com_list ) {
    struct sctk_shm_bigmsg_queue_s *bigmsg_queue;
    struct sctk_shm_bigmsg_reduce_slot_s *selected_slot;
    char *msg_tocopy;
    int remain;
    int size_tocopy;
    int current_slot;

    msg_tocopy = __msg_content;
    bigmsg_queue = __com_list->reduce_bigmsg_queue;

    DBG_S ( 0 );
    do {

        current_slot =
            bigmsg_queue->next_slots[bigmsg_queue->ptr_msg_first_slot];
        selected_slot = bigmsg_queue->msg_slot[current_slot];

        /* wait until the big msg is ready to be read */
        while ( current_slot == bigmsg_queue->ptr_msg_last_slot ) {
            sctk_thread_yield ();
        }

        sctk_nodebug ( "More than 1 bigmsg slot" );
        size_tocopy = selected_slot->msg_size;

        memcpy ( msg_tocopy, selected_slot->msg_content, size_tocopy );

        sctk_nodebug ( "GET %d  msg_remaining(%p) : %d",
                       current_slot,
                       & ( selected_slot->msg_remaining ),
                       selected_slot->msg_remaining );

        /*  if msg are remaining */
        remain = selected_slot->msg_remaining;

        msg_tocopy += size_tocopy;

        /* update last slot */
        sctk_shm_move_first_flag ( bigmsg_queue, 1 );
    } while ( remain );

    DBG_E ( 0 );
    return;
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_put_collective_bigmsg
 *  Description:  Put a broadcast msg into the bigmsg queue
 *
 *  __shm_msg : the msg to broadcast
 *  __size : size of the msg to broadcast
 *  __com_list : pointer to the communicator
 *  __queue : type of the queue
 * broadcast_slot : slot of the fastmsg broadcast
 * is_reduce : is the following broadcast come from a reduce operation
 * ==================================================================
 */
inline void
sctk_shm_put_broadcast_bigmsg ( const void *__shm_msg,
                                size_t __size,
                                struct sctk_shm_com_list_s *__com_list,
                                const int __queue_type,
                                struct sctk_shm_broadcast_slot_s *broadcast_slot,
                                const int is_reduce ) {
    DBG_S ( 0 );

    struct sctk_shm_bigmsg_queue_s *bigmsg_queue = NULL;
    struct sctk_shm_bigmsg_broadcast_slot_s *selected_slot = NULL;

    int nb_elements;
    int selected_slot_number;
    const char *msg_tocopy = __shm_msg;
    int current_size = __size;
    int size_tocopy;
    int msg_remaining;
    int first_allocation = 0;

    /* total of processes expected */
    nb_elements = __com_list->nb_process_involved;

    bigmsg_queue = __com_list->broadcast_bigmsg_queue;

    sctk_spinlock_lock ( & ( bigmsg_queue->lock_write ) );

    while ( current_size > 0 ) {
        selected_slot =
            bigmsg_queue->msg_slot[bigmsg_queue->ptr_msg_last_slot];
        selected_slot_number = bigmsg_queue->ptr_msg_last_slot;

        sctk_nodebug ( "BEGIN put_broadcast_bigmsg (size : %d) slot (%d - %p) ",
                       __size, bigmsg_queue->ptr_msg_last_slot,
                       selected_slot );

        sctk_nodebug ( "next (%d) first_slot (%d)",
                       bigmsg_queue->next_slots[selected_slot_number],
                       bigmsg_queue->ptr_msg_first_slot );
        /* keeps the lock until a slot is freed */
        while ( selected_slot_number == bigmsg_queue->ptr_msg_first_slot ) {
            sctk_thread_yield ();
        }

        /* set the attrs of the slot */
        selected_slot->total_elements = nb_elements - 1;
        selected_slot->current_elements = 0;

        /* if it remains data to copy */
        if ( current_size > SCTK_SHM_BIGMSG_BROADCAST_MAXLEN ) {
            sctk_nodebug ( "PUT MORE THAN ONE MSG" );
            size_tocopy = SCTK_SHM_BIGMSG_BROADCAST_MAXLEN;
            msg_remaining = 1;
            current_size -= SCTK_SHM_BIGMSG_BROADCAST_MAXLEN;
        } else {		/* if there is no more data to copy */
            size_tocopy = current_size;
            msg_remaining = 0;
            current_size = 0;
        }

        memcpy ( selected_slot->msg_content, msg_tocopy, size_tocopy );

        /* updates variables */
        selected_slot->msg_remaining = msg_remaining;
        sctk_nodebug ( "PUT msg_remaining(%p) : %d",
                       & ( selected_slot->msg_remaining ),
                       selected_slot->msg_remaining );
        selected_slot->msg_size = size_tocopy;
        sctk_nodebug ( "SLOT %d write !!", selected_slot_number );

        msg_tocopy += size_tocopy;

        /* indicates that the slot is ready to be read */
        selected_slot->is_msg_ready_to_read = 1;

        if ( !first_allocation ) {
            sctk_nodebug ( "First allocation! slot %d",
                           selected_slot_number );
            first_allocation = 1;
            /* warm processes that the msg is available */
            broadcast_slot->shm_msg_type =
                SCTK_SHM_SLOT_TYPE_COLLECTIVE_BROADCAST | SCTK_SHM_SLOT_TYPE_BIGMSG;

            sctk_nodebug ( "fastmsg %d", broadcast_slot->shm_msg_type );

            /* update broadcast slot values */
            broadcast_slot->msg_size = __size;
            broadcast_slot->shm_is_msg_ready = 1;

            /* if this is a broadcast from a reduce */
            if ( is_reduce == 1 )
                __com_list->shm_reduce_slot.shm_is_broadcast_ready = 1;
        }

        /* update slots */
        sctk_shm_move_last_flag ( bigmsg_queue, 1 );
    }

    /* release the spinlock -> other processes can write */
    sctk_spinlock_unlock ( & ( bigmsg_queue->lock_write ) );

    DBG_E ( 0 );

    return;
}

/*
 * ===  FUNCTION  ===================================================
 *         Name:  shm_get_broadcast_bigmsg
 *  Description: Function which gets a broadcast bigmsg msg
 *
 *  __rank : rank of the process where getting the msg
 * __msg_content : pointer to the variable where writing the msg
 * __com_list : pointer to the communicator
 * __size : size of the msg
 * __slot_type : type of the slot
 * ==================================================================
 */
static inline void *
sctk_shm_get_broadcast_bigmsg (
    const unsigned int __rank,
    void *__msg_content,
    struct sctk_shm_com_list_s *__com_list,
    const size_t __size, const int __slot_type ) {
    struct sctk_shm_bigmsg_queue_s *bigmsg_queue;
    struct sctk_shm_bigmsg_broadcast_slot_s *selected_slot;
    char *msg_tocopy = __msg_content;
    int remain;
    int size_tocopy;
    int current_slot;
    int slot_can_be_erased;

    bigmsg_queue = __com_list->broadcast_bigmsg_queue;

    current_slot =
        bigmsg_queue->next_slots[bigmsg_queue->ptr_msg_first_slot];

    sctk_nodebug ( "BEGIN get_broadcast_bigmsg from slot" );
    do {

        sctk_nodebug ( "BEGIN - Reading bigmsg slot %d", current_slot );
        selected_slot = bigmsg_queue->msg_slot[current_slot];

        /* wait until the big msg is ready to be read */
        while ( selected_slot->is_msg_ready_to_read != 1 ) {
            sctk_thread_yield ();
        }

        sctk_nodebug ( "More than 1 bigmsg slot" );
        size_tocopy = selected_slot->msg_size;

        memcpy ( msg_tocopy, selected_slot->msg_content, size_tocopy );

        /* should the slot be erased? */
        sctk_spinlock_lock ( & ( selected_slot->shm_lock_current_elements ) );
        sctk_nodebug ( "Slot %d : Current : %d - Total : %d - remaining : %d",
                       current_slot, selected_slot->current_elements,
                       selected_slot->total_elements,
                       selected_slot->msg_remaining );
        if ( ( selected_slot->current_elements + 1 ) ==
                selected_slot->total_elements ) {
            slot_can_be_erased = 1;
        } else {
            ++ ( selected_slot->current_elements );
            slot_can_be_erased = 0;
        }
        sctk_spinlock_unlock ( & ( selected_slot->shm_lock_current_elements ) );


        /* update last slot */
        sctk_nodebug ( "GET msg_remaining(%p) : %d",
                       & ( selected_slot->msg_remaining ),
                       selected_slot->msg_remaining );
        remain = selected_slot->msg_remaining;
        msg_tocopy += selected_slot->msg_size;

        /* delete the slot if it should be */
        if ( slot_can_be_erased == 1 ) {

            sctk_nodebug ( "SLOT (%p) %d read and erased !!", selected_slot,
                           current_slot );
            selected_slot->is_msg_ready_to_read = 0;
            sctk_shm_move_first_flag ( bigmsg_queue, 1 );
        }

        sctk_nodebug ( "END - Reading bigmsg slot" );
        current_slot = bigmsg_queue->next_slots[current_slot];
    } while ( remain );

    sctk_nodebug ( "END get_broadcast_bigmsg" );
    return selected_slot;
}



/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_init_next_slots
 *  Description:  init queue's next slots according to the queue's type.
 *                Also fills the next_slot array. This array allows the program
 *                to know which slot is the next one.
 * ==================================================================
 */
static inline void
sctk_shm_init_next_slots ( void *__queue ) {
    int i;
    struct sctk_shm_generic_queue_s *gen_queue = __queue;
    int size = 0;

    switch ( gen_queue->queue_type ) {
    case ( SCTK_SHM_DATA_BIG ) :
        size = SCTK_SHM_BIGMSG_QUEUE_LEN;
        break;

    case ( SCTK_SHM_DATA_FAST ) :
        size = SCTK_SHM_FASTMSG_QUEUE_LEN;
        break;

    case ( SCTK_SHM_REDUCE ) :
        size = SCTK_SHM_BIGMSG_REDUCE_QUEUE_LEN;
        break;

    case ( SCTK_SHM_BROADCAST ) :
        size = SCTK_SHM_BIGMSG_BROADCAST_QUEUE_LEN;
        break;

    case ( SCTK_SHM_RPC_FAST ) :
        size = SCTK_SHM_RPC_QUEUE_LEN;
        break;

    case ( SCTK_SHM_RPC_BIG ) :
        size = SCTK_SHM_BIGMSG_RPC_QUEUE_LEN;
        break;

    case ( SCTK_SHM_RDMA_FAST ) :
        size = SCTK_SHM_RDMA_QUEUE_LEN;
        break;

    default:
        sctk_error ( "Cannot initialize queue %d", gen_queue->queue_type );
        break;
    }

    for ( i = 0; i < size; ++i ) {
        gen_queue->next_slots[i] = ( ( i + 1 ) % size );
    }
    gen_queue->queue_size = size;

}



/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_compute_process_number
 *  Description:  Function which computes the number of processes in a communicator
 *
 *  involved_list : list of tasks involved in the communicator
 *  involved_process : array filled and returned in this function.
 *    Contains all processes' numbers involved in the communicator.
 *  process_match : matching table bewteen tasks' numbers and where they are located, in which process.
 *  nb_tasks : nb of tasks in the communicator
 *  nb_processes : nb of processes in the communicator
 *
 * ==================================================================
 */
static int
sctk_shm_compute_process_number (
    int *involved_list,
    int *involved_process,
    int *process_match,
    int nb_tasks,
    int *nb_processes ) {
    int i;
    int j;
    *nb_processes = 0;
    int task_num;
    int is_found = 0;

    /* reset the content of the array */
    //memset ( involved_process, -1, SCTK_SHM_MAX_NB_LOCAL_PROCESSES );

    for ( i = 0; i < nb_tasks; ++i ) {
        task_num = involved_list[i];
        sctk_nodebug ( "Nb tasks (%d) task (%d) in process %d", i,
                       task_num[i], process_match[i] );


        for ( j = 0; j < *nb_processes; ++j ) {
            sctk_nodebug ( "j %d | Process arr %d == process match %d", j,
                           involved_process[j], process_match[i] );
            if ( involved_process[j] == process_match[i] ) {
                is_found = 1;
            }
        }
        /*  if the process isn't found, we add it */
        if ( !is_found ) {
            sctk_nodebug ( "Inc process. New process %d", process_match[i] );
            involved_process[*nb_processes] = process_match[i];
            ( *nb_processes ) ++;
        }
        is_found = 0;
    }

    return 0;
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_put_broadcast_bigmsg
 *  Description:  Put a broadcast msg into the bigmsg queue
 * ==================================================================
 */
inline void *
sctk_shm_put_dma_bigmsg (
    const int __dest,
    const void *__shm_msg,
    const size_t __size,
    int* ack ) {
    struct sctk_shm_bigmsg_queue_s *bigmsg_queue = NULL;
    struct sctk_shm_bigmsg_broadcast_slot_s *selected_slot = NULL;

    int selected_slot_number;
    const char *msg_tocopy = __shm_msg;
    int current_size = __size;
    int size_tocopy;
    int msg_remaining;
    int first_allocation = 0;

    /* select the correct queue according the msg type */
    bigmsg_queue =
        sctk_shm_mem_struct->shm_ptp_queue[__dest]->dma_bigmsg_queue;
    sctk_assert ( bigmsg_queue != NULL );

    sctk_spinlock_lock ( & ( bigmsg_queue->lock_write ) );

    while ( current_size > 0 ) {

        selected_slot =
            bigmsg_queue->msg_slot[bigmsg_queue->ptr_msg_last_slot];
        selected_slot_number = bigmsg_queue->ptr_msg_last_slot;

        sctk_nodebug ( "BEGIN put_broadcast_bigmsg (size : %d) slot (%d - %p) ",
                       __size, bigmsg_queue->ptr_msg_last_slot,
                       selected_slot );

        sctk_nodebug ( "next (%d) first_slot (%d)",
                       bigmsg_queue->next_slots[selected_slot_number],
                       bigmsg_queue->ptr_msg_first_slot );

        /* keeps the lock until a slot is freed */
        while ( selected_slot_number == bigmsg_queue->ptr_msg_first_slot ) {
            sctk_thread_yield ();
        }

        /* if it remains data to copy */
        if ( current_size > SCTK_SHM_BIGMSG_BROADCAST_MAXLEN ) {
            size_tocopy = SCTK_SHM_BIGMSG_BROADCAST_MAXLEN;
            msg_remaining = 1;
            current_size -= SCTK_SHM_BIGMSG_BROADCAST_MAXLEN;
        } else {		/* if there is no more data to copy */
            size_tocopy = current_size;
            msg_remaining = 0;
            current_size = 0;
        }

        memcpy ( selected_slot->msg_content, msg_tocopy, size_tocopy );

        /* updates variables */
        selected_slot->msg_remaining = msg_remaining;
        sctk_nodebug ( "PUT msg_remaining(%p) : %d",
                       & ( selected_slot->msg_remaining ),
                       selected_slot->msg_remaining );
        selected_slot->msg_size = size_tocopy;
        sctk_nodebug ( "SLOT %d write !!", selected_slot_number );

        /* indicates that the slot is ready to be read */
        selected_slot->is_msg_ready_to_read = 1;

        if ( first_allocation == 0 ) {
            sctk_nodebug ( "First allocation! slot %d", selected_slot_number );
            first_allocation = 1;

            *ack = 1;
        }

        msg_tocopy += size_tocopy;

        /* update slots */
        sctk_shm_move_last_flag ( bigmsg_queue, 1 );
    }

    /* release the spinlock -> other processes can write */
    sctk_spinlock_unlock ( & ( bigmsg_queue->lock_write ) );

    sctk_nodebug ( "END put_broadcast_bigmsg (slot %d)",
                   selected_slot_number );

    return selected_slot;
}

/*
 * ===  FUNCTION  ===================================================
 *         Name:  shm_get_broadcast_bigmsg
 *  Description:
 * ==================================================================
 */
inline void *
sctk_shm_get_dma_bigmsg ( const unsigned int __rank,
                          void *__msg_content,
                          const size_t __size ) {
    struct sctk_shm_bigmsg_queue_s *bigmsg_queue;
    struct sctk_shm_bigmsg_broadcast_slot_s *selected_slot;
    char *msg_tocopy = __msg_content;
    int remain;
    int size_tocopy;
    int current_slot;
    int slot_can_be_erased;

    bigmsg_queue =
        sctk_shm_mem_struct->shm_ptp_queue[sctk_local_process_rank]->dma_bigmsg_queue;

    current_slot =
        bigmsg_queue->next_slots[bigmsg_queue->ptr_msg_first_slot];

    sctk_nodebug ( "BEGIN get_broadcast_bigmsg from slot" );
    do {

        sctk_nodebug ( "BEGIN - Reading bigmsg slot %d", current_slot );
        selected_slot = bigmsg_queue->msg_slot[current_slot];

        /* wait until the big msg is ready to be read */
        while ( selected_slot->is_msg_ready_to_read != 1 ) {
            sctk_thread_yield ();
        }

        sctk_nodebug ( "More than 1 bigmsg slot" );
        size_tocopy = selected_slot->msg_size;

        memcpy ( msg_tocopy, selected_slot->msg_content, size_tocopy );

        /* should the slot be erased? */
        sctk_spinlock_lock ( & ( selected_slot->shm_lock_current_elements ) );
        sctk_nodebug ( "Slot %d : Current : %d - Total : %d - remaining : %d",
                       current_slot, selected_slot->current_elements,
                       selected_slot->total_elements,
                       selected_slot->msg_remaining );
        if ( ( selected_slot->current_elements + 1 ) ==
                selected_slot->total_elements ) {
            slot_can_be_erased = 1;
        } else {
            ++ ( selected_slot->current_elements );
            slot_can_be_erased = 0;
        }
        sctk_spinlock_unlock ( & ( selected_slot->shm_lock_current_elements ) );


        /* update last slot */
        sctk_nodebug ( "GET msg_remaining(%p) : %d",
                       & ( selected_slot->msg_remaining ),
                       selected_slot->msg_remaining );
        remain = selected_slot->msg_remaining;
        msg_tocopy += selected_slot->msg_size;

        /* delete the slot if it should be */
        if ( slot_can_be_erased == 1 ) {

            sctk_nodebug ( "SLOT (%p) %d read and erased !!", selected_slot,
                           current_slot );
            selected_slot->is_msg_ready_to_read = 0;
            sctk_shm_move_first_flag ( bigmsg_queue, 1 );
        }

        sctk_nodebug ( "END - Reading bigmsg slot" );
        current_slot = bigmsg_queue->next_slots[current_slot];
    } while ( remain );

    sctk_nodebug ( "END get_broadcast_bigmsg" );
    return selected_slot;
}




/*
 * ===  FUNCTION  ======================================================================
 *         Name:  sctk_shm_init_reduce_bigmsg_queue
 *  Description:  Create a new reduce bigmsg queue. This function is called when COM_WORLD
 *                is initialized and when a new communicator is created.
 * =====================================================================================
 */
struct sctk_shm_bigmsg_queue_s *
            sctk_shm_init_reduce_bigmsg_queue () {
    struct sctk_shm_bigmsg_queue_s *queue;
    int j;

    /* init collective big messages reduce queue */
    queue = ( struct sctk_shm_bigmsg_queue_s * )
            sctk_shm_malloc ( sizeof ( struct sctk_shm_bigmsg_queue_s ) );

    queue->queue_type = SCTK_SHM_REDUCE;

    sctk_shm_init_next_slots ( queue );

    for ( j = 0; j < SCTK_SHM_BIGMSG_REDUCE_QUEUE_LEN; ++j ) {
        queue->msg_slot[j] =
            sctk_shm_malloc ( sizeof ( struct sctk_shm_bigmsg_reduce_slot_s ) );

        if ( sctk_local_process_rank == 0 ) {
            struct sctk_shm_bigmsg_reduce_slot_s *tmp = queue->msg_slot[j];
            sctk_assert ( tmp );

            tmp->slot_number = j;
        }
    }

    /* init queues variables */
    queue->lock_write = 0;
    queue->lock_read = 0;
    queue->ptr_msg_first_slot = 0;
    queue->ptr_msg_last_slot = 1;

    sctk_nodebug ( " queue : %p first : %d - last %d - next %d", queue,
                   queue->ptr_msg_first_slot, queue->ptr_msg_last_slot,
                   queue->next_slots[queue->ptr_msg_first_slot] );

    return queue;
}


/*
 * ===  FUNCTION  ======================================================================
 *         Name:  sctk_shm_init_broadcast_bigmsg_queue
 *  Description:  Create a new broadcast bigmsg queue. This function is called when COM_WORLD
 *                is initialized and when a new communicator is created.
 * =====================================================================================
 */
struct sctk_shm_bigmsg_queue_s *
            sctk_shm_init_broadcast_bigmsg_queue () {
    struct sctk_shm_bigmsg_queue_s *queue;
    int j;

    /* init collective big messages collective queues */
    queue =
        ( struct sctk_shm_bigmsg_queue_s * )
        sctk_shm_malloc ( sizeof ( struct sctk_shm_bigmsg_queue_s ) );

    queue->queue_type = SCTK_SHM_BROADCAST;

    sctk_shm_init_next_slots ( queue );

    for ( j = 0; j < SCTK_SHM_BIGMSG_BROADCAST_QUEUE_LEN; ++j ) {
        queue->msg_slot[j] =
            sctk_shm_malloc ( sizeof ( struct sctk_shm_bigmsg_broadcast_slot_s ) );

        if ( sctk_local_process_rank == 0 ) {
            struct sctk_shm_bigmsg_broadcast_slot_s *tmp = queue->msg_slot[j];
            sctk_assert ( tmp );

            tmp->is_msg_ready_to_read = 0;
            tmp->slot_number = j;
        }
    }

    /* init broadcast queue variables */
    queue->lock_write = 0;
    queue->lock_read = 0;
    queue->ptr_msg_first_slot = 0;
    queue->ptr_msg_last_slot = 1;

    return queue;
}

/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_sctk_shm_init_mem_struct
 *  Description:  initialize a shm mem struct and return a poiter to
 *                this structure. Each structure is mapped into the
 *                shared memory thanks to the "sctk_shm_malloc" function.
 * ==================================================================
 */
struct sctk_shm_mem_struct_s *
            sctk_shm_init_mem_struct ( void *__ptr_shm_base, void* __malloc_base ) {
    int i, j;
    /* strucuture with infos about the shared memory*/
    struct mmap_infos_s* mem_init;

    ptr_shm_current = __ptr_shm_base;
    sctk_assert ( __ptr_shm_base != NULL );

    /* check sizes.
     * WARNING : if the size of a queue is over SCTK_SHM_MAX_SLOT_*, the program
     * will not run. SCTK_SHM_MAX_SLOT_* must be greater or equal to the max number
     * of slots in each queue */
    if ( ( SCTK_SHM_BIGMSG_QUEUE_LEN > SCTK_SHM_MAX_SLOTS_BIGMSG_QUEUE ) |
            ( SCTK_SHM_BIGMSG_BROADCAST_QUEUE_LEN > SCTK_SHM_MAX_SLOTS_BIGMSG_QUEUE ) |
            ( SCTK_SHM_BIGMSG_REDUCE_QUEUE_LEN > SCTK_SHM_MAX_SLOTS_BIGMSG_QUEUE ) |
            ( SCTK_SHM_BIGMSG_QUEUE_LEN > SCTK_SHM_MAX_SLOTS_BIGMSG_QUEUE ) ) {
        sctk_error
        ( "Please increase the value of SCTK_SHM_MAX_SLOTS_BIGMSG_QUEUE" );
    }

    if ( ( SCTK_SHM_FASTMSG_QUEUE_LEN > SCTK_SHM_MAX_SLOTS_FASTMSG_QUEUE ) |
            ( SCTK_SHM_FASTMSG_BROADCAST_QUEUE_LEN > SCTK_SHM_MAX_SLOTS_FASTMSG_QUEUE ) ) {
        sctk_error
        ( "Please increase the value of SCTK_SHM_MAX_SLOTS_BIGMSG_QUEUE" );
    }
    /* end check sizes */

    mem_init = sctk_shm_malloc ( sizeof ( struct mmap_infos_s ) );
    /* allocate the shm for the main structure */
    sctk_shm_mem_struct = ( struct sctk_shm_mem_struct_s * )
                          sctk_shm_malloc ( sizeof ( struct sctk_shm_mem_struct_s ) );
    sctk_assert ( sctk_shm_mem_struct != NULL );

    sctk_nodebug ( "RANK %d - Number of processes : %d", __process_rank, size );

    /* loop on the number of processes */
    for ( i = 0; i < sctk_local_process_number; ++i ) {
        /*-----------------------------------------------------------
         *  SHM - initialize ptp queues
         *----------------------------------------------------------*/
        sctk_shm_mem_struct->shm_ptp_queue[i] =
            ( struct sctk_shm_ptp_queue_s * )
            sctk_shm_malloc ( sizeof ( struct sctk_shm_ptp_queue_s ) );
        sctk_assert ( sctk_shm_mem_struct->shm_ptp_queue[i] != NULL );

        /* init big messages queue */
        sctk_shm_mem_struct->shm_ptp_queue[i]->shm_bigmsg_queue =
            ( struct sctk_shm_bigmsg_queue_s * )
            sctk_shm_malloc ( sizeof ( struct sctk_shm_bigmsg_queue_s ) );

        sctk_shm_mem_struct->shm_ptp_queue[i]->shm_bigmsg_queue->queue_type =
            SCTK_SHM_DATA_BIG;

        sctk_shm_init_next_slots ( sctk_shm_mem_struct->
                                   shm_ptp_queue[i]->shm_bigmsg_queue );

        for ( j = 0; j < SCTK_SHM_BIGMSG_QUEUE_LEN; ++j ) {
            sctk_shm_mem_struct->shm_ptp_queue[i]->
            shm_bigmsg_queue->msg_slot[j] =
                sctk_shm_malloc ( SCTK_SHM_BIGMSG_DATA_MAXLEN );
        }

        /* init fast messages queue */
        /* MSG queue */
        sctk_shm_mem_struct->
        shm_ptp_queue[i]->shm_fastmsg_queue[SCTK_SHM_DATA_FAST] =
            ( struct sctk_shm_fastmsg_queue_s * )
            sctk_shm_malloc ( sizeof ( struct sctk_shm_fastmsg_queue_s ) );

        sctk_shm_mem_struct->shm_ptp_queue[i]->
        shm_fastmsg_queue[SCTK_SHM_DATA_FAST]->queue_type =
            SCTK_SHM_DATA_FAST;

        sctk_shm_init_next_slots ( sctk_shm_mem_struct->
                                   shm_ptp_queue[i]->shm_fastmsg_queue
                                   [SCTK_SHM_DATA_FAST] );

        for ( j = 0; j < SCTK_SHM_FASTMSG_QUEUE_LEN; ++j ) {
            sctk_shm_mem_struct->
            shm_ptp_queue[i]->shm_fastmsg_queue[SCTK_SHM_DATA_FAST]->
            msg_slot[j] =
                sctk_shm_malloc ( sizeof ( struct sctk_shm_fastmsg_slot_s ) );

            struct sctk_shm_fastmsg_slot_s *tmp =
                            sctk_shm_mem_struct->
                            shm_ptp_queue[i]->shm_fastmsg_queue[SCTK_SHM_DATA_FAST]->
                            msg_slot[j];
            sctk_assert ( tmp );

            tmp->is_msg_ready_to_read = 0;
            tmp->slot_number = j;
        }


        /* init fast messages queue */
        /* RPC queue */
        sctk_shm_mem_struct->shm_ptp_queue[i]->
        shm_fastmsg_queue[SCTK_SHM_RPC_FAST] =
            ( struct sctk_shm_fastmsg_queue_s * )
            sctk_shm_malloc ( sizeof ( struct sctk_shm_fastmsg_queue_s ) );

        sctk_shm_mem_struct->shm_ptp_queue[i]->
        shm_fastmsg_queue[SCTK_SHM_RPC_FAST]->queue_type = SCTK_SHM_RPC_FAST;

        sctk_shm_init_next_slots ( sctk_shm_mem_struct->
                                   shm_ptp_queue[i]->shm_fastmsg_queue
                                   [SCTK_SHM_RPC_FAST] );

        sctk_shm_mem_struct->shm_ptp_queue[i]->
        shm_fastmsg_queue[SCTK_SHM_RPC_FAST]->flag_new_msg_received = 0;
        sctk_shm_mem_struct->shm_ptp_queue[i]->
        shm_fastmsg_queue[SCTK_SHM_RPC_FAST]->lock_count_msg_received = 0;
        sctk_shm_mem_struct->shm_ptp_queue[i]->
        shm_fastmsg_queue[SCTK_SHM_RPC_FAST]->count_msg_received = 0;

        for ( j = 0; j < SCTK_SHM_RPC_QUEUE_LEN; ++j ) {
            sctk_shm_mem_struct->
            shm_ptp_queue[i]->shm_fastmsg_queue[SCTK_SHM_RPC_FAST]->
            msg_slot[j] =
                sctk_shm_malloc ( sizeof ( struct sctk_shm_rpc_slot_s ) );

            struct sctk_shm_rpc_slot_s *tmp =
                            sctk_shm_mem_struct->
                            shm_ptp_queue[i]->shm_fastmsg_queue[SCTK_SHM_RPC_FAST]->
                            msg_slot[j];
            sctk_assert ( tmp );

            tmp->slot_number = j;
        }

        /* RDMA fastmsg queue */
        sctk_shm_mem_struct->
        shm_ptp_queue[i]->shm_fastmsg_queue[SCTK_SHM_RDMA_FAST] =
            ( struct sctk_shm_fastmsg_queue_s * )
            sctk_shm_malloc ( sizeof ( struct sctk_shm_fastmsg_queue_s ) );

        sctk_shm_mem_struct->shm_ptp_queue[i]->
        shm_fastmsg_queue[SCTK_SHM_RDMA_FAST]->queue_type =
            SCTK_SHM_RDMA_FAST;

        sctk_shm_init_next_slots ( sctk_shm_mem_struct->
                                   shm_ptp_queue[i]->shm_fastmsg_queue
                                   [SCTK_SHM_RDMA_FAST] );

        sctk_shm_mem_struct->
        shm_ptp_queue[i]->shm_fastmsg_queue[SCTK_SHM_RDMA_FAST]->
        flag_new_msg_received = 0;
        sctk_shm_mem_struct->
        shm_ptp_queue[i]->shm_fastmsg_queue[SCTK_SHM_RDMA_FAST]->
        lock_count_msg_received = 0;
        sctk_shm_mem_struct->
        shm_ptp_queue[i]->shm_fastmsg_queue[SCTK_SHM_RDMA_FAST]->
        count_msg_received = 0;

        for ( j = 0; j < SCTK_SHM_RDMA_QUEUE_LEN; ++j ) {
            sctk_shm_mem_struct->
            shm_ptp_queue[i]->shm_fastmsg_queue[SCTK_SHM_RDMA_FAST]->
            msg_slot[j] =
                sctk_shm_malloc ( sizeof ( struct sctk_shm_fastmsg_dma_slot_s ) );

            struct sctk_shm_fastmsg_dma_slot_s *tmp =
                            sctk_shm_mem_struct->
                            shm_ptp_queue[i]->shm_fastmsg_queue[SCTK_SHM_RDMA_FAST]->
                            msg_slot[j];
            sctk_assert ( tmp );

            tmp->slot_number = j;
        }

        /* RDMA bigmsg queue */
        sctk_shm_mem_struct->shm_ptp_queue[i]->dma_bigmsg_queue =
            ( struct sctk_shm_bigmsg_queue_s * )
            sctk_shm_malloc ( sizeof ( struct sctk_shm_bigmsg_queue_s ) );

        sctk_shm_mem_struct->shm_ptp_queue[i]->dma_bigmsg_queue->queue_type =
            SCTK_SHM_RPC_BIG;

        sctk_shm_init_next_slots ( sctk_shm_mem_struct->
                                   shm_ptp_queue[i]->dma_bigmsg_queue );

        for ( j = 0; j < SCTK_SHM_BIGMSG_RPC_QUEUE_LEN; ++j ) {
            sctk_shm_mem_struct->shm_ptp_queue[i]->
            dma_bigmsg_queue->msg_slot[j] =
                sctk_shm_malloc ( sizeof ( struct sctk_shm_bigmsg_dma_slot_s ) );
        }


        /*-----------------------------------------------------------
         *  SHM - Set up the initial value of each variable
         *----------------------------------------------------------*/
        sctk_nodebug ( "Init queues for process %d", i );
        /* fast messages queue */
        sctk_shm_mem_struct->
        shm_ptp_queue[i]->shm_fastmsg_queue[SCTK_SHM_DATA_FAST]->
        ptr_msg_first_slot = 0;

        sctk_shm_mem_struct->
        shm_ptp_queue[i]->shm_fastmsg_queue[SCTK_SHM_DATA_FAST]->
        ptr_msg_last_slot = 1;

        sctk_shm_mem_struct->
        shm_ptp_queue[i]->shm_fastmsg_queue[SCTK_SHM_RPC_FAST]->
        ptr_msg_first_slot = 0;

        sctk_shm_mem_struct->
        shm_ptp_queue[i]->shm_fastmsg_queue[SCTK_SHM_RPC_FAST]->
        ptr_msg_last_slot = 1;

        sctk_shm_mem_struct->
        shm_ptp_queue[i]->shm_fastmsg_queue[SCTK_SHM_RDMA_FAST]->
        ptr_msg_first_slot = 0;

        sctk_shm_mem_struct->
        shm_ptp_queue[i]->shm_fastmsg_queue[SCTK_SHM_RDMA_FAST]->
        ptr_msg_last_slot = 1;

        /* big messages queue */
        sctk_shm_mem_struct->shm_ptp_queue[i]->
        shm_bigmsg_queue->ptr_msg_first_slot = 0;

        sctk_shm_mem_struct->shm_ptp_queue[i]->
        shm_bigmsg_queue->ptr_msg_last_slot = 1;

        /* initialize spinlocks values */
        sctk_shm_mem_struct->
        shm_ptp_queue[i]->shm_fastmsg_queue[SCTK_SHM_DATA_FAST]->
        lock_write = 0;

        sctk_shm_mem_struct->
        shm_ptp_queue[i]->shm_fastmsg_queue[SCTK_SHM_DATA_FAST]->
        lock_read = 0;

        sctk_shm_mem_struct->
        shm_ptp_queue[i]->shm_fastmsg_queue[SCTK_SHM_RPC_FAST]->
        lock_write = 0;

        sctk_shm_mem_struct->
        shm_ptp_queue[i]->shm_fastmsg_queue[SCTK_SHM_RPC_FAST]->
        lock_read = 0;

        sctk_shm_mem_struct->
        shm_ptp_queue[i]->shm_fastmsg_queue[SCTK_SHM_RDMA_FAST]->
        lock_write = 0;

        sctk_shm_mem_struct->
        shm_ptp_queue[i]->shm_fastmsg_queue[SCTK_SHM_RDMA_FAST]->
        lock_read = 0;

        sctk_shm_mem_struct->shm_ptp_queue[i]->
        shm_bigmsg_queue->lock_write = 0;

        sctk_shm_mem_struct->shm_ptp_queue[i]->shm_bigmsg_queue->lock_read = 0;

        /* communicators lists */
        sctk_shm_mem_struct->com_list_lock = 0;

        /* translation table */
        sctk_shm_mem_struct->translation_table_lock = 0;
        sctk_shm_mem_struct->ptr_translation_table = 0;

        memset ( sctk_shm_mem_struct->shm_local_to_global_translation_table, -1, SCTK_SHM_MAX_NB_LOCAL_PROCESSES );
        memset ( sctk_shm_mem_struct->shm_global_to_local_translation_table, -1, SCTK_SHM_MAX_NB_TOTAL_PROCESSES );

        sctk_spinlock_unlock ( & ( sctk_shm_mem_struct->translation_table_lock ) );
    }
    mem_init->mem_base = sctk_shm_mem_struct;
    mem_init->malloc_ptr = ptr_shm_current;
    mem_init->malloc_base = __malloc_base;

    return sctk_shm_mem_struct;
}



/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_get_allocated_mem_size
 *  Description:  returns the size allocated on the SHM
 * ==================================================================
 */
size_t sctk_shm_get_allocated_mem_size () {
    return sctk_shm_size_allocated;
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_get_memstruct_base
 *  Description:  returns the size allocated in the SHM
 * ==================================================================
 */
struct sctk_shm_mem_struct_s * sctk_shm_get_memstruct_base () {
    return sctk_shm_mem_struct;
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_init_coolective_structs
 *  Description:  initializes collectives structures
 * ==================================================================
 */
void
sctk_shm_init_collective_structs ( int init ) {
    int i;

    if (init == sctk_local_process_rank) {
      DBG_S ( 0 );
      /* set to null all other pointers to collective comms */
      for (i=0; i<SCTK_SHM_MAX_COMMUNICATORS;++i)
      {
        sctk_shm_mem_struct->sctk_shm_com_list[i].com_id = -1;
        sctk_nodebug("Comm %d", sctk_shm_mem_struct->sctk_shm_com_list[i].com_id);
      }
      DBG_E ( 0 );
    }
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_init_new_com_list
 *  Description:  init a new comm list from an index
 * ==================================================================
 */
static void
sctk_shm_init_new_com_list ( const sctk_internal_communicator_t * __com,
    const int com_index, const int nb_involved, const int *task_list,
    struct sctk_shm_com_list_s *com_list ) {
  int i;
  int tmp_task;
  DBG_S ( 0 );

  for ( i = 0; i < nb_involved; ++i ) {
    com_list->shm_barrier_slot.rank[i] = 0;

    /* initialize reduce slot */
    com_list->shm_reduce_slot.shm_is_msg_ready[i] = -1;

    /* init involved list */
    com_list->involved_task_list[i] = task_list[i];

    /* compute the global rank of a local task */
    tmp_task = sctk_translate_to_global_rank_local ( task_list[i],
        __com->origin_communicator );

    /*  try to get where the task is located, in which process */
    tmp_task = sctk_get_ptp_process_localisation ( tmp_task );
    com_list->match_task_process[i] = tmp_task;

    sctk_nodebug ( "Task %d found in process %d", task_list[i], tmp_task );
  }

  /* Number of processes which take part of the communicator  */
  sctk_nodebug ( "nb involved : %d", nb_involved );
  com_list->nb_task_involved = nb_involved;

  /* compute the number of processes in a communicator */
  sctk_shm_compute_process_number ( com_list->involved_task_list,
      com_list->involved_process_list,
      com_list->match_task_process,
      nb_involved,
      & ( com_list->nb_process_involved ) );
  sctk_nodebug ( "Number of processes in comm : %d",
      com_list->nb_process_involved );
  com_list->com_id = __com->communicator_number;

  /* Barrier */
  com_list->shm_barrier_slot.total = 0;
  com_list->shm_barrier_slot.spin_total = 0;

  /* initialize broadcast slot */
  com_list->shm_broadcast_slot.shm_is_msg_ready = 0;

  /* reduce slot */
  com_list->shm_reduce_slot.shm_is_broadcast_ready = 0;
  com_list->shm_reduce_slot.lock = 0;

  DBG_E ( 0 );
  return;
}



/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_init_world_com
 *  Description:  Init the comm_world communicator. This comm integrates
 *  all processes which take part of the communication.
 * ==================================================================
 */
struct sctk_shm_com_list_s * sctk_shm_init_world_com ( int init ) {
  int i;
  struct sctk_shm_com_list_s *sctk_shm_com_list;

  if ( init == sctk_local_process_rank ) {

    sctk_nodebug ( "Initializing MPC_COMM_WORLD" );
    /* create world com */
    sctk_shm_com_list = &sctk_shm_mem_struct->sctk_shm_com_list[0];
    sctk_assert ( sctk_shm_com_list );

    memset ( sctk_shm_com_list->shm_barrier_slot.rank, 0, SCTK_SHM_MAX_NB_TOTAL_PROCESSES );
    memset ( sctk_shm_com_list->shm_barrier_slot.done, 0, SCTK_SHM_MAX_NB_TOTAL_PROCESSES );
    /* initialize reduce slot */
    memset ( sctk_shm_com_list->shm_reduce_slot.shm_is_msg_ready, -1, SCTK_SHM_MAX_NB_TOTAL_PROCESSES );

    /* init involved list */
    for ( i = 0; i < sctk_get_total_tasks_number(); ++i ) {
      sctk_shm_com_list->involved_task_list[i] = i;
    }

    /* Number of processes which take part of the communicator.
       Bugfix when there is more processes than tasks */
    if (sctk_get_total_tasks_number() < sctk_local_process_number)
      sctk_shm_com_list->nb_process_involved = sctk_get_total_tasks_number();
    else
      sctk_shm_com_list->nb_process_involved = sctk_local_process_number;
    /* fill involved process_list table */
    for ( i = 0; i < sctk_shm_com_list->nb_process_involved; ++i ) {
      sctk_shm_com_list->involved_process_list[i] = i;
    }

    sctk_nodebug ( "Number of processes involved : %d", sctk_shm_com_list->nb_process_involved );
    sctk_nodebug ( "Number of threads involved : %d",
        sctk_get_total_tasks_number () );
    sctk_shm_com_list->com_id = MPC_COMM_WORLD;

    /* Barrier */
    sctk_shm_com_list->shm_barrier_slot.total = 0;
    sctk_shm_com_list->shm_barrier_slot.spin_total = 0;
    /* initialize broadcast slot */
    sctk_shm_com_list->shm_broadcast_slot.shm_is_msg_ready = 0;
    /* reduce slot */
    sctk_shm_com_list->shm_reduce_slot.shm_is_broadcast_ready = 0;
    sctk_shm_com_list->shm_reduce_slot.lock = 0;
    /* init lock */
    sctk_shm_mem_struct->com_list_lock = 0;

    sctk_shm_com_list->reduce_bigmsg_queue = sctk_shm_init_reduce_bigmsg_queue ();
    sctk_shm_com_list->broadcast_bigmsg_queue = sctk_shm_init_broadcast_bigmsg_queue ();

    sctk_nodebug ( "POINTER 1 %p", sctk_shm_com_list->reduce_bigmsg_queue );
    sctk_nodebug ( "MPC_COMM_WORLD initialized with id %d", MPC_COMM_WORLD );
    return sctk_shm_com_list;
  }
  return NULL;
}


/*
 * ===  FUNCTION  ======================================================================
 *         Name:  sctk_shm_init_new_com
 *  Description:  function which initializes a new communicator. This function is called
 *                when a new communicator is created in MPC (hook).
 * =====================================================================================
 */
void
sctk_shm_init_new_com ( const sctk_internal_communicator_t * __com, const int nb_involved,
    const int *task_list ) {
  int com_index = 0;
  int found_index = -1;

  sctk_spinlock_lock ( & ( sctk_shm_mem_struct->com_list_lock ) );

  sctk_nodebug ( "Initializing new comm (%d) all tasks (%d) nb_involved (%d)",
      __com->communicator_number, sctk_get_total_tasks_number (),
      nb_involved );

  while ( com_index < SCTK_SHM_MAX_COMMUNICATORS ) {

    sctk_nodebug("%d <-> %d" ,sctk_shm_mem_struct->sctk_shm_com_list[com_index].com_id, -1);

    /*  the current communicator has already been created */
    if ( sctk_shm_mem_struct->sctk_shm_com_list[com_index].com_id != -1 ) {
      sctk_nodebug ( "com id %d __com id %d",
          sctk_shm_mem_struct->sctk_shm_com_list[com_index].com_id,
          __com->communicator_number);

      /* if the com id matches */
      if ( sctk_shm_mem_struct->sctk_shm_com_list[com_index].com_id ==
          __com->communicator_number ) {
        found_index = com_index;

        sctk_nodebug("Leave...");
        sctk_spinlock_unlock ( & ( sctk_shm_mem_struct->com_list_lock ) );
        return;
      }
    }
    else
    {
      sctk_nodebug("Index : %d", com_index);
      if (found_index == -1)
        found_index = com_index;
    }
    ++com_index;
  }
  /* if all communicators entries are filled.
   * The user must increase the variable SCTK_SHM_MAX_COMMUNICATORS */
  assume(found_index != -1);

  sctk_nodebug
    ( "Create a new com entry with index %d ((com id : %d - com : %p - involved : %d - found index %d)",
      com_index, __com->communicator_number, __com, nb_involved, found_index );

#if SCTK_HYBRID_DEBUG == 1
  if ( sctk_local_process_rank == 0 ) {
    fprintf ( stderr,
        "\n# ------------------NODE %3d | NEW COMM CREATED----------------------------\n"
        "# - Communicator id                 : %d\n"
        "# - Number of tasks involved        : %d\n"
        "# - Size of bigmsg broadcast queue  : %ld Mo\n"
        "# - Size of bigmsg reduce queue     : %ld Mo\n"
        "# - Actual size allocated           : %ld Mo\n"
        "# -------------------------------------------------------------------------\n\n",
        sctk_node_rank,__com->communicator_number, nb_involved, ( sizeof ( struct sctk_shm_bigmsg_broadcast_slot_s ) * SCTK_SHM_BIGMSG_BROADCAST_QUEUE_LEN ) / ( 1024 * 1024 ), ( sizeof ( struct sctk_shm_bigmsg_reduce_slot_s ) * SCTK_SHM_BIGMSG_REDUCE_QUEUE_LEN ) / ( 1024 * 1024 ), sctk_shm_get_allocated_mem_size () / ( 1024 * 1024 ) );
  }
#endif

  sctk_shm_init_new_com_list ( __com, found_index, nb_involved,
      task_list,
      &sctk_shm_mem_struct->
      sctk_shm_com_list[found_index] );


  sctk_shm_mem_struct->sctk_shm_com_list[found_index].
    reduce_bigmsg_queue = sctk_shm_init_reduce_bigmsg_queue ();
  sctk_shm_mem_struct->sctk_shm_com_list[found_index].
    broadcast_bigmsg_queue = sctk_shm_init_broadcast_bigmsg_queue ();

  sctk_nodebug ( "Queue created : %p",
      sctk_shm_mem_struct->sctk_shm_com_list[found_index].
      reduce_bigmsg_queue );

  sctk_spinlock_unlock ( & ( sctk_shm_mem_struct->com_list_lock ) );
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_free_com
 *  Description: Function which frees a communicator according to this
 * communicator id.
 * ==================================================================
 */
void
sctk_shm_free_com ( const int com_id ) {
  int com_index = 0;

  sctk_nodebug ( "Freeing communicator %d", com_id );

  sctk_spinlock_lock ( & ( sctk_shm_mem_struct->com_list_lock ) );
  while ( com_index < SCTK_SHM_MAX_COMMUNICATORS ) {
    /*  the current communicator has already been created */
    if ( sctk_shm_mem_struct->sctk_shm_com_list[com_index].com_id != -1 ) {
      /* if the com id matches */
      if ( sctk_shm_mem_struct->sctk_shm_com_list[com_index].com_id ==
          com_id ) {

#if SCTK_HYBRID_DEBUG == 1
        if ( sctk_local_process_rank == 0 ) {
          fprintf ( stderr,
              "\n# ------------------NODE %3d | NEW COMM FREED----------------------------\n"
              "# - Communicator id                 : %d\n"
              "# -------------------------------------------------------------------------\n\n",
              sctk_node_rank,
              sctk_shm_mem_struct->sctk_shm_com_list[com_index].com_id);
        }
#endif
        sctk_shm_mem_struct->sctk_shm_com_list[com_index].com_id = -1;
        sctk_nodebug ( "Communicator %d freed", com_id );
        sctk_spinlock_unlock ( & ( sctk_shm_mem_struct->com_list_lock ) );
        return;
      }
    }
    ++com_index;
  }
  sctk_spinlock_unlock ( & ( sctk_shm_mem_struct->com_list_lock ) );
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_select_right_com_list
 *  Description:  select the right com list according to the communicator
 * ==================================================================
 */
struct sctk_shm_com_list_s *
sctk_shm_select_right_com_list ( const sctk_collective_communications_t * __com ) {
  int com_index = 0;

  sctk_nodebug ( "BEGIN sctk_shm_select_right_com_list  (com id : %d)",
      __com->id );

  while ( com_index < SCTK_SHM_MAX_COMMUNICATORS  ) {

    sctk_nodebug ( "Index (%d) Comm id found (%d) comm looking for (%d)",
        com_index,
        sctk_shm_mem_struct->sctk_shm_com_list[com_index].com_id,
        __com->id );

    if ( sctk_shm_mem_struct->sctk_shm_com_list[com_index].com_id ==
        __com->id ) {
      sctk_nodebug ( "END sctk_shm_select_right_com_list");

      return &sctk_shm_mem_struct->sctk_shm_com_list[com_index];
    }
    ++com_index;
  }

  /* if the communicator isn't found, return NULL  */
  return NULL;
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_put_ptp_msg
 *  Description:  write a new PTP msg
 * ==================================================================
 */
inline void
sctk_shm_put_ptp_msg ( const unsigned int __dest,
    void *__payload,
    const  size_t __size_header, const size_t __size_payload ) {

  /* if the message is bigger than a fastmsg slot */
  if ( ( __size_header + __size_payload +
        sizeof ( struct sctk_shm_bigmsg_header_s ) ) > SCTK_SHM_FASTMSG_DATA_MAXLEN ) {

    sctk_nodebug ( "Trying to send a ptp bigmsg in bigmsg queue" );

    sctk_shm_put_ptp_bigmsg ( __dest,
        __payload, __size_header, __size_payload );

    sctk_nodebug ( "Ptp bigmsg sent in queue" );
  } else {
    sctk_nodebug ( "Trying to send a ptp fastmsg in queue" );

    while ( ( sctk_shm_put_ptp_fastmsg ( __dest,
            __payload,
            __size_header,
            __size_payload,
            SCTK_SHM_SLOT_TYPE_PTP_MESSAGE |
            SCTK_SHM_SLOT_TYPE_FASTMSG ) ) ==
        NULL ) {
      sctk_thread_yield ();
    }

    sctk_nodebug ( "Ptp fastmsg sent in queue" );
  }

  return;
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_put_fastmsg
 *  Description:  This function aims to put :
 *                   - fast ptp msg
 *                   - fast rpc msg
 *                   - rdma msg
 *       Output:  Pointer to the slot allocated
 * ==================================================================
 */
inline void *
sctk_shm_put_fastmsg ( const unsigned int __dest,
    const void *__shm_msg,
    const size_t __size, const int ptr_to_slot_queue_type ) {
  struct sctk_shm_fastmsg_queue_s *fastmsg_queue;
  struct sctk_shm_fastmsg_slot_s *selected_slot;
  struct sctk_shm_rpc_slot_s *rpc_slot;
  int selected_slot_number;

  /* select the correct queue according the msg type */
  if ( ( ptr_to_slot_queue_type & SCTK_SHM_SLOT_TYPE_PTP_RPC ) ==
      SCTK_SHM_SLOT_TYPE_PTP_RPC ) {
    fastmsg_queue =
      sctk_shm_mem_struct->
      shm_ptp_queue[__dest]->shm_fastmsg_queue[SCTK_SHM_RPC_FAST];
  } else if ( ( ptr_to_slot_queue_type & SCTK_SHM_SLOT_TYPE_PTP_RDMA_READ ) ==
      SCTK_SHM_SLOT_TYPE_PTP_RDMA_READ ) {
    fastmsg_queue =
      sctk_shm_mem_struct->
      shm_ptp_queue[__dest]->shm_fastmsg_queue[SCTK_SHM_RDMA_FAST];
  } else if ( ( ptr_to_slot_queue_type & SCTK_SHM_SLOT_TYPE_PTP_RDMA_WRITE ) ==
      SCTK_SHM_SLOT_TYPE_PTP_RDMA_WRITE ) {
    fastmsg_queue =
      sctk_shm_mem_struct->
      shm_ptp_queue[__dest]->shm_fastmsg_queue[SCTK_SHM_RDMA_FAST];
  } else {
    fastmsg_queue =
      sctk_shm_mem_struct->
      shm_ptp_queue[__dest]->shm_fastmsg_queue[SCTK_SHM_DATA_FAST];
  }
  sctk_assert ( fastmsg_queue != NULL );

  /* spinlock the queue for writing */
  sctk_spinlock_lock ( & ( fastmsg_queue->lock_write ) );

  /* return if the queue is full */
  if ( fastmsg_queue->ptr_msg_first_slot ==
      fastmsg_queue->ptr_msg_last_slot ) {
    sctk_spinlock_unlock ( & ( fastmsg_queue->lock_write ) );
    return NULL;
  }

  selected_slot_number = fastmsg_queue->ptr_msg_last_slot;
  selected_slot = fastmsg_queue->msg_slot[selected_slot_number];
  selected_slot->msg_size = __size;

  switch ( ptr_to_slot_queue_type ) {

    /**************************************************
     *                  RPC MESSAGE
     *************************************************/
    case ( SCTK_SHM_SLOT_TYPE_PTP_RPC | SCTK_SHM_SLOT_TYPE_FASTMSG ) :
      rpc_slot = ( struct sctk_shm_rpc_slot_s * ) selected_slot;

      /* wait until the slot is freed */
      while ( rpc_slot->is_rpc_acked != 0 ) {
        sctk_thread_yield ();
      }

      sctk_nodebug ( "RPC msg is being added to the queue..." );

      /* this is a fast mesage */
      rpc_slot->shm_msg_type =
        ( SCTK_SHM_SLOT_TYPE_PTP_RPC | SCTK_SHM_SLOT_TYPE_FASTMSG );

      memcpy ( rpc_slot->msg_content, __shm_msg, __size );

      /* the msg is ready to read */
      /* moves last slot pointer */
      sctk_shm_move_last_flag ( fastmsg_queue, 1 );

      /* release writer spinlock */
      sctk_spinlock_unlock ( & ( fastmsg_queue->lock_write ) );

      sctk_nodebug ( "Fastmsg added to the queue..." );

      break;

      /**************************************************
       *                  RDMA MESSAGE
       *************************************************/
    case ( SCTK_SHM_SLOT_TYPE_PTP_RDMA_READ ) :
    case ( SCTK_SHM_SLOT_TYPE_PTP_RDMA_WRITE ) :

      rpc_slot = ( struct sctk_shm_rpc_slot_s * ) selected_slot;

      sctk_nodebug
        ( "RDMA msg is being added to the queue... (%p) (%d) Size : %d",
          selected_slot, ptr_to_slot_queue_type, __size );

      /* wait until the slot is freed */
      while ( rpc_slot->is_rpc_acked != 0 ) {
        sctk_thread_yield ();
      }

      /* wait until the message is available */
      memcpy ( rpc_slot->msg_content, __shm_msg, __size );

      rpc_slot->shm_msg_type = ptr_to_slot_queue_type;

      /* the msg is ready to read */
      /* moves last slot pointer */
      sctk_shm_move_last_flag ( fastmsg_queue, 1 );

      /* release writer spinlock */
      sctk_spinlock_unlock ( & ( fastmsg_queue->lock_write ) );

      sctk_nodebug ( "Fastmsg added to the queue..." );

      break;

  }

  /* set up the flag which shows that a new message has been received */
  sctk_spinlock_lock ( & ( fastmsg_queue->lock_count_msg_received ) );
  ++fastmsg_queue->count_msg_received;
  sctk_spinlock_unlock ( & ( fastmsg_queue->lock_count_msg_received ) );

  return selected_slot;
}



/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_get_ptp_fastmsg
 *  Description:  get a ptp fastmsg from a queue
 * ==================================================================
 */
inline void *
sctk_shm_get_ptp_fastmsg ( const unsigned int __rank, const int __mode ) {

  struct sctk_shm_fastmsg_queue_s *fastmsg_queue;
  struct sctk_shm_fastmsg_slot_s *selected_slot;
  size_t size;
  void *ret;

  /* __queue_type shows which queue the program must use */
  fastmsg_queue =
    sctk_shm_mem_struct->
    shm_ptp_queue[__rank]->shm_fastmsg_queue[SCTK_SHM_DATA_FAST];


  sctk_nodebug ( "Pointer to first slot %d - Pointer to last slot %d",
      tmp_fastmsg_queue->
      next_slots[tmp_fastmsg_queue->ptr_msg_first_slot],
      tmp_fastmsg_queue->ptr_msg_last_slot );

  /* return NULL if the queue is empty */
  if ( fastmsg_queue->next_slots[fastmsg_queue->ptr_msg_first_slot] ==
      fastmsg_queue->ptr_msg_last_slot ) {
    return NULL;
  }

  /*select the right slot where read the message */
  selected_slot = fastmsg_queue->
    msg_slot[fastmsg_queue->next_slots
    [fastmsg_queue->ptr_msg_first_slot]];

  /* malloc while the thread is ready to read */
  size = selected_slot->msg_size;
  ret = sctk_malloc ( size );

  /* wait until the message is available */
  while ( selected_slot->is_msg_ready_to_read != 1 ) {
    sctk_thread_yield ();
  }

  switch ( selected_slot->shm_msg_type ) {
    /**************************************************
     *                  PTP DATA MESSAGE
     *************************************************/
    case ( SCTK_SHM_SLOT_TYPE_PTP_MESSAGE | SCTK_SHM_SLOT_TYPE_BIGMSG ) :
      sctk_nodebug ( "Ptp bigmsg received" );
      /* try to get the big message */
      sctk_shm_get_ptp_bigmsg ( __rank, ret );
      break;

    case ( SCTK_SHM_SLOT_TYPE_PTP_MESSAGE | SCTK_SHM_SLOT_TYPE_FASTMSG ) :
      sctk_nodebug ( "Ptp fastmsg received" );
      memcpy ( ret, selected_slot->msg_content, size );
      break;

    default:
      sctk_nodebug ( "ERROR : Type of the message not recognized !" );
      abort ();
      break;
  }

  selected_slot->is_msg_ready_to_read = 0;

  sctk_nodebug ( "Queue moved to the next slot (slot %p destroyed) queue (%p)",
      selected_slot, __ret->queue );

  /* Move the first slot pointer */
  sctk_shm_move_first_flag ( fastmsg_queue, 1 );

  sctk_nodebug ( "First slot (%d) Last slot (%d)",
      fastmsg_queue->ptr_msg_first_slot,
      fastmsg_queue->ptr_msg_last_slot );
  return ret;
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_get_ptp_msg
 *  Description:  This funtion gets a message from the fastmsg queue.
 *
 *
 *  __rank      : rank of the process where getting the message
 *  msg_content : pointer to the space where the outgoing message has to
 *                be copied. If this pointer is null, the programs does a
 *                malloc with the size of the message.
 *  __size      : msg size that we extract. If msg_content is NULL, this
 *                variable ins't read and we use the size of the message in
 *                the slot.
 *  __message_size : Size of the message contained in the slot.
 *  __queue_type : Type of the queue. According to this value, the queue read
 *                is different
 *
 * ==================================================================
 */

inline int
sctk_shm_get_ptp_msg ( const unsigned int __rank,
    void *__msg_content,
    size_t __size,
    size_t * __message_size,
    const int __queue_type, struct sctk_shm_ret_get_ptp_s *__ret ) {
  struct sctk_shm_fastmsg_queue_s *fastmsg_queue;
  struct sctk_shm_rpc_slot_s *rpc_slot;
  struct sctk_shm_fastmsg_slot_s *selected_slot;

  /* __queue_type shows which queue the program must use */
  fastmsg_queue =
    sctk_shm_mem_struct->
    shm_ptp_queue[__rank]->shm_fastmsg_queue[__queue_type];
  sctk_assert ( fastmsg_queue != NULL );

  sctk_nodebug ( "Pointer to first slot %d - Pointer to last slot %d",
      fastmsg_queue->
      next_slots[fastmsg_queue->ptr_msg_first_slot],
      fastmsg_queue->ptr_msg_last_slot );

  /* return -1 if the queue is empty */
  if ( fastmsg_queue->next_slots[fastmsg_queue->ptr_msg_first_slot] ==
      fastmsg_queue->ptr_msg_last_slot )
    return -1;

  /*select the right slot where read the message */
  selected_slot =
    fastmsg_queue->
    msg_slot[fastmsg_queue->next_slots
    [fastmsg_queue->ptr_msg_first_slot]];

  /*  switch on the queue type */
  switch ( __queue_type ) {
    case SCTK_SHM_DATA_FAST:

      /* wait until the message is available */
      while ( selected_slot->is_msg_ready_to_read != 1 ) {
        sctk_thread_yield ();
      }

      /* fill the msg type */
      __ret->msg_type = selected_slot->shm_msg_type;

      switch ( selected_slot->shm_msg_type ) {
        /**************************************************
         *                  PTP DATA MESSAGE
         *************************************************/
        case ( SCTK_SHM_SLOT_TYPE_PTP_MESSAGE | SCTK_SHM_SLOT_TYPE_BIGMSG ) :

          sctk_nodebug ( "Ptp bigmsg received (size : %d)",
              selected_slot->msg_size );

          /* try to get the big message */
          sctk_shm_get_ptp_bigmsg ( __rank, __msg_content );

          break;

        case ( SCTK_SHM_SLOT_TYPE_PTP_MESSAGE | SCTK_SHM_SLOT_TYPE_FASTMSG ) :
          sctk_nodebug ( "Ptp fastmsg received" );

          sctk_nodebug ( "Malloc executed (size : %d)",
              selected_slot->msg_size );

          __size = selected_slot->msg_size;
          * ( __ret->selected_slot_content ) = sctk_malloc ( __size );

          /* copy */
          memcpy ( * ( __ret->selected_slot_content ),
              selected_slot->msg_content, __size );

          __ret->queue = fastmsg_queue;

          break;

        default:
          sctk_nodebug ( "ERROR : Type of the message not recognized !" );
          abort ();
          break;
      }
      selected_slot->is_msg_ready_to_read = 0;

      break;

      /**************************************************
       *                  RPC MESSAGE
       *************************************************/
    case SCTK_SHM_RPC_FAST:
      rpc_slot = ( struct sctk_shm_rpc_slot_s * ) selected_slot;

      __ret->msg_type = rpc_slot->shm_msg_type;

      switch ( rpc_slot->shm_msg_type ) {
        case ( SCTK_SHM_SLOT_TYPE_PTP_RPC | SCTK_SHM_SLOT_TYPE_FASTMSG ) :

          /* copy */
          memcpy ( __msg_content, rpc_slot->msg_content, __size );

          * ( __ret->selected_slot_content ) = __msg_content;
          __ret->queue = fastmsg_queue;

          break;

        default:
          sctk_error ( "Type of the message not recognized !" );
          sctk_abort ();
          break;
      }

      break;

      /**************************************************
       *                  RDMA MESSAGE
       *************************************************/
    case SCTK_SHM_RDMA_FAST:
      rpc_slot = ( struct sctk_shm_rpc_slot_s * ) selected_slot;
      sctk_nodebug ( "RDMA received, slot (%p)", rpc_slot );

      __ret->msg_type = rpc_slot->shm_msg_type;
      memcpy ( __msg_content, rpc_slot->msg_content, __size );
      * ( __ret->selected_slot_content ) = __msg_content;
      __ret->queue = fastmsg_queue;

      break;
  }

  /* update return values */
  __ret->selected_slot = selected_slot;

  sctk_nodebug ( "Queue moved to the next slot (slot %p destroyed) queue (%p)",
      selected_slot, __ret->queue );

  /* update first slot pointer */
  sctk_shm_move_first_flag ( fastmsg_queue, 1 );

  sctk_nodebug ( "First slot (%d) Last slot (%d)",
      fastmsg_queue->ptr_msg_first_slot,
      fastmsg_queue->ptr_msg_last_slot );

  sctk_spinlock_lock ( & ( fastmsg_queue->lock_count_msg_received ) );
  --fastmsg_queue->count_msg_received;
  sctk_spinlock_unlock ( & ( fastmsg_queue->lock_count_msg_received ) );

  return 0;
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_put_broadcast_msg
 *  Description:  write a new broadcast msg
 * ==================================================================
 */
  void
sctk_shm_put_broadcast_msg ( const void *__shm_msg,
    const size_t __size,
    struct sctk_shm_com_list_s *__com_list, const int is_reduce )
{
  struct sctk_shm_broadcast_slot_s *broadcast_slot;
  broadcast_slot = & ( __com_list->shm_broadcast_slot );

  if ( __size > SCTK_SHM_FASTMSG_BROADCAST_MAXLEN ) {
    sctk_shm_put_broadcast_bigmsg ( __shm_msg,
        __size,
        __com_list,
        SCTK_SHM_SLOT_TYPE_COLLECTIVE_BROADCAST,
        broadcast_slot,
        is_reduce );
  } else {
    /* smaller or equal to a big message slot */
    memcpy ( broadcast_slot->msg_content, __shm_msg, __size );

    sctk_nodebug ( "msg : %d - size : %d", ( ( int * ) __shm_msg ) [0], __size );

    broadcast_slot->shm_msg_type =
      SCTK_SHM_SLOT_TYPE_COLLECTIVE_BROADCAST | SCTK_SHM_SLOT_TYPE_FASTMSG;

    sctk_nodebug ( "fastmsg %d", broadcast_slot->shm_msg_type );
    /* update broadcast slot values */
    broadcast_slot->msg_size = __size;
    broadcast_slot->shm_is_msg_ready = 1;

    /* if this is a broadcast from a reduce */
    if ( is_reduce == 1 )
      __com_list->shm_reduce_slot.shm_is_broadcast_ready = 1;
  }
  return;
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_get_broadcast_msg
 *  Description:
 * ==================================================================
 */
inline void
sctk_shm_get_broadcast_msg ( const unsigned int __rank,
    void *__msg_content,
    const size_t __size,
    struct sctk_shm_com_list_s *__com_list ) {
  struct sctk_shm_broadcast_slot_s *broadcast_slot;

  broadcast_slot = & ( __com_list->shm_broadcast_slot );

  switch ( broadcast_slot->shm_msg_type ) {
    case ( SCTK_SHM_SLOT_TYPE_COLLECTIVE_BROADCAST | SCTK_SHM_SLOT_TYPE_FASTMSG ) :
      /* wait until the broadcast fastmsg is read */
      sctk_nodebug ( "Coll fast msg received : %p",
          __msg_content );

      memcpy ( __msg_content,
          broadcast_slot->msg_content, broadcast_slot->msg_size );

      break;

    case ( SCTK_SHM_SLOT_TYPE_COLLECTIVE_BROADCAST | SCTK_SHM_SLOT_TYPE_BIGMSG ) :
      //      sctk_nodebug ("Coll big msg received");
      sctk_shm_get_broadcast_bigmsg ( __rank,
          __msg_content,
          __com_list,
          broadcast_slot->msg_size,
          SCTK_SHM_BROADCAST );
      break;

  }
  return;
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_shm_put_reduce_msg
 *  Description:  write a new reduce msg
 * ==================================================================
 */
inline void
sctk_shm_put_reduce_msg ( const unsigned int __dest,
    const void *__shm_msg,
    const size_t __size,
    struct sctk_shm_com_list_s *__com_list ) {

  if ( ( __size > SCTK_SHM_FASTMSG_REDUCE_MAXLEN ) ) {
    sctk_nodebug ( "Trying to send in bigmsg queue" );

    sctk_shm_put_reduce_bigmsg ( __shm_msg, __size, __com_list );
  } else {
    sctk_nodebug
      ( "Trying to send in fastmsg queue (in : %p - out : %p - size %d)",
        __com_list->shm_reduce_slot.msg_content[sctk_local_process_rank], __shm_msg, __size );

    memcpy ( __com_list->shm_reduce_slot.msg_content[sctk_local_process_rank],
        __shm_msg, __size );

    __com_list->shm_reduce_slot.msg_size[sctk_local_process_rank] = __size;
    __com_list->shm_reduce_slot.shm_msg_type =
      SCTK_SHM_SLOT_TYPE_PTP_MESSAGE | SCTK_SHM_SLOT_TYPE_FASTMSG;

    /* set the msg ready */
    sctk_spinlock_lock ( & ( __com_list->shm_reduce_slot.lock ) );

    int i;
    for ( i = 0; i < SCTK_SHM_MAX_NB_LOCAL_PROCESSES; ++i ) {
      if ( __com_list->shm_reduce_slot.shm_is_msg_ready[i] == -1 ) {
        __com_list->shm_reduce_slot.shm_is_msg_ready[i] = sctk_local_process_rank;
        __com_list->shm_reduce_slot.msg_size[i] = __size;
        sctk_nodebug ( "Process %d in index %d", sctk_local_process_rank, i );
        break;
      }
    }
    sctk_spinlock_unlock ( & ( __com_list->shm_reduce_slot.lock ) );

    sctk_nodebug ( "Message sent in fastmsg reduce queue" );
  }
  return;
}


/*
 * ===  FUNCTION  ===================================================
 *         Name:  sctk_get_reduce_msg
 *  Description:
 * ==================================================================
 */
inline void
sctk_get_reduce_msg ( const unsigned int __dest, const size_t __size,
    void *__shm_msg, struct sctk_shm_com_list_s *__com_list ) {
  sctk_assert ( __shm_msg );

  switch ( __com_list->shm_reduce_slot.shm_msg_type ) {
    case ( SCTK_SHM_SLOT_TYPE_PTP_MESSAGE | SCTK_SHM_SLOT_TYPE_FASTMSG ) :
      memcpy ( __shm_msg,
          __com_list->shm_reduce_slot.msg_content[__dest],
          __com_list->shm_reduce_slot.msg_size[__dest] );
      break;

    case ( SCTK_SHM_SLOT_TYPE_PTP_MESSAGE | SCTK_SHM_SLOT_TYPE_BIGMSG ) :
      sctk_nodebug ( "Get big reduce msg from slot" );
      sctk_shm_get_reduce_bigmsg ( __shm_msg, __com_list );
      break;
  }
  return;
}
