#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <lcp.h>
#include <mpc_lowcomm.h>
#include <mpc_launch_pmi.h>
#include <mpc_lowcomm_monitor.h>
#include <mpc_lowcomm_communicator.h>

int main(int argc, char** argv) {
	int rc;
	lcp_context_h ctx;
        lcp_context_param_t param;
	lcp_ep_h ep;
	mpc_lowcomm_set_uid_t suid;
        int my_tid, src_tid, dest_tid;
	mpc_lowcomm_peer_uid_t src_uid, dest_uid, my_uid;

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

	rc = _mpc_lowcomm_monitor_setup();
	if (rc != 0) {
		printf("ERROR: monitor setup\n");
		goto pmi_fini;
	}
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

        param = (lcp_context_param_t) {
                .flags = LCP_CONTEXT_PROCESS_UID,
                .process_uid = mpc_lowcomm_monitor_get_uid()
        };
	rc = lcp_context_create(&ctx, 0);
	if (rc != 0) {
		printf("ERROR: create context\n");
		goto monitor_fini;
	}

	//NOTE: pmi barrier needed to commit pmi registration
	//      of rails.
	mpc_launch_pmi_barrier();

	if (my_uid == 0) {
		rc = lcp_ep_create(ctx, &ep, dest_uid, 0);
		if (rc != 0) {
			printf("ERROR: create ep\n");
		}
	}

	mpc_launch_pmi_barrier();

	if (my_uid == 0) {
		rc = lcp_ep_create(ctx, &ep, dest_uid, 0);
		if (rc != 0) {
			printf("ERROR: create ep\n");
		}
	}

	mpc_launch_pmi_barrier();

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
