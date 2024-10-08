#include "lowcomm_config.h"
#include <stdio.h>
#include <stdlib.h>

#include <lcp.h>
#include <mpc_conf.h>
#include <mpc_lowcomm.h>

int main() {
	int rc;
	lcp_context_h ctx;
        lcp_context_param_t param;

	mpc_conf_root_config_init("mpcframework");
	_mpc_lowcomm_config_register();
	_mpc_lowcomm_config_validate();
	mpc_conf_root_config_load_env_all();

        param = (lcp_context_param_t) {
                .field_mask = LCP_CONTEXT_PROCESS_UID |
                        LCP_CONTEXT_NUM_TASKS         |
                        LCP_CONTEXT_DATATYPE_OPS,
                .process_uid = 0,
                .num_tasks = 1,
                .dt_ops = {
                        .pack   = NULL,
                        .unpack = NULL,
                },
        };
	rc = lcp_context_create(&ctx, &param);
	if (rc != 0) {
		printf("ERROR: create context\n");
		goto err;
	}

	rc = lcp_context_fini(ctx);
	if (rc != 0) {
		printf("ERROR: fini context\n");
	}

        printf("SUCCESS");
err:
	return rc;
}
