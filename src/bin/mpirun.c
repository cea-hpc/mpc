#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>



#include <mpc_config.h>

void display_help()
{
	printf("mpirun MPC\n"
	       "USAGE: mpirun [OPTION]... [PROGRAM]...\n\n"
	       "\t-n|-np\t\tNumber of processes to launch\n"
	       "\t-c\t\tNumber of corees per process\n"
	       "\t--verbose\tEnable debug messages.\n"
	       "\n");
}

void display_version()
{
	printf("mpirun (MPC framework) %s\n", MPC_VERSION_STRING);

}

#define COMMAND_SIZE 4*1024

static int verbose_flag = 0;

int main(int argc, char **argv)
{
	int nb_process = 0;
	int nb_cores   = 0;
	int c;
	int done = 0;

	int i;

	for(i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-np") )
		{
			argv[i] = "-n";
		}
	}

	while(1)
	{
		static struct option long_options[] =
		{
			/* These options set a flag. */
			{ "version" , no_argument       , 0 , 'V'} ,
			{ "verbose" , no_argument       , 0 , 'v'} ,
			{ "brief"   , no_argument       , 0 , 'v'} ,
			{ "np"      , required_argument , 0 , 'n'} ,
			{ "cores"   , required_argument , 0 , 'c'} ,
			{ "help"    , no_argument       , 0 , 'h'} ,
			{ 0         , 0                 , 0 , 0}
		};
		/* getopt_long stores the option index here. */
		int option_index = 0;

		c = getopt_long(argc, argv, "+hn:c:V", long_options, &option_index);

		/* Detect the end of the options. */
		if(c == -1)
		{
			break;
		}

		switch(c)
		{
			case 'n':
				nb_process = atoi(optarg);
				break;

			case 'c':
				nb_cores = atoi(optarg);
				break;

			case 'h':
				display_help();
				return 0;
				break;
			case 'V':
				display_version();
				return 0;
				break;
			case 'v':
				verbose_flag=1;
				break;
			default:
				done = 1;
				break;
		}

		/* Encountered unknown arg */
		if(done)
		{
			break;
		}
	}

    /* This is passed to strncat and we need the NULL terminator */
	char *command = malloc((1 + COMMAND_SIZE) * sizeof(char) );

	if(!command)
	{
		perror("malloc");
		return 1;
	}

	command[0] = '\0';

	char tmp[128];

	strncat(command, MPC_PREFIX_PATH "/bin/mpcrun ", COMMAND_SIZE);

	char *nodes = getenv("SLURM_JOB_NUM_NODES");

	if(nodes)
	{
		snprintf(tmp, 128, " -N=%s ", nodes);
		strncat(command, tmp, COMMAND_SIZE);
	}

	if(0 < nb_process)
	{
		snprintf(tmp, 128, " -p=%d ", nb_process);
		strncat(command, tmp, COMMAND_SIZE);
	}

	if(0 < nb_process)
	{
		snprintf(tmp, 128, "-n=%d ", nb_process);
		strncat(command, tmp, COMMAND_SIZE);
	}

	if(0 < nb_cores)
	{
		snprintf(tmp, 128, "-c=%d ", nb_cores);
		strncat(command, tmp, COMMAND_SIZE);
	}

	if(verbose_flag)
	{
		strncat(command, " -vvv", COMMAND_SIZE);
	}

	for(i = optind; i < argc; i++)
	{
		strncat(command, " ",COMMAND_SIZE);
		strncat(command, argv[i], COMMAND_SIZE);
	}

	int ret = system(command);
	free(command);
	
	/* failed to start the child process */
	if(ret == -1)
		return errno;
	/* failed to spawn the shell within the child process */
	else if(ret == 127)
	{

	}
	else /* sycalls succeds, contains the return code (among others)*/
	{
		ret = WEXITSTATUS(ret);
	}

	return ret;
}
