#include <lcr/lcr_component.h>

#include <utlist.h>
#include <mpc_common_debug.h>

#include "sctk_alloc.h"

/* Init component list */
lcr_component_t *lcr_component_list = NULL; 

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
