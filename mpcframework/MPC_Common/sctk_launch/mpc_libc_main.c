#define _GNU_SOURCE 1
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
/* #   - Julien Adam <adamj@paratools.com>                                # */
/* #                                                                      # */
/* ######################################################################## */
#ifdef ENABLE_HOOK_MAIN
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/wait.h>

/* main() prototype */
typedef int(*main_proto_env_t)(int, char**, char**);
typedef int(*main_proto_t)(int, char**);
/* glibc prototype */
typedef int(*libc_proto_t)(main_proto_env_t, int, char**, void (*)(void), void (*) (void), void (*)(void),void (*));

#ifndef SCTK_LIB_MODE
#define MAX_SIZE 4096
#define WRAP_ALL_CONSTANT "__mpc_wrap_all_mains"

int sctk_unified_main(int argc, char** argv, char ** env)
{
	/* if C -> call main_c
	 * if Fortran -> call main_fortran and set bool to 1
	 * if C++ ?
	 */
	char *tmp = NULL, *name = NULL;
	tmp=malloc(sizeof(char) * MAX_SIZE);
	snprintf(tmp, MAX_SIZE, "strings `which %s` | \\grep -o \"^MAIN__$\" > /dev/null", argv[0]);
	int ret = system(tmp);
	if(ret == -1)
	{
		perror("System at fork");
		abort();
	}

	if(WIFEXITED(ret) && WEXITSTATUS(ret) == 0)
	{
		/*fprintf(stderr, "Fortran !!\n");*/
		name = "main_fortran";
	}
	else
	{
		/*fprintf(stderr, "C/C++ !!\n");*/
		name = "main_c";

	}

	main_proto_t handler = dlsym(RTLD_DEFAULT, name);
	if(!handler)
	{
		fprintf(stderr, "Handler MPC not found !!\n");
		abort();
	}

	/*fprintf(stderr, "handler = %p -> %s\n", handler, name);*/
	ret = handler(argc, argv);
	return ret;
}


short sctk_check_main(char * exe_name)
{
	short res = 0;
	char *tmp = NULL, *old = NULL, *hpc_syms = NULL;
	
	hpc_syms = getenv("MPC_SYMBOL_REGEX");
	/* if not defined, no binary will match (infinite loop prevention)*/
	if(!hpc_syms)
	{
		return 0;
	}

	/* temporarily unset LD_PRELOAD
	 * This avoids to preload hook library for popen()
	 */
	old = getenv("LD_PRELOAD");
	unsetenv("LD_PRELOAD");
	
	tmp = (char*)calloc(MAX_SIZE, sizeof(char));
	/* an '\' is added before the grep command for using the native commmand (no aliasing) */
	snprintf(tmp,MAX_SIZE, "nm `which %s` 2>/dev/null | \\egrep -oi \"^[a-fA-F0-9 ]* [a-zA-Z] (%s)(_)?(@@.*)*$\" > /dev/null",exe_name, hpc_syms);
	/* retrieve the main() address */
	int ret = system(tmp);
	if(ret == -1)
	{
		perror("system at fork:");
		abort();
	}

	res = (WIFEXITED(ret) && WEXITSTATUS(ret) == 0);
	
	if(old)
	{
		setenv("LD_PRELOAD", old, 0);
	}

	free(tmp);
	return res;
}

#ifdef HAVE_ENVIRON_VAR
main_proto_env_t real_main = NULL;
int mpc_user_main__(int argc, char ** argv, char ** env)
{
	return real_main(argc, argv, env);
}
#else
main_proto_t real_main = NULL;
int mpc_user_main__(int argc, char ** argv)
{
	return real_main(argc, argv);
}
#endif

void sctk_cleanup_preload()
{
	char *old = NULL, *cur = NULL, *new = NULL;
	size_t n;

	old = getenv("LD_PRELOAD");
	n = strlen(old) + 1;

	new = calloc(n, sizeof(char));

	cur = strtok(old, " ");
	while(cur != NULL)
	{
		if(strstr(cur, "libmpc_hookmain.so") == NULL)
		{
			strcat(new, cur);
			strcat(new, " ");
		}

		cur = strtok(NULL, " ");
	}


	if(new[0] != '\0')
	{
		setenv("LD_PRELOAD", new, 1);
	}
}

int __libc_start_main(main_proto_env_t first_main, int argc, char * * ubp_av, void (*init) (void), void (*fini) (void), void (*rtld_fini) (void), void (* stack_end))
{
	main_proto_env_t to_execute = NULL;
	char *user_bin = NULL;

	/* retrieve the libc call */
	libc_proto_t real_libc_main = dlsym(RTLD_NEXT, "__libc_start_main");
	if(!real_libc_main)
	{
		fprintf(stderr, "Things gone bad. We did not found the real __libc_start_main() !\n");
		abort();
	}

	user_bin = getenv("MPC_USER_BINARY");
	/* here, we made two checks. To hook the main, either one of these conditions have to be true:
	 * - The user does not set any preference and we check if the current binary contains at least
	 *   one symbol matching with the regex provided by getenv(MPC_SYMBOL_REGEX) (".*" by default, which means
	 *   that any symbol will match.
	 * - OR the user set a specific binary to hook and we check if the current binary is the chosen one.
	 */
	if(user_bin)
	{
		if(strstr(ubp_av[0], user_bin) != NULL || strcmp(user_bin, WRAP_ALL_CONSTANT) == 0)
		{
			/*fprintf(stderr, "%s considered by user\n", ubp_av[0] );*/
			to_execute = sctk_unified_main;
		}
		else
		{
			/*fprintf(stderr, "%s not considered by user\n", ubp_av[0]);*/
			to_execute = first_main;
		}
	}
	else
	{
		if(sctk_check_main(ubp_av[0]))
		{
			/*fprintf(stderr, "%s considered by detection\n", ubp_av[0] );*/
			to_execute = sctk_unified_main;
		}
		else
		{
			/*fprintf(stderr, "%s not considered by detection\n", ubp_av[0]);*/
			to_execute = first_main;
		}
	}

	if(to_execute != first_main)
	{
		sctk_cleanup_preload();
	}
	real_main = first_main;
	return real_libc_main(to_execute, argc, ubp_av, init, fini, rtld_fini, stack_end);
}



#endif
#endif /* SCTK_LIB_MODE */
