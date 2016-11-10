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
/* #   - Valat Sébastien sebastien.valat@cea.fr                           # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef SCTK_ALLOC_RFQ_H
#define SCTK_ALLOC_RFQ_H

#ifdef __cplusplus
extern "C"
{
#endif

/********************************* INCLUDES *********************************/
#include "sctk_alloc_mpscf_queue.h"

/******************************** STRUCTURE *********************************/
/**
 * Struct used to register free chunks into the free queue list on a remote allocation chaine.
 * RFQ = Remote Free Queue
**/
struct sctk_alloc_rfq_entry
{
	struct sctk_mpscf_queue_entry entry;
	void * ptr;
};

/******************************** STRUCTURE *********************************/
/**
 * Struct to define a remote free queue for parallel free. RFQ = Remote Free Queue
**/
struct sctk_alloc_rfq
{
	struct sctk_mpscf_queue queue;
};

/************************* FUNCTION ************************/
//remote free queue management for allocation chains
void sctk_alloc_rfq_init(struct sctk_alloc_rfq *rfq);
bool sctk_alloc_rfq_empty(struct sctk_alloc_rfq *rfq);
SCTK_PUBLIC void sctk_alloc_rfq_register(struct sctk_alloc_rfq * rfq,void * ptr);
struct sctk_alloc_rfq_entry *sctk_alloc_rfq_extract(struct sctk_alloc_rfq *rfq);
int sctk_alloc_rfq_count_entries(struct sctk_alloc_rfq_entry *entries);
void sctk_alloc_rfq_destroy(struct sctk_alloc_rfq *rfq);

#ifdef __cplusplus
}
#endif

#endif /* SCTK_ALLOC_RFQ_H */
