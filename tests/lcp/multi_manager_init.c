#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <lcp.h>
#include <mpc_conf.h>
#include <mpc_lowcomm.h>
#include <mpc_launch_pmi.h>
#include <mpc_lowcomm_monitor.h>

int main(int argc, char** argv) {
	int rc;
	lcp_context_h ctx;
        lcp_context_param_t ctx_params;
        lcp_manager_h mngr1;
        lcp_manager_h mngr2;
        lcp_manager_param_t mngr_params;

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
		goto err;
	}


	/* create communication context */
        ctx_params = (lcp_context_param_t) {
                .flags = LCP_CONTEXT_PROCESS_UID,
                .process_uid = 0 
        };
	rc = lcp_context_create(&ctx, &ctx_params);
	if (rc != 0) {
		printf("ERROR: create context\n");
		goto err;
	}

        mngr_params = (lcp_manager_param_t) {
                .field_mask = LCP_MANAGER_ESTIMATED_EPS |
                        LCP_MANAGER_NUM_TASKS,
                .estimated_eps = 2,
                .num_tasks = 1,
        };
        rc = lcp_manager_create(ctx, &mngr1, &mngr_params);
        if (rc != 0) {
                printf("ERROR: create manager\n");
                goto err;
        }

        rc = lcp_manager_create(ctx, &mngr2, &mngr_params);
        if (rc != 0) {
                printf("ERROR: create manager\n");
                goto err;
        }

        rc = lcp_manager_fini(mngr1);
        if (rc != 0) {
                printf("ERROR: fini manager\n");
                goto err;
        }

        rc = lcp_manager_fini(mngr2);
        if (rc != 0) {
                printf("ERROR: fini manager\n");
                goto err;
        }

	rc = lcp_context_fini(ctx);
	if (rc != 0) {
		printf("ERROR: fini context\n");
                goto err;
	}

	rc = _mpc_lowcomm_monitor_teardown();
	if (rc != 0) {
		printf("ERROR: fini monitor\n");
                goto err;
	}

	rc = mpc_launch_pmi_finalize();
	if (rc != 0) {
		printf("ERROR: fini pmi\n");
                goto err;
	}


        printf("SUCCESS");
err:
	return rc;
}
