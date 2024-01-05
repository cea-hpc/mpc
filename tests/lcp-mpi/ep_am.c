#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#include <lcp.h>
#include <mpc_conf.h>
#include <mpc_lowcomm.h>
#include <mpc_launch_pmi.h>
#include <mpc_lowcomm_monitor.h>

//NOTE: completion flag:
//      * 0: pending
//      * 1: done
//      * 2: cancelled

static const uint64_t magic = 0xDFDFDFDFDFDFDFDFllu;

typedef struct hdr_request {
        size_t   size;
        uint64_t magic;
} hdr_request_t;

typedef struct request {
        void  *buffer;
        size_t size;
        lcp_ep_h reply_ep;
        int completion_flag;
} request_t;

typedef struct gateway {
        lcp_context_h ctx;
        lcp_task_h    task;
        request_t     request;
} gateway_t;

int recv_completion(size_t sent, void *user_data) {
        struct request *req = (struct request *)user_data;

        printf("req=%p\n", req);
        req->completion_flag = 1;
}

#define AM_ID 1
int am_handler(void *arg, void *user_hdr, size_t hdr_size, 
               void *data, size_t length,
               lcp_am_recv_param_t *am_param) 
{
        int rc = 0;
        gateway_t *gateway = (gateway_t *)arg;
        hdr_request_t *hdr = (hdr_request_t *)user_hdr;
        
        assert(hdr->magic == magic);
        gateway->request.buffer = malloc(hdr->size);
        gateway->request.size   = hdr->size;
        if (am_param->flags & LCP_AM_RNDV) {
                gateway->request.size     = hdr->size;
                gateway->request.reply_ep = am_param->reply_ep;

                lcp_request_param_t param = {
                        .on_am_completion = recv_completion,
                        .datatype = LCP_DATATYPE_CONTIGUOUS,
                        .flags = LCP_REQUEST_AM_CALLBACK,
                        .user_request = &gateway->request,
                };
                rc = lcp_am_recv_nb(gateway->task, data, gateway->request.buffer, 
                                    hdr->size, &param);
                if (rc != LCP_SUCCESS) {
                        printf("ERROR: could not receive data");
                }
        } else if (am_param->flags & LCP_AM_EAGER) {
                memcpy(gateway->request.buffer, hdr + 1, hdr->size);
                
                recv_completion(length, &gateway->request);
        }

        return rc;
}

int main(int argc, char** argv) {
	int rc, i;
        /* LCP variables */
	lcp_context_h ctx;
        lcp_context_param_t param;
        lcp_task_h task;
	lcp_ep_h ep;
        /* Example variables */
        gateway_t gateway;
        hdr_request_t hdr;
	mpc_lowcomm_set_uid_t suid;
        int src_tid, dest_tid, my_tid;
	mpc_lowcomm_peer_uid_t src_uid, dest_uid, my_uid;
	int check = 1;

	if (argc != 2) {
		printf("error: one argument needed, [size]\n");
		return 1;
	}
	size_t size = atoi(argv[1]);

	/* uid must be set before initializing data */
	int *data = malloc(size * sizeof(int));
	memset(data, 0, size * sizeof(int));
	int *data_check = malloc(size * sizeof(int));
	memset(data_check, 0, size * sizeof(int));

        param = (lcp_context_param_t) {
                .flags = LCP_CONTEXT_PROCESS_UID,
                .process_uid = mpc_lowcomm_monitor_get_uid()
        };
        /* create communication context */
        rc = lcp_context_create(&ctx, &param);
        if (rc != 0) {
                printf("ERROR: create context\n");
                goto err;
        }
        
        /* pmi barrier needed to commit pmi rail registration */
        mpc_launch_pmi_barrier();

	/* get uids */
	suid = mpc_lowcomm_monitor_get_gid();
        my_tid = mpc_lowcomm_get_rank();

        if (my_tid == 0) {
                src_tid  = mpc_lowcomm_get_rank();
                src_uid  = mpc_lowcomm_monitor_get_uid();
                dest_tid = 1;
                dest_uid = mpc_lowcomm_monitor_uid_of(suid, 0);
        } else {
                src_tid = 0;
                src_uid = mpc_lowcomm_monitor_uid_of(suid, 0);
                dest_uid = mpc_lowcomm_monitor_get_uid();
                dest_tid  = mpc_lowcomm_get_rank();
        }

        rc = lcp_task_create(ctx, my_tid, &task);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                printf("ERROR: create task");
                goto err;
        }

        gateway.ctx  = ctx;
        gateway.task = task;
        rc = lcp_am_set_handler_callback(task, AM_ID, &gateway, am_handler, 0);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

	/* init data */
	srand(0);
	for (i=0; i<(int)size; i++) {
		int rnd = rand();
		if (my_tid == 0)
			data[i] = rnd;
		else
			data_check[i] = 0;
	}

	/* sender creates endpoint */
	if (my_tid == 0) {
		rc = lcp_ep_create(ctx, &ep, dest_uid, 0);
		if (rc != 0) {
			printf("ERROR: create ep\n");
		}
                gateway.request = (struct request) {
                        .buffer = malloc(size),
                        .size   = size * sizeof(int),
                        .completion_flag = 0,
                };
                hdr = (hdr_request_t) {
                        .magic = magic,
                        .size  = size *sizeof(int),
                };
	} else {
                gateway.request = (struct request) {
                        .completion_flag = 0,
                };
        }

	/* send/recv */
	if (my_tid == 0) {
                lcp_request_param_t param = {
                        .datatype     = LCP_DATATYPE_CONTIGUOUS,
                        .on_am_completion = recv_completion,
                        .user_request = &gateway.request,
                        .flags        = LCP_REQUEST_AM_CALLBACK
                };
                rc = lcp_am_send_nb(ep, task, dest_tid, AM_ID, &hdr, sizeof(hdr), 
                                    (void *)data, size*sizeof(int), &param);
		if (rc != LCP_SUCCESS) {
			printf("ERROR: send\n");
		}
	}

	/* progress communication */
	while (gateway.request.completion_flag == 0) {
		usleep(10);
		lcp_progress(ctx);
	}

	/* receiver perform data check */
	if (my_tid == 1) {	
		for (i=0; i<(int)size; i++) {
			if (data[i] != data_check[i]) {
                                printf("%d\n", i);
				check = 0;
                                break;
			}
		}
	}
        if (!check)
                printf("ERROR: data check\n");

	mpc_launch_pmi_barrier();

err:
        free(data);
        free(data_check);

	return check ? 0 : 1;
}
