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
	lcp_ep_h ep;
	mpc_lowcomm_set_uid_t suid;
	mpc_lowcomm_peer_uid_t my_uid, dest_uid;

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
	suid = mpc_lowcomm_monitor_get_gid();
	my_uid = mpc_lowcomm_monitor_get_uid();	

	rc = lcp_context_create(&ctx, 0);
	if (rc != 0) {
		printf("ERROR: create context\n");
		goto monitor_fini;
	}

	//NOTE: pmi barrier needed to commit pmi registration
	//      of rails.
	mpc_launch_pmi_barrier();

	if (mpc_lowcomm_peer_get_rank(my_uid) == 0) {
		dest_uid = mpc_lowcomm_monitor_uid_of(suid, 1);
		rc = lcp_ep_create(ctx, &ep, dest_uid, 0);
		if (rc != 0) {
			printf("ERROR: create ep\n");
		}
	}

	mpc_launch_pmi_barrier();

	if (mpc_lowcomm_peer_get_rank(my_uid) == 0) {
		dest_uid = mpc_lowcomm_monitor_uid_of(suid, 1);
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
