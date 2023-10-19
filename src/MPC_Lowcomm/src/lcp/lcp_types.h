/* ############################# MPC License ############################## */
/* # Thu May  6 10:26:16 CEST 2021                                        # */
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
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - CANAT Paul pcanat@paratools.fr                                     # */
/* # - BESNARD Jean-Baptiste jbbesnard@paratools.com                      # */
/* # - MOREAU Gilles gilles.moreau@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef LCP_TYPES_H
#define LCP_TYPES_H

#include <stdint.h>
#include <sys/uio.h> //iovec

/* Config */
#define LCP_CONF_STRING_SIZE 512

/* Chanel */
#define LCP_MAX_CHANNELS 6
#define LCP_NULL_CHANNEL ((lcp_chnl_idx_t)-1)
typedef uint8_t lcp_chnl_idx_t; /* Communication channel index */

/* Active Message */
enum {
        LCP_AM_ID_EAGER_TAG = 0,  /* Eager Tag                             */
        LCP_AM_ID_EAGER_TAG_SYNC, /* Eager Tag Synchronize                 */
        LCP_AM_ID_EAGER_AM,       /* Eager Active message                  */
        LCP_AM_ID_EAGER_AM_SYNC,  /* Eager Active message                  */
        LCP_AM_ID_ACK_SYNC,       /* Ack synchronize                       */
        LCP_AM_ID_ACK_AM_SYNC,    /* Ack synchronize                       */
        LCP_AM_ID_RTS_AM,         /* Ready To Send (RTS) Active Message    */
        LCP_AM_ID_RTS_TAG,        /* Ready To Send (RTS) Tag               */
        LCP_AM_ID_RTR,            /* Ready To Receive (RTR) Rendez-vous    */
        LCP_AM_ID_RTR_TM,         /* Ready To Receive (RTR) TM Rendez-vous */
        LCP_AM_ID_RFIN,           /* Rendez-vous FIN (RFIN)                */
        LCP_AM_ID_RFIN_TM,        /* Rendez-vous FIN (RFIN) TM             */
        LCP_AM_ID_LAST
};

#define LCP_AM_ID_USER_MAX 64

typedef enum {
	LCP_RNDV_GET = 1,
	LCP_RNDV_PUT,
	LCP_RNDV_SEND
} lcp_rndv_mode_t;

#endif
