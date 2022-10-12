#ifndef LCR_COMPONENT_H
#define LCR_COMPONENT_H

#include <lcr/lcr_def.h>
#include <lowcomm_config.h>
#include <lcp_common.h>

#include <utlist.h>

enum {
        LCR_COMPONENT_DEVICE_NUM = LCP_BIT(0),
        LCR_COMPONENT_DEVICES    = LCP_BIT(1)
};

#define LCR_COMPONENT_REGISTER(_component) \
        extern lcr_component_t *lcr_component_list; \
        MPC_STATIC_INIT { \
               LL_APPEND(lcr_component_list, _component); \
        }

typedef struct lcr_device {
        char name[LCR_DEVICE_NAME_MAX];
} lcr_device_t;

typedef struct lcr_component {
        char                  name[LCR_COMPONENT_NAME_MAX];
        char                  rail_name[LCR_COMPONENT_NAME_MAX];
        lcr_device_t         *devices;
        unsigned              num_devices;
        int                   count; /* Number of current instance */
        unsigned              flags;
        struct lcr_component *next; /* Next component in the list */
        int (*query_devices)(struct lcr_component *component, 
                             lcr_device_t **device_list, 
                             unsigned int *num_devices);
        int (*iface_open)(lcr_rail_config_t *rail_config, 
                          lcr_driver_config_t *driver_config,
                          sctk_rail_info_t **iface_p);
} lcr_component_t;


int lcr_query_components(lcr_component_h **components_p, 
                         unsigned *num_components_p);
int lcr_free_components(lcr_component_h *components, 
                        unsigned num_components,
                        int devices);

#endif
