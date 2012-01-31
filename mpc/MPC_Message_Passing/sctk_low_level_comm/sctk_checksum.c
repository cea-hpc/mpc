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
/* #   - DIDELOT Sylvain didelot.sylvain@gmail.com                        # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef SCTK_USE_CHECKSUM
#include "sctk_checksum.h"
#include "sctk_shell_colors.h"
#include <zlib.h>

static char checksum_enabled = 1;

unsigned long sctk_checksum_message(sctk_thread_ptp_message_t *msg) {
  unsigned long adler = 0;
  if (!checksum_enabled) return 0;

  switch(msg->tail.message_type){
  case sctk_message_contiguous: {
    size_t size;
    size = msg->body.header.msg_size;
    if(size > msg->tail.message.contiguous.size){
      size = msg->tail.message.contiguous.size;
    }
    adler = adler32(adler, (unsigned char*) msg->tail.message.contiguous.addr, size);
    break;
  }
  case sctk_message_network: {
    size_t size;
    void* body;

    size = msg->body.header.msg_size;
    body = (char*)msg + sizeof(sctk_thread_ptp_message_t);

    adler = adler32(adler, (unsigned char*) body, size);
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
    adler = adler32(adler, ((unsigned char *) (msg->tail.message.pack.list.std[i].addr)) +
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
	  adler = adler32(adler, ((unsigned char *) (msg->tail.message.pack.list.absolute[i].addr)) +
		 msg->tail.message.pack.list.absolute[i].begins[j] *
		 msg->tail.message.pack.list.absolute[i].elem_size,size);
	}
    break;
  }
  default: not_reachable();
  }
  return adler;
}

unsigned long sctk_checksum_buffer(char* body,
  sctk_thread_ptp_message_t *msg) {
  uLong adler = 0;
  if (!checksum_enabled) return 0;

  switch(msg->tail.message_type){
  case sctk_message_contiguous: {
    size_t size;
    size = msg->body.header.msg_size;
    if(size > msg->tail.message.contiguous.size){
      size = msg->tail.message.contiguous.size;
    }

    adler = adler32(adler, (unsigned char*) body, size);

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
    adler = adler32(adler, (unsigned char*) body, size);
	  body += size;
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

    adler = adler32(adler, (unsigned char*) body, size);
	  body += size;
	}
    break;
  }
  default: not_reachable();
  }
  return adler;
}


void sctk_checksum_register(sctk_thread_ptp_message_t *msg) {
  msg->body.checksum = sctk_checksum_message(msg);
}
void sctk_checksum_unregister(sctk_thread_ptp_message_t *msg) {
  msg->body.checksum = 0;
}

unsigned long sctk_checksum_verify(sctk_thread_ptp_message_t *send, sctk_thread_ptp_message_t *recv) {
  unsigned long adler = 0;
  if (!checksum_enabled) return 0;

  /* If Checksum != 0, we verify it because the user asked for it */
  if (send->body.checksum) {
    adler = sctk_checksum_message(recv);
    if (adler != send->body.checksum) {
#ifdef MPC_USE_INFINIBAND
      sctk_ib_print_msg(send);
#endif
      sctk_error("Got wrong checksum (got:%lu, expected:%lu)", adler, send->body.checksum);
      sctk_abort();
    }
  }
  return adler;
}

void sctk_checksum_init() {
  if (sctk_process_rank == 0) {
    fprintf(stderr, SCTK_COLOR_RED_BOLD(WARNING: inter-node message checking enabled!)"\n");
  }
}
#else
void sctk_checksum_init() { /* nothing to do */ }

#endif
