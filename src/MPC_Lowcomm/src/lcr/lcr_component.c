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

#include <lcr/lcr_component.h>

#include <utlist.h>
#include <string.h>
#include <mpc_common_debug.h>

#include "sctk_alloc.h"

/* Init component list */
lcr_component_t *lcr_component_list = NULL;


lcr_component_t * lcr_query_component_by_name(const char * name)
{
        lcr_component_t * tmp = NULL;
        LL_FOREACH(lcr_component_list, tmp) {
                if(!strcmp(name, tmp->name))
                {
                        return tmp;
                }
        }

        return NULL;
}



int lcr_query_components(lcr_component_h **components_p,
                         unsigned *num_components_p)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcr_component_h *components;
        lcr_component_t  *component;
        unsigned num_components;

        LL_COUNT(lcr_component_list, component, num_components);

        components = sctk_malloc(num_components*sizeof(lcr_component_h));
        if (components == NULL) {
                mpc_common_debug_error("LCR: could not allocate components");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        *num_components_p = num_components;
        *components_p = components;

        LL_FOREACH(lcr_component_list, component) {
                *(components++) = component;
        }

err:
        return rc;
}

int lcr_free_components(lcr_component_h *components, unsigned num_components, int devices)
{
        int i;
        if (devices) {
                for (i=0; i<(int)num_components; i++) {
                        sctk_free(components[i]->devices);
                }
        }
        sctk_free(components);
        return MPC_LOWCOMM_SUCCESS;
}
