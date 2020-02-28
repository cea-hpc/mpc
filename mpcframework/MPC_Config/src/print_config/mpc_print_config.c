/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - VALAT Sebastien sebastien.valat@cea.fr                           # */
/* #                                                                      # */
/* ######################################################################## */

/********************************* INCLUDES *********************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sctk_debug.h>
#include <mpc_runtime_config.h>
#include <mpc_config_struct.h>

#include <runtime_config_mapper.h>
#include <runtime_config_walk.h>

#include "mpc_print_config_sh.h"
#include "mpc_print_config_xml.h"

/********************************** ENUM ************************************/
/** Output mode available to print the config. **/
enum output_mode {
	/** Output an XML file which can be reuse as input configuration file. **/
	OUTPUT_MODE_XML,
	/** Print a more human readable text format. **/
	OUTPUT_MODE_TEXT,
	/** Display only the entries for launcher and format it in shell variables format. **/
	OUTPUT_MODE_LAUNCHER
};

/******************************** STRUCTURE *********************************/
/** Structure to store parsing status of all available options for the command. **/
struct command_options {
	/** Setup the output mode (--xml, --launcher, --text). **/
	enum output_mode mode;
	/** Setup the system file to load (--system). **/
	char * system_file;
	/** Setup the user file to load (--user). **/
	char * user_file;
	/** Setup manual selection of config profiles (--profiles). **/
	char * user_profiles;
	/** Setup manual path to XSD file (--schema). **/
	char * schema;
	/** Request the help of the command (--help). **/
	bool help;
  /** Disable the loading of config files (--nofile). **/
  bool nofile;
  /** Display all the available profile names. **/
  bool list_profiles;
};

/********************************  CONSTS  **********************************/
/**
 * Declare the global symbol containing the meta description for mapping XML to C struct.
 * The content of this table is generated by XSLT files at build time.
**/
extern const struct sctk_runtime_config_entry_meta sctk_runtime_config_db[];
/** Text for --help **/
static const char * cst_help_message = "\
mpc_print_config [--text|--xml|--launcher|--help]\n\
\n\
Options :\n\
  --text            : Display the current configuration in simple text format.\n\
  --xml             : Display the current configuration in MPC config XML format.\n\
  --launcher        : Display launcher options in bash variable format.\n\
  --system={file}   : Override the system configuration file. Use none to disable.\n\
  --user={file}     : Override the user configuration file. Use none to disable.\n\
  --nofile          : Only use the internal default values.\n\
  --profiles={list} : Manually enable some profiles (comma separated).\n\
  --schema={file}   : Override the default path to XML schema for validation.\n\
\n\
You can also influence the loaded files with environment variables :\n\
  - MPC_SYSTEM_CONFIG   : System configuration file (" MPC_PREFIX_PATH "/share/mpc/config.xml)\n\
  - MPC_USER_CONFIG     : Application configuration file (disabled)\n\
  - MPC_DISABLE_CONFIG  : Disable loading of configuration files if setup to 1.\n\
  - MPC_USER_PROFILES   : Equivalent to --profiles.\n";

/********************************* FUNCTION *********************************/
/**
 * Display the configuration for the launcher script (mpcrun) in sh format.
 * It will only display the section config.modules.launcher.
**/
void display_launcher(const struct sctk_runtime_config * config)
{
	display_tree_sh(sctk_runtime_config_db, "config_launcher","sctk_runtime_config_struct_launcher", (void*)&config->modules.launcher);

	/* Now Generate list of CLI configurations */
	char localbuff[500];
	char list_of_cli_options[4096];
	list_of_cli_options[0] = '\0';

	int i;

	for(i = 0 ; i < config->networks.cli_options_size; i++)
	{

		snprintf(localbuff, 500, "%s%s", config->networks.cli_options[i].name, (i!=config->networks.cli_options_size-1)?" ":"");
		strncat(list_of_cli_options, localbuff, 4096);
	}

	printf("NETWORKING_CLI_OPTION_LIST=\"%s\"\n", list_of_cli_options);

	/* Now generate for each rail the list of internal configurations */


	for(i = 0 ; i < config->networks.cli_options_size; i++)
	{
		struct sctk_runtime_config_struct_net_cli_option * cli = &config->networks.cli_options[i];
		char varname[500];
		snprintf(varname, 500, "NETWORKING_CLI_%s_RAILS", cli->name);

		char rail_list_tmp[1024];
		rail_list_tmp[0] = '\0';

		int j;

		for(j=0; j < cli->rails_size; j++)
		{
			snprintf(localbuff, 500, "%s%s", cli->rails[j], (j != cli->rails_size-1)?" ":"");
			strncat(rail_list_tmp, localbuff, 1024);
		}

		printf("%s=\"%s\"\n", varname, rail_list_tmp);
	}


}


/********************************* FUNCTION *********************************/
/**
 * Standard display of the struct to see the complete tree
**/
void display_all(__UNUSED__ const struct sctk_runtime_config * config)
{
	sctk_runtime_config_runtime_display();
}

/********************************* FUNCTION *********************************/
/**
 * Standard display of the struct to see the complete tree
**/
void display_xml(const struct sctk_runtime_config * config)
{
	mpc_print_config_xml(sctk_runtime_config_db,"config","sctk_runtime_config",(void*)config);
}

/********************************* FUNCTION *********************************/
/**
 * Display help for command arguments.
**/
void display_help(void)
{
	puts(cst_help_message);
}

/********************************* FUNCTION *********************************/
void init_default_options(struct command_options * options)
{
	/* errors */
	assert(options != NULL);

	/* default values */
	options->help          = false;
	options->mode          = OUTPUT_MODE_TEXT;
	options->system_file   = NULL;
	options->user_file     = NULL;
	options->user_profiles = NULL;
	options->schema        = NULL;
	options->nofile        = false;
	options->list_profiles = false;
}

/********************************* FUNCTION *********************************/
/** Function to parse command line arguments. **/
bool parse_args(struct command_options * options,int argc, char ** argv)
{
	/* vars */
	int i;

	/* errors */
	assert(options != NULL);
	assert(argc >= 1);
	assert(argv != NULL);

	/* setup default */
	init_default_options(options);

	/* read args */
	for ( i = 1 ; i < argc ; i++) {
		if (argv[i][0] != '-' || argv[i][1] != '-') {
			/* error */
			fprintf(stderr,"Error : invalid argument %s\n",argv[i]);
			return false;
		} else {
			/* find the good one */
			if (strcmp(argv[i],"--help") == 0) {
			  options->help = true;
			} else if (strcmp(argv[i],"--nofile") == 0) {
			  options->nofile = true;
			} else if (strcmp(argv[i],"--text") == 0) {
			  options->mode = OUTPUT_MODE_TEXT;
			} else if (strcmp(argv[i],"--xml") == 0) {
			  options->mode = OUTPUT_MODE_XML;
			} else if (strcmp(argv[i],"--launcher") == 0) {
			  options->mode = OUTPUT_MODE_LAUNCHER;
			} else if (strncmp(argv[i],"--system=",9) == 0) {
			  options->system_file = strdup(argv[i]+9);
			} else if (strncmp(argv[i],"--user=",7) == 0) {
			  options->user_file = strdup(argv[i]+7);
			} else if (strncmp(argv[i],"--profiles=",11) == 0) {
			  options->user_profiles = strdup(argv[i]+11);
			} else if (strncmp(argv[i],"--schema=",9) == 0) {
        options->schema = strdup(argv[i]+9);
      } else if (strncmp(argv[i],"--list-profiles",15) == 0) {
        options->list_profiles = true;
      } else {
			  fprintf(stderr,"Error : invalid argument %s\n",argv[i]);
			  return false;
			}
		}
	}

	return true;
}

/********************************* FUNCTION *********************************/
void cleanup_options(struct command_options * options)
{
	/* errors */
	assert(options != NULL);

	/* free mem */
	if (options->system_file != NULL)
		free(options->system_file);
	if (options->user_file != NULL)
		free(options->user_file);
	if (options->schema != NULL)
		free(options->schema);
	if (options->user_profiles != NULL)
		free(options->user_profiles);
}

/********************************* FUNCTION *********************************/
int load_mpc_config(const struct command_options * options, const struct sctk_runtime_config ** config )
{
	/* overwrite files */
	if (options->system_file != NULL)
		setenv("MPC_SYSTEM_CONFIG",options->system_file,1);
	if (options->user_file != NULL)
		setenv("MPC_USER_CONFIG",options->user_file,1);
	if (options->nofile)
		setenv("MPC_DISABLE_CONFIG","1",1);
	if (options->user_profiles)
		setenv("MPC_USER_PROFILES",options->user_profiles,1);
	if (options->schema)
		setenv("MPC_CONFIG_SCHEMA",options->schema,1);

	/* load the config */
	*config = sctk_runtime_config_get_checked();

	/* final return */
	return EXIT_SUCCESS;
}

/********************************* FUNCTION *********************************/
int print_mpc_config(const struct command_options * options, const struct sctk_runtime_config ** config)
{
  /* vars */
  int res = EXIT_SUCCESS;

  /* print */
  switch(options->mode) {
    case OUTPUT_MODE_TEXT:
      display_all(*config);;
      break;
    case OUTPUT_MODE_XML:
      display_xml(*config);
      break;
    case OUTPUT_MODE_LAUNCHER:
      display_launcher(*config);
      printf("CONFIG_FILES_VALID=true\n");
      break;
    default:
      fprintf(stderr,"Error : invalid output mode : %d\n",options->mode);
      res = EXIT_FAILURE;
  }

  /* final return */
  return res;
}

/********************************* FUNCTION *********************************/
int print_profiles_names_config(const struct sctk_runtime_config ** config)
{
  /* vars */
  int res = EXIT_SUCCESS;
  int i;

  printf("Names of the available profiles:\n");
  if (! (*config)->number_profiles) {
    printf("\tNo available profiles.\n");
  }
  else {
    for (i = 0; i < (*config)->number_profiles; i ++) {
        printf("\t%d/ %s\n", i + 1, (*config)->profiles_name_list[i]);
      }
  }

  /* final return */
  return res;
}

/********************************* FUNCTION *********************************/
int main(int argc, char ** argv)
{
	/* vars */
	int res = EXIT_SUCCESS;
	struct command_options options;
	const struct sctk_runtime_config *config;

	/** To avoid crash on symbol load when running mpc_print_config (mpc not linked). **/
	sctk_crash_on_symbol_load = false;

	/* parse options */
	if (parse_args(&options,argc,argv)) {
		if (options.help) {
			display_help();
		} else if(options.list_profiles){
		  res = load_mpc_config(&options, &config);
		  print_profiles_names_config(&config);
		}
		else
		{
			res = load_mpc_config(&options, &config);
			res = print_mpc_config(&options, &config);
		}
	} else {
		res = EXIT_FAILURE;
	}

	/* cleanup */
	cleanup_options(&options);

	/* return final status */
	return res;
}

/********************************* FUNCTION *********************************/
/**
 * This is a simply way to avoid link error and avoid to init mpc just to load the config.
**/
int mpc_user_main__(void)
{
	return EXIT_FAILURE;
}
