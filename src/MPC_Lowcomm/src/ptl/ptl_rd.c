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
/* #   - ADAM Julien julien.adam@paratools.com                            # */
/* #   - BESNARD Jean-Baptiste jean-baptiste.besnard@paratools.com        # */
/* #   - MOREAU Gilles gilles.moreau@cea.fr                               # */
/* #                                                                      # */
/* ######################################################################## */

#include <mpc_config.h>

#ifdef MPC_USE_PORTALS

#include "ptl.h"
#include "mpc_launch_pmi.h"
#include "rail.h"

#include <lcr_component.h>

#include <dirent.h>

static int max_num_devices = 0;


int lcr_ptl_query_devices(__UNUSED__ lcr_component_t *component,
                          lcr_device_t **devices_p,
                          unsigned int *num_devices_p)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        static const char *bxi_dir = "/sys/class/bxi";
        lcr_device_t *devices;
        int is_up;
        int num_devices;
        struct dirent *entry;
        DIR *dir;

        devices = NULL;
        num_devices = 0;

        /* First, try simulator */
        const char *ptl_iface_name; 
        if ((ptl_iface_name = getenv("PTL_IFACE_NAME")) != NULL) {
                devices = sctk_malloc(sizeof(lcr_device_t));
                strcpy(devices[0].name, ptl_iface_name);
                num_devices = 1;
                goto out;
        }

        /* Then, check if bxi are available in with sysfs */
        dir = opendir(bxi_dir);
        if (dir == NULL) {
                mpc_common_debug_warning("LCR PTL: could not find any ptl device.");
                goto out;
        }

        for (;;) {
                errno = 0;
                entry = readdir(dir);
                if (entry == NULL) {
                        if (errno != 0) {
                                mpc_common_debug_error("LCR PTL: bxi directory exists "
                                                       "but no entry found.");
                                rc = MPC_LOWCOMM_ERROR;
                                goto close_dir;
                        }
                        break;
                }

                /* avoid reading entry like . and .. */
                if (entry->d_type != DT_LNK) {
                        continue;
                }

                is_up = 1;
                //TODO: check if interface is up with bixnic -i <iface> info
                //      LINK_STATUS
                if (!is_up) {
                        continue;
                }

                devices = sctk_realloc(devices, sizeof(*devices) * (num_devices + 1));
                if (devices == NULL) {
                        mpc_common_debug_error("PTL: could not allocate devices");
                        rc = MPC_LOWCOMM_ERROR;
                        goto close_dir;
                }
                
                //FIXME: interface name should always be of the form:
                //       bxi<id> with id, 0 < id < 9 
                strcpy(devices[num_devices].name, entry->d_name);
                ++num_devices;
        }

close_dir:
        closedir(dir);

out:
        //FIXME: hack to be sure the ptl device have correct id in iface_open
        max_num_devices = num_devices;
        *devices_p = devices;
        *num_devices_p = num_devices;

        return rc;
}

int lcr_ptl_iface_open(__UNUSED__ const char *device_name, int id,
		       lcr_rail_config_t *rail_config, 
		       lcr_driver_config_t *driver_config,
		       sctk_rail_info_t **iface_p,
                       unsigned flags)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        unsigned init_flags = 0;
        sctk_rail_info_t *iface = NULL;

        lcr_rail_init(rail_config, driver_config, &iface);
        if (iface == NULL) {
                mpc_common_debug_error("LCR PTL: could not allocate tcp rail");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        //FIXME: modulo on max_num_devices so that they are correctly setup
	iface->rail_number = id % max_num_devices; /* used as tag for pmi registration */

        /* Init capabilities */
        iface->cap = LCR_IFACE_CAP_RMA | 
                LCR_IFACE_CAP_LOOPBACK |
                LCR_IFACE_CAP_REMOTE;

        if (flags & LCR_FEATURE_TS) {
                if (driver_config->driver.value.portals.offload) {
                        init_flags |= LCR_PTL_FEATURE_TAG;
                        iface->cap |= LCR_IFACE_CAP_TAG_OFFLOAD;
                } else {
                        init_flags |= LCR_PTL_FEATURE_AM | 
                                LCR_PTL_FEATURE_RMA;
                }
        }

        /* If RMA was requested through flags, then this means it should be used
         * alone thus remove AM feature. */
        if (flags & LCR_FEATURE_OS) 
                init_flags |= LCR_PTL_FEATURE_RMA;

        rc = lcr_ptl_iface_init(iface, init_flags);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

	lcr_ptl_rail_info_t* srail = &iface->network.ptl;
	/* serialize the id_t to a string, compliant with string  handling */
	srail->connection_infos_size = lcr_ptl_data_serialize(
			&srail->addr,   /* the interface id to serialize */
			sizeof(lcr_ptl_addr_t),  /* size of the Portals ID struct */
			srail->connection_infos, /* the string to store the serialization */
			MPC_COMMON_MAX_STRING_SIZE          /* max allowed string's size */
	);
	assert(srail->connection_infos_size > 0);

	if(mpc_launch_pmi_is_initialized())
	{
		/* register the serialized id into the PMI */
		int tmp_ret = mpc_launch_pmi_put_as_rank (
						srail->connection_infos,      /* the string to publish */
						iface->rail_number,             /* rail ID: PMI tag */
						0 /* Not local */
						);
		assert(tmp_ret == 0);
	}
        /* Add new API call */
	//iface->iface_get_attr      = lcr_ptl_get_attr;

        /* Active message calls */
        iface->send_am_bcopy        = lcr_ptl_send_am_bcopy;
        iface->send_am_zcopy        = lcr_ptl_send_am_zcopy;
        /* Tag calls */
        iface->send_tag_bcopy       = lcr_ptl_send_tag_bcopy;
        iface->send_tag_zcopy       = lcr_ptl_send_tag_zcopy;
        iface->post_tag_zcopy       = lcr_ptl_post_tag_zcopy;
        iface->connect_on_demand    = lcr_ptl_connect_on_demand;
        /* RMA calls */
        iface->put_bcopy            = lcr_ptl_send_put_bcopy;
        iface->put_zcopy            = lcr_ptl_send_put_zcopy;
        iface->get_zcopy            = lcr_ptl_send_get_zcopy;
        iface->get_tag_zcopy        = lcr_ptl_get_tag_zcopy;
        iface->iface_pack_memp      = lcr_ptl_pack_rkey;
        iface->iface_unpack_memp    = lcr_ptl_unpack_rkey;
        iface->iface_register_mem   = lcr_ptl_mem_register;
        iface->iface_unregister_mem = lcr_ptl_mem_unregister;
        /* Endpoint Sync calls */
        iface->ep_flush            = lcr_ptl_ep_flush;
        /* Interface progress */
        iface->iface_progress      = lcr_ptl_iface_progress;
        iface->iface_is_reachable  = lcr_ptl_iface_is_reachable;
        
        iface->driver_finalize     = lcr_ptl_iface_fini;

        *iface_p = iface;
err:
        return rc; 
}

lcr_component_t ptl_component = {
        .name = { "portalsmpi" },
        .query_devices = lcr_ptl_query_devices,
        .iface_open = lcr_ptl_iface_open,
        .devices = NULL,
        .num_devices = 0,
        .flags = 0,
        .next = NULL
};
LCR_COMPONENT_REGISTER(&ptl_component)

lcr_component_t mptl_component = {
        .name = { "mportalsmpi" },
        .query_devices = lcr_ptl_query_devices,
        .iface_open = lcr_ptl_iface_open,
        .devices = NULL,
        .num_devices = 0,
        .flags = 0,
        .next = NULL
};
LCR_COMPONENT_REGISTER(&mptl_component)

#endif
