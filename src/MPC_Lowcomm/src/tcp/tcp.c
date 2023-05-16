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


#include <mpc_common_debug.h>
#include <sctk_net_tools.h>
#include <tcp_toolkit.h>
#include <sys/socket.h> /* shutdown() */

#include <sctk_alloc.h>

#include <dirent.h>
#ifdef MPC_LOWCOMM_PROTOCOL
#include <lcr/lcr_component.h>
#endif

#include "sctk_rail.h"

/********************************************************************/
/* Inter Thread Comm Hooks                                          */
/********************************************************************/

static void *__tcp_thread_loop(_mpc_lowcomm_endpoint_t *tmp)
{
	int fd = tmp->data.tcp.fd;

	mpc_common_debug("Rail %d from %d launched", tmp->rail->rail_number, tmp->dest);

	while(1)
	{
		mpc_lowcomm_ptp_message_t *msg;
		void *  body;
		size_t  size;
		ssize_t res;

		res = mpc_common_io_safe_read(fd, ( char * )&size, sizeof(size_t) );

		if( (res <= 0) )
		{
			/* EOF or ERROR */
			break;
		}

		if(res < (ssize_t)sizeof(size_t) )
		{
			break;
		}

		if(size < sizeof(mpc_lowcomm_ptp_message_body_t) )
		{
			break;
		}

		size = size - sizeof(mpc_lowcomm_ptp_message_body_t) + sizeof(mpc_lowcomm_ptp_message_t);

		msg = sctk_malloc(size);

		assume(msg != NULL);

		body = ( char * )msg + sizeof(mpc_lowcomm_ptp_message_t);


		/* Recv header*/
		res = mpc_common_io_safe_read(fd, ( char * )msg, sizeof(mpc_lowcomm_ptp_message_body_t) );

		if( (res <= 0) )
		{
			/* EOF or ERROR */
			break;
		}

		if(res != sizeof(mpc_lowcomm_ptp_message_body_t) )
		{
			break;
		}

		SCTK_MSG_COMPLETION_FLAG_SET(msg, NULL);
		msg->tail.message_type = MPC_LOWCOMM_MESSAGE_NETWORK;

		/* Recv body*/
		size = size - sizeof(mpc_lowcomm_ptp_message_t);

		res = mpc_common_io_safe_read(fd, ( char * )body, size);

		if( (res <= 0) )
		{
			/* EOF or ERROR */
			break;
		}

		_mpc_comm_ptp_message_clear_request(msg);
		_mpc_comm_ptp_message_set_copy_and_free(msg, sctk_free, sctk_net_message_copy);


		tmp->rail->send_message_from_network(msg);
	}

	shutdown(fd, SHUT_RDWR);
	close(fd);

	mpc_common_debug("TCP THREAD LEAVING");

	return NULL;
}

/**
 * Entry point for sending a message with the TCP network.
 * \param[in] msg the message to send
 * \param[in] endpoint the route to use
 */
static void _mpc_lowcomm_tcp_send_message(mpc_lowcomm_ptp_message_t *msg, _mpc_lowcomm_endpoint_t *endpoint)
{
	size_t size;
	int    fd;

	mpc_common_spinlock_lock(&(endpoint->data.tcp.lock) );

	fd = endpoint->data.tcp.fd;

	size = SCTK_MSG_SIZE(msg) + sizeof(mpc_lowcomm_ptp_message_body_t);

	mpc_common_nodebug("SEND MSG of size %d ENDPOINT TCP to %d", size, endpoint->dest);

	mpc_common_io_safe_write(fd, ( char * )&size, sizeof(size_t) );

	mpc_common_io_safe_write(fd, ( char * )msg, sizeof(mpc_lowcomm_ptp_message_body_t) );

	sctk_net_write_in_fd(msg, fd);
	mpc_common_spinlock_unlock(&(endpoint->data.tcp.lock) );

	mpc_common_nodebug("SEND MSG ENDPOINT TCP to %d DONE", endpoint->dest);

	mpc_lowcomm_ptp_message_complete_and_free(msg);
}


/**
 * Handler triggering the send_message_from_network call, before reaching the inter_thread_comm matching process.
 * \param[in] msg the message received from the network, to be matched w/ a local RECV.
 */
static int _mpc_lowcomm_tcp_send_message_from_network(mpc_lowcomm_ptp_message_t *msg)
{
	if(_mpc_lowcomm_reorder_msg_check(msg) == _MPC_LOWCOMM_REORDER_NO_NUMBERING)
	{
		/* No reordering */
		_mpc_comm_ptp_message_send_check(msg, 1);
	}

	return 1;
}

/********************************************************************/
/* TCP Init                                                         */
/********************************************************************/

/**
 * Procedure to clear the TCP rail.
 * \param[in,out] rail the rail to close.
 */
void _mpc_lowcomm_tcp_finalize(sctk_rail_info_t *rail)
{
	_mpc_lowcomm_tcp_rail_info_t *rail_tcp = &rail->network.tcp;

	shutdown(rail_tcp->sockfd, SHUT_RDWR);
	close(rail_tcp->sockfd);
	rail_tcp->sockfd = -1;
	rail_tcp->portno = -1;
	rail_tcp->connection_infos[0]   = '\0';
	rail_tcp->connection_infos_size = 0;
}

/**
 * Procedure to initialize a new TCP rail.
 * \param[in] rail the TCP rail
 */
void sctk_network_init_tcp(sctk_rail_info_t *rail)
{
	/* Register Hooks in rail */
	rail->send_message_endpoint     = _mpc_lowcomm_tcp_send_message;
	rail->notify_recv_message       = NULL;
	rail->notify_matching_message   = NULL;
	rail->notify_perform_message    = NULL;
	rail->notify_idle_message       = NULL;
	rail->notify_any_source_message = NULL;
	rail->send_message_from_network = _mpc_lowcomm_tcp_send_message_from_network;
	rail->driver_finalize           = _mpc_lowcomm_tcp_finalize;

	sctk_rail_init_route(rail, rail->runtime_config_rail->topology, tcp_on_demand_connection_handler);

	char * interface = rail->runtime_config_rail->device;

	if(!interface)
	{
		interface = "default";
	}

	/* Handle the IPoIB case */
    char net_name[1024];
	snprintf(net_name, 1024, "TCP (%s)", interface);
    rail->network_name = strdup(net_name);

	/* Actually initialize the network (note TCP kind specific functions) */
	sctk_network_init_tcp_all(rail, interface, __tcp_thread_loop);
}

int lcr_tcp_get_attr(sctk_rail_info_t *rail,
                     lcr_rail_attr_t *attr)
{
        struct _mpc_lowcomm_config_struct_net_driver_tcp tcp_driver =
                rail->runtime_config_driver_config->driver.value.tcp;
        attr->iface.cap.am.max_iovecs = 6; //FIXME: arbitrary value...
        attr->iface.cap.am.max_bcopy = 0; /*FIXME: send() return whenever data has been
                                            copied to kernel buf. So, potentiel benefit 
                                            compared to zcopy...
                                            TODO: check MPI_BSend */
        attr->iface.cap.am.max_zcopy = tcp_driver.max_msg_size; 

        attr->iface.cap.tag.max_bcopy = 0; /* No tag-matching capabilities */
        attr->iface.cap.tag.max_zcopy = 0; /* No tag-matching capabilities */

        attr->iface.cap.rndv.max_put_zcopy = tcp_driver.max_msg_size;
        attr->iface.cap.rndv.max_get_zcopy = tcp_driver.max_msg_size;

        return MPC_LOWCOMM_SUCCESS;
}

#ifdef MPC_LOWCOMM_PROTOCOL
/**
 * Function called by each started polling thread, processing message on the given route.
 * \param[in] tmp the route to progress
 * \return NULL
 */
static inline int lcr_tcp_invoke_am(sctk_rail_info_t *rail, 
				    uint8_t am_id,
				    size_t length,
				    void *data)
{
	int rc = MPC_LOWCOMM_SUCCESS;

	lcr_am_handler_t handler = rail->am[am_id];
	if (handler.cb == NULL) {
		mpc_common_debug_fatal("LCP: handler id %d not supported.", am_id);
	}

	rc = handler.cb(handler.arg, data, length, 0);
	if (rc != MPC_LOWCOMM_SUCCESS) {
		mpc_common_debug_error("LCP: handler id %d failed.", am_id);
	}

	return rc;
}

static void *lcr_tcp_thread_loop(_mpc_lowcomm_endpoint_t *ep)
{
	int fd = ep->data.tcp.fd;

	mpc_common_debug("Rail %d from %d launched", ep->rail->rail_number, ep->dest);

	while(1)
	{
		ssize_t recv_length;
		void *data;
		lcr_tcp_am_hdr_t hdr;

		recv_length = mpc_common_io_safe_read(fd, (char *)&hdr, sizeof(hdr));

		if (recv_length < 0) {
			break;
		} else if (recv_length < (ssize_t)sizeof(hdr)) {
			break;
		} else {
			data = sctk_malloc(hdr.length);
			recv_length = mpc_common_io_safe_read(fd, data, hdr.length);
			if (recv_length <= 0) 
				break;
			else if ((size_t)recv_length < hdr.length)
				break;

			lcr_tcp_invoke_am(ep->rail, hdr.am_id, hdr.length, data);
			sctk_free(data);
		}

	}

	shutdown(fd, SHUT_RDWR);
	close(fd);

	mpc_common_debug("TCP THREAD LEAVING");

	return NULL;
}

/*
 * Blocking send call with copy, because it is a blocking call we can free
 * the buffer
 *
 */
static ssize_t lcr_tcp_send_am_bcopy(_mpc_lowcomm_endpoint_t *ep, uint8_t id, 
				     lcr_pack_callback_t pack, void *arg,
				     __UNUSED__ unsigned flags)
{
	sctk_rail_info_t *rail = ep->rail;
	uint32_t payload_length;
	ssize_t sent;

	lcr_tcp_am_hdr_t *hdr = sctk_malloc(rail->network.tcp.bcopy_buf_size);
	if (hdr == NULL) {
	       mpc_common_debug_error("Could not allocate TCP buffer.");
	       payload_length = -1;
	       goto err;
	}
	memset(hdr, 0, rail->network.tcp.bcopy_buf_size);

	hdr->am_id = id;
	hdr->length = payload_length = pack(hdr + 1, arg);

	mpc_common_spinlock_lock(&(ep->data.tcp.lock));
	sent = mpc_common_io_safe_write(ep->data.tcp.fd, hdr, hdr->length + sizeof(*hdr));
	mpc_common_spinlock_unlock(&(ep->data.tcp.lock));
	if (sent < 0) {
		payload_length = -1;
	}

	sctk_free(hdr);	
err:
	return payload_length;	
}

static void lcr_tcp_send_am_prepare(const struct iovec *iov, int iovcnt,
				   const void *header, unsigned hdr_length,
				   uint8_t id, lcr_tcp_am_zcopy_hdr_t *hdr, 
				   size_t *payload_length)
{
	int i;

	hdr->base.am_id = id;
	hdr->iovcnt = 0;
	hdr->iov[hdr->iovcnt].iov_base = hdr;
	hdr->iov[hdr->iovcnt].iov_len = sizeof(lcr_tcp_am_hdr_t);
	hdr->iovcnt++;

	if (hdr_length > 0) {
		hdr->iov[hdr->iovcnt].iov_base = header;
		hdr->iov[hdr->iovcnt].iov_len = hdr_length;
		hdr->iovcnt++;
	}

	/* Copy iov and compute payload length */
	*payload_length = 0;
	for (i=0; i<iovcnt; i++) {
	       hdr->iov[hdr->iovcnt].iov_base = iov[i].iov_base;
	       hdr->iov[hdr->iovcnt].iov_len  = iov[i].iov_len;
	       *payload_length += iov[i].iov_len;
	       hdr->iovcnt++;
	}
}

static int lcr_tcp_send_am_zcopy(_mpc_lowcomm_endpoint_t *ep,
				 uint8_t id, const void *header,
				 unsigned hdr_length, const struct iovec *iov,
				 size_t iovcnt, __UNUSED__ unsigned flags,
				 lcr_completion_t *comp)
{
	int rc;
	lcr_tcp_am_zcopy_hdr_t *hdr = NULL;
	size_t payload_length;
	ssize_t sent;
	sctk_rail_info_t *rail = ep->rail;

	hdr = sctk_malloc(rail->network.tcp.zcopy_buf_size*sizeof(char));
	if (hdr == NULL) {
		mpc_common_debug_error("Could not allocate zcopy header.");
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}
	memset(hdr, 0, rail->network.tcp.zcopy_buf_size*sizeof(char));

	lcr_tcp_send_am_prepare(iov, iovcnt, header, hdr_length,
				id, hdr, &payload_length);
	hdr->base.length = payload_length + hdr_length;

	mpc_common_spinlock_lock(&(ep->data.tcp.lock));
	sent = mpc_common_iovec_safe_write(ep->data.tcp.fd, hdr->iov, hdr->iovcnt, 
					   hdr->base.length + sizeof(lcr_tcp_am_hdr_t));
	mpc_common_spinlock_unlock(&(ep->data.tcp.lock));
	if (sent < 0) {
		rc = MPC_LOWCOMM_ERROR;
		goto clean_buf;
	}

	/* Call completion callback */
	comp->sent = payload_length;
	comp->comp_cb(comp);

	rc = MPC_LOWCOMM_SUCCESS;
clean_buf:
	sctk_free(hdr);
err:
	return rc;	
}

int lcr_tcp_init_iface(sctk_rail_info_t *rail)
{
        //FIXME: to pass the assert in sctk_network_init_tcp_all
        rail->send_message_from_network = _mpc_lowcomm_tcp_send_message_from_network;

	/* New API */
	rail->send_am_bcopy  = lcr_tcp_send_am_bcopy;
	rail->send_am_zcopy  = lcr_tcp_send_am_zcopy;
        rail->iface_get_attr = lcr_tcp_get_attr;

        /* init config */
        rail->network.tcp.max_iov = 8;
        rail->network.tcp.bcopy_buf_size = 32768; /* 32kb */
        rail->network.tcp.zcopy_buf_size = sizeof(lcr_tcp_am_zcopy_hdr_t) +
                rail->network.tcp.max_iov * sizeof(struct iovec);

        /* Init capabilities */
        rail->cap = LCR_IFACE_CAP_REMOTE;

	sctk_rail_init_route(rail, rail->runtime_config_rail->topology, tcp_on_demand_connection_handler);

	char * interface = rail->runtime_config_rail->device;

	if(!interface)
	{
		interface = "default";
	}

        /* Handle the IPoIB case */
        char net_name[1024];
        snprintf(net_name, 1024, "TCP (%s)", interface);
        rail->network_name = strdup(net_name);

        /* Actually initialize the network (note TCP kind specific functions) */
        sctk_network_init_tcp_all(rail, interface, lcr_tcp_thread_loop);

        return MPC_LOWCOMM_SUCCESS;
}

int lcr_tcp_query_devices(__UNUSED__ lcr_component_t *component,
                          lcr_device_t **devices_p,
                          unsigned int *num_devices_p)
{

        int rc = MPC_LOWCOMM_SUCCESS;
        static const char *net_dir = "/sys/class/net";
        lcr_device_t *devices;
        int num_devices;
        struct dirent *entry;
        DIR *dir;

        dir = opendir(net_dir);
        if (dir == NULL) {
                mpc_common_debug_error("TCP: could not find net directory");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        devices = NULL;
        num_devices = 0;
        for (;;) {
                errno = 0;
                entry = readdir(dir);
                if (entry == NULL) {
                        if (errno != 0) {
                                mpc_common_debug_error("TCP: net directory exists "
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

                devices = sctk_realloc(devices, sizeof(*devices) * (num_devices + 1));
                if (devices == NULL) {
                        mpc_common_debug_error("PTL: could not allocate devices");
                        rc = MPC_LOWCOMM_ERROR;
                        goto close_dir;
                }
                
                strcpy(devices[num_devices].name, entry->d_name);
                ++num_devices;
        }

        *devices_p = devices;
        *num_devices_p = num_devices;

close_dir:
        closedir(dir);
err:
        return rc;
}

int lcr_tcp_iface_open(char *device_name, int id,
		       lcr_rail_config_t *rail_config, 
                       lcr_driver_config_t *driver_config,
                       sctk_rail_info_t **iface_p)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        sctk_rail_info_t *iface = NULL;
        UNUSED(device_name);

        lcr_rail_init(rail_config, driver_config, &iface);
        if (iface == NULL) {
                mpc_common_debug_error("LCR: could not allocate tcp rail");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        //TODO: no support for user specified interface
        //      resetting to default.
        strcpy(iface->device_name, "default");
	iface->rail_number = id;

        rc = lcr_tcp_init_iface(iface);

        *iface_p = iface;
err:
       return rc; 
}

lcr_component_t tcp_component = {
        .name = { "tcp" },
        .rail_name = { "tcpmpi" },
        .query_devices = lcr_tcp_query_devices,
        .iface_open = lcr_tcp_iface_open,
        .devices = NULL,
        .num_devices = 0,
        .flags = 0,
        .next = NULL
};
LCR_COMPONENT_REGISTER(&tcp_component)
#endif
