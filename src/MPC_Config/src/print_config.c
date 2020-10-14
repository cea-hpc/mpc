#include <mpc.h>
#include <mpc_conf.h>
#include <string.h>



mpc_conf_output_type_t       output_format = MPC_CONF_FORMAT_XML;
int did_print = 0;

int parse_arg(char *arg)
{
	if(!strcmp(arg, "--xml") )
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

int main(int argc, char **argv)
{
	if(parse_args(argc, argv) )
	{
		return 1;
	}


	if(!did_print)
	{
		/* No argument print all */
		return mpc_conf_root_config_print(output_format);
	}

	return 0;
}
