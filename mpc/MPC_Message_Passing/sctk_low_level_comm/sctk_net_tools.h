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
#ifndef __SCTK__NET_TOOLS_H_
#define __SCTK__NET_TOOLS_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include "sctk_low_level_comm.h"
  static void sctk_net_copy_in_buffer (sctk_thread_ptp_message_t * msg,
				       char *buffer)
  {
    size_t i;
    size_t j;
    size_t size;
      sctk_nodebug ("Write from %p", buffer);
    for (i = 0; i < msg->message.nb_items; i++)
      {
	if (msg->message.begins_absolute[i] != NULL)
	  {
	    for (j = 0; j < msg->message.sizes[i]; j++)
	      {
		size = (msg->message.ends_absolute[i][j] -
			msg->message.begins_absolute[i][j] +
			1) * msg->message.elem_sizes[i];
		memcpy (buffer, ((char *) (msg->message.adresses[i])) +
			msg->message.begins_absolute[i][j] *
			msg->message.elem_sizes[i], size);
		buffer += size;
	      }
	  }
	else
	  {
	    if (msg->message.begins[i] == NULL)
	      {
		size = msg->message.sizes[i];
		memcpy (buffer, msg->message.adresses[i], size);
		buffer += size;
	      }
	    else
	      {
		for (j = 0; j < msg->message.sizes[i]; j++)
		  {
		    size =
		      (msg->message.ends[i][j] - msg->message.begins[i][j] +
		       1) * msg->message.elem_sizes[i];
		    memcpy (buffer, (char *) (msg->message.adresses[i]) +
			    msg->message.begins[i][j] *
			    msg->message.elem_sizes[i], size);
		    buffer += size;
		  }
	      }
	  }
      }
  }

  static size_t sctk_net_determine_message_size (sctk_thread_ptp_message_t *
						 msg)
  {
    size_t total_size = 0;
    size_t i;

    for (i = 0; i < msg->message.nb_items; i++)
      {
	if (msg->message.begins_absolute[i] != NULL)
	  {
	    size_t j;
	    for (j = 0; j < msg->message.sizes[i]; j++)
	      {
		total_size +=
		  (msg->message.ends_absolute[i][j] -
		   msg->message.begins_absolute[i][j] +
		   1) * msg->message.elem_sizes[i];
	      }
	  }
	else
	  {
	    if (msg->message.begins[i] == NULL)
	      {
		total_size += msg->message.sizes[i];
	      }
	    else
	      {
		size_t j;
		for (j = 0; j < msg->message.sizes[i]; j++)
		  {
		    total_size +=
		      (msg->message.ends[i][j] - msg->message.begins[i][j] +
		       1) * msg->message.elem_sizes[i];
		  }
	      }
	  }
      }

    return total_size;
  }

#ifdef __cplusplus
}
#endif
#endif
