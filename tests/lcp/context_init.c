#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <lcp.h>
#include <mpc_conf.h>
#include <mpc_lowcomm.h>

int main(int argc, char** argv) {
	int rc;
	lcp_context_h ctx;

	mpc_conf_root_config_init("mpcframework");
	_mpc_lowcomm_config_register();
	_mpc_lowcomm_config_validate();
	mpc_conf_root_config_load_env_all();

	rc = lcp_context_create(&ctx, 0);
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
