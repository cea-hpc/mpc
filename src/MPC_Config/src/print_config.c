#include <mpc.h>
#include <mpc_config.h>
#include <mpc_conf.h>
#include <string.h>
#include <stdlib.h>

mpc_conf_output_type_t       output_format = MPC_CONF_FORMAT_XML;
int did_print = 0;

void print_help(void)
{
	fprintf(stderr, "mpc_print_config [--FORMAT] [CONF NODES] ...\n\n");
	fprintf(stderr, "Multiple formats and conf nodes can be passed, they are read in order\n");
	fprintf(stderr, "Separator is '.'\n\n");
	fprintf(stderr, "\t --help Show this help\n");
	fprintf(stderr, "\nFormats (default is XML):\n\n");
	fprintf(stderr, "\t --xml output in XML\n");
	fprintf(stderr, "\t --ini output in ini\n");
	fprintf(stderr, "\t --conf output to environment\n");
	fprintf(stderr, "\nExamples :\n\n");
	fprintf(stderr, "\t mpc_print_config --conf foo.bar --xml foo.bar\n");
	fprintf(stderr, "\n");
}


void print_cli_options(void)
{
	/* This prints all netowkring options */

#if 0
	Configured CLI switches for network configurations:

		- ib:
			* Rail 0: shm_mpi
			* Rail 1: ib_mpi
		- opa:
			* Rail 0: shm_mpi
			* Rail 1: opa_mpi
		- tcponly:
			* Rail 0: tcp_mpi
		- tcp:
			* Rail 0: shm_mpi
			* Rail 1: tcp_mpi
		- multirail_tcp:
			* Rail 0: shm_mpi
			* Rail 1: tcp_large
		- portals:
			* Rail 0: portals_mpi
		- shm:
			* Rail 0: shm_mpi
		- default:
			* Rail 0: shm_mpi
			* Rail 1: ib_mpi
#endif
	mpc_conf_config_type_elem_t * def = mpc_conf_root_config_get_sep("mpcframework.lowcomm.networking.cli.default", ".");
	char * default_cli = mpc_conf_type_elem_get_as_string(def);

	printf("\tConfigured CLI switches for network configurations (default: %s):\n\n", default_cli);


	mpc_conf_config_type_elem_t * options = mpc_conf_root_config_get_sep("mpcframework.lowcomm.networking.cli.options", ".");
	mpc_conf_config_type_t * cli = mpc_conf_config_type_elem_get_inner(options);

	int len = mpc_conf_config_type_count(cli);

	unsigned int i;

	for(i = 0 ; i < len; i++ )
	{
		mpc_conf_config_type_t * param = mpc_conf_config_type_elem_get_inner(mpc_conf_config_type_nth(cli, i));

		printf("\t- %s:\n", param->name);

		int param_len = mpc_conf_config_type_count(param);

		unsigned int j;
		for(j = 0 ; j < param_len; j++ )
		{
			mpc_conf_config_type_elem_t * rail = mpc_conf_config_type_nth(param, j);
			printf("\t\t* %s\n", mpc_conf_type_elem_get_as_string(rail));
		}

		printf("\n");
	}

}




int parse_arg(char *arg)
{
	if(!strcmp(arg, "--cli"))
	{
		print_cli_options();
		exit(0);
	}
	else if(!strcmp(arg, "--xml") )
	{
		output_format = MPC_CONF_FORMAT_XML;
	}
	else if(!strcmp(arg, "--conf") )
	{
		output_format = MPC_CONF_FORMAT_CONF;
	}
	else if(!strcmp(arg, "--ini") )
	{
		output_format = MPC_CONF_FORMAT_INI;
	}
	else if(!strcmp(arg, "--help") ||  !strcmp(arg, "-h"))
	{
		print_help();
		return 1;
	}
	else
	{
		did_print = 1;
		mpc_conf_config_type_elem_t * target_elem = mpc_conf_root_config_get_sep(arg, ".");

		if(!target_elem)
		{
			fprintf(stderr, "Could not locate '%s' in configuration\n", arg);
			return 1;
		}

		if( mpc_conf_config_type_elem_print(target_elem, output_format) )
		{
			return 1;
		}
	}


	return 0;
}

int parse_args(int argc, char **argv)
{
	int i;

	for(i = 1; i < argc; i++)
	{
		if(parse_arg(argv[i]) )
		{
			return 1;
		}
	}

	return 0;
}

#ifdef MPC_Lowcomm
	void mpc_lowcomm_init();
	void mpc_lowcomm_release();
#endif

int main(int argc, char **argv)
{
#ifdef MPC_Lowcomm
	 mpc_lowcomm_init();
	int my_rank = mpc_lowcomm_get_rank();

	if(my_rank)
	{
		/* Only 0 prints */
		return 0;
	}
#endif



	int ret = 0;

	if(parse_args(argc, argv) )
	{
		ret = 1;
	}

	if( (ret  == 0) && !did_print)
	{
		/* No argument print all */
		ret = mpc_conf_root_config_print(output_format);
	}

#ifdef MPC_Lowcomm
	mpc_lowcomm_release();
#endif

	return ret;
}
