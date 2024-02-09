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

#ifndef LCR_COMPONENT_H
#define LCR_COMPONENT_H

#include <lcr_def.h>
#include <lowcomm_config.h>

#include <utlist.h>
#include <mpc_common_bit.h>

enum {
        LCR_COMPONENT_DEVICE_NUM = MPC_BIT(0),
        LCR_COMPONENT_DEVICES    = MPC_BIT(1)
};

#define LCR_COMPONENT_REGISTER(_component) \
        extern lcr_component_t *lcr_component_list; \
        MPC_STATIC_INIT { \
                static int init_done = 0; \
                if(init_done) return; \
                init_done = 1; \
                LL_APPEND(lcr_component_list, _component); \
        }

typedef struct lcr_device {
        char name[LCR_DEVICE_NAME_MAX];
} lcr_device_t;

typedef struct lcr_component {
        char                  name[LCR_COMPONENT_NAME_MAX];
        lcr_rail_config_t    *rail_config;
        lcr_driver_config_t  *driver_config;
        lcr_device_t         *devices;
        unsigned              num_devices;
        int                   count; /* Number of current instance */
        unsigned              flags;
        struct lcr_component *next; /* Next component in the list */
        int (*query_devices)(struct lcr_component *component,
                             lcr_device_t **device_list,
                             unsigned int *num_devices);
        int (*iface_open)(const char *device_name, int id,
			  lcr_rail_config_t *rail_config,
                          lcr_driver_config_t *driver_config,
                          sctk_rail_info_t **iface_p, 
                          unsigned fflags /* feature flags */);
} lcr_component_t;


lcr_component_t * lcr_query_component_by_name(const char * name);

int lcr_query_components(lcr_component_h **components_p,
                         unsigned *num_components_p);
int lcr_free_components(lcr_component_h *components,
                        unsigned num_components,
                        int devices);

#endif
