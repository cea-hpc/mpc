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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef SCTK_CONTROL_MESSAGE_H
#define SCTK_CONTROL_MESSAGE_H

#include <sctk_inter_thread_comm.h>


/************************************************************************/
/* Control Messages Types                                               */
/************************************************************************/

/** This is the context of the control message engine */
struct sctk_control_message_context
{
	void (*sctk_user_control_message)( int source_process, int source_rank, char subtype, char param, void * data ); /**< This function is called when the application has registered a function */
};

void sctk_control_message_context_set_user( void (*fn)( int , int , char , char , void * ) );



/************************************************************************/
/* Control Messages Interface                                           */
/************************************************************************/

void sctk_control_messages_send ( int dest, sctk_message_class_t message_class, int subtype, int param, void *buffer, size_t size );
void sctk_control_messages_send_rail( int dest, int subtype, int param, void *buffer, size_t size, int  rail_id );
void sctk_control_messages_incoming( sctk_thread_ptp_message_t * msg );

#endif /* SCTK_CONTROL_MESSAGE_H */
