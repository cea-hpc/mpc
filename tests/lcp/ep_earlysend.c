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
	int rc;
	lcp_context_h ctx;
	lcp_ep_h ep;
	mpc_lowcomm_set_uid_t suid;
	mpc_lowcomm_peer_uid_t src_uid, dest_uid, my_uid;

	int data1;

	/* load default config */
	mpc_conf_root_config_init("mpcframework");
	_mpc_lowcomm_config_register();
	_mpc_lowcomm_config_validate();
	mpc_conf_root_config_load_env_all();

	/* init pmi */
	rc = mpc_launch_pmi_init();
	if (rc != 0) {
		printf("ERROR: init pmi\n");
		goto err;
	}

	/* setup monitor: necessary to exchange set uid */
	rc = _mpc_lowcomm_monitor_setup();
	if (rc != 0) {
		printf("ERROR: monitor setup\n");
		goto pmi_fini;
	}

	/* create communication context */
	rc = lcp_context_create(&ctx, 0);
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
	my_uid = mpc_lowcomm_monitor_get_uid();
	src_uid = mpc_lowcomm_monitor_uid_of(suid, 0);
	dest_uid = mpc_lowcomm_monitor_uid_of(suid, 1);

	/* sender creates endpoint */
	if (mpc_lowcomm_peer_get_rank(my_uid) == 0) {
		rc = lcp_ep_create(ctx, &ep, dest_uid, 0);
		if (rc != 0) {
			printf("ERROR: create ep\n");
		}
	}

	/* init mpc request */
	mpc_lowcomm_request_t req1 = {
		.header = {
			.communicator_id = mpc_lowcomm_get_comm_world_id(),
			.destination = dest_uid,
			.destination_task = (int)dest_uid,
			.msg_size = sizeof(data1),
			.source = src_uid,
			.source_task = (int)src_uid,
			.message_tag = 0,
		},
		.completion_flag = 0,
		.request_completion_fn = lowcomm_request_complete
	};                          

	/* send/recv */
	if (mpc_lowcomm_peer_get_rank(my_uid) == 0) {
		data1 = 42;
		rc = lcp_send(ep, &req1, &data1, 0);
		if (rc != 0) {
			printf("ERROR: send\n");
		}
	} else {
		/* wait to force early send */
		usleep(10);
		rc = lcp_recv(ctx, &req1, &data1);
		if (rc != 0) {
			printf("ERROR: recv\n");
		}
	}

	/* progress communication */
	while (req1.completion_flag == 0) {
		usleep(10);
		lcp_progress(ctx);
		if (mpc_lowcomm_peer_get_rank(my_uid) == 1) {
			printf("recv1=%d\n", data1);
		}
	}

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
	return rc;
}
