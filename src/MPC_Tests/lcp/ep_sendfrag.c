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

int lowcomm_request_complete(mpc_lowcomm_request_t *req){
	req->completion_flag = 1;
	return 0;
}

int main(int argc, char** argv) {
	int rc, i;
	lcp_context_h ctx;
        lcp_context_param_t param;
        lcp_task_h task;
	lcp_ep_h ep;
	mpc_lowcomm_set_uid_t suid;
        int my_tid, src_tid, dest_tid;
	mpc_lowcomm_peer_uid_t src_uid, dest_uid, my_uid;
	int check = 1;

	if (argc != 2) {
		printf("error: one argument needed, [size]");
		return 1;
	}
	size_t size = atoi(argv[1]);

	/* uid must be set before initializing data */
	int *data = malloc(size * sizeof(int));
	memset(data, 0, size * sizeof(int));
	int *data_check = malloc(size * sizeof(int));
	memset(data_check, 0, size * sizeof(int));

	/* load default config */
	mpc_conf_root_config_init("mpcframework");
	_mpc_lowcomm_config_register();
	_mpc_lowcomm_config_validate();
	/* load configuration set up by env variable */
	mpc_conf_root_config_load_env_all();

	/* init pmi */
	rc = mpc_launch_pmi_init();
	if (rc != 0) {
		printf("ERROR: init pmi\n");
		goto err;
	}

	//TODO: assert(num_proc == 2)

	/* setup monitor: necessary to exchange set uid */
	rc = _mpc_lowcomm_monitor_setup();
	if (rc != 0) {
		printf("ERROR: monitor setup\n");
		goto pmi_fini;
	}

        param = (lcp_context_param_t) {
                .flags = LCP_CONTEXT_PROCESS_UID,
                .process_uid = mpc_lowcomm_monitor_get_uid()
        };
	/* create communication context */
	rc = lcp_context_create(&ctx, &param);
	if (rc != 0) {
		printf("ERROR: create context\n");
		goto monitor_fini;
	}

	/* pmi barrier needed to commit pmi rail registration */
	mpc_launch_pmi_barrier();

	/* init world and self comm */
	_mpc_lowcomm_communicator_init();

	/* get uids */
	suid = mpc_lowcomm_monitor_get_gid();
        my_tid = mpc_lowcomm_get_rank();

        if (my_tid == 0) {
                src_tid  = mpc_lowcomm_get_rank();
                src_uid  = mpc_lowcomm_monitor_get_uid();
                dest_tid = 1;
                dest_uid = mpc_lowcomm_monitor_uid_of(suid, 1);
        } else {
                src_tid = 0;
                src_uid = mpc_lowcomm_monitor_uid_of(suid, 0);
                dest_uid = mpc_lowcomm_monitor_get_uid();
                dest_tid  = mpc_lowcomm_get_rank();
        }

        rc = lcp_task_create(ctx, my_tid, &task);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                printf("ERROR: create task");
                goto monitor_fini;
        }

	/* init data */
	srand(0);
	for (i=0; i<size; i++) {
		int rnd = rand();
		if (my_tid == 0)
			data[i] = rnd;
		else
			data_check[i] = rnd;
	}

	/* sender creates endpoint */
	if (my_tid == 0) {
		rc = lcp_ep_create(ctx, &ep, dest_uid, 0);
		if (rc != 0) {
			printf("ERROR: create ep\n");
		}
	}

	/* init mpc request */
	mpc_lowcomm_request_t req = {
		.header = {
			.communicator_id = mpc_lowcomm_get_comm_world_id(),
			.destination = dest_uid,
			.destination_task = dest_tid,
			.msg_size = size*sizeof(int),
			.source = src_uid,
			.source_task = src_tid,
			.message_tag = 0,
		},
		.completion_flag = 0,
		.request_completion_fn = lowcomm_request_complete
	};

	/* send/recv */
	if (my_tid == 0) {
                lcp_request_param_t param = {
                        .datatype  = LCP_DATATYPE_CONTIGUOUS,
                        .recv_info = &req.recv_info,
                };
		rc = lcp_tag_send_nb(ep, task, (void *)data, size*sizeof(int), &req, &param);
		if (rc != 0) {
			printf("ERROR: send\n");
		}
	} else {
                lcp_request_param_t param = {
                        .datatype  = LCP_DATATYPE_CONTIGUOUS,
                        .recv_info = &req.recv_info
                };
		rc = lcp_tag_recv_nb(task, (void *)data, size*sizeof(int), &req, &param);
		if (rc != 0) {
			printf("ERROR: recv\n");
		}
	}

	/* progress communication */
	while (req.completion_flag == 0) {
		usleep(10);
		lcp_progress(ctx);
	}

        if (req.truncated) {
                printf("WARNING: truncated\n");
                goto no_check;
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

no_check:
	mpc_launch_pmi_barrier();

	_mpc_lowcomm_communicator_release();

	rc = lcp_context_fini(ctx);
	if (rc != 0) {
		printf("ERROR: fini context\n");
	}
monitor_fini:
	rc = _mpc_lowcomm_monitor_teardown();
	if (rc != 0) {
		printf("ERROR: fini monitor\n");
	}
pmi_fini:
	rc = mpc_launch_pmi_finalize();
	if (rc != 0) {
		printf("ERROR: fini pmi\n");
	}
err:
        free(data);
        free(data_check);

	return check ? 0 : 1;
}
