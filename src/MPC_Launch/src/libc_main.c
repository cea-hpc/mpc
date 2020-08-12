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
#include <mpc_config.h>
#include <mpc_common_debug.h>

#if defined(ENABLE_HOOK_MAIN)

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

/* main() prototype */
typedef int (*main_proto_env_t)(int, char **, char **);
typedef int (*main_proto_t)(int, char **);
/* glibc prototype */
typedef int (*libc_proto_t)(main_proto_env_t, int, char **, void (*)(void),
                            void (*)(void), void (*)(void), void(*));

#define MAX_SIZE 4096
#define WRAP_ALL_CONSTANT "__mpc_wrap_all_mains"

/**
 * unified main, to create one handler to inject later.
 * This helps to create different path for C/C++ and Fortran
 * as their main() signature are different.
 * \param[in] argc the real argc
 * \param[in] argv the real argv
 * \param[in] env the real environ
 * \return the real main() return value
 */
int sctk_unified_main(int argc, char **argv, char **env) {
    /* if C -> call main_c
     * if Fortran -> call main_fortran and set bool to 1
     * if C++ ?
     */
    char *tmp = NULL, *name = NULL;
    tmp = malloc(sizeof(char) * MAX_SIZE);
    snprintf(tmp, MAX_SIZE,
             "strings `which %s` | \\grep -o \"^MAIN__$\" > /dev/null", argv[0]);
    int ret = system(tmp);
    if (ret == -1) {
        perror("System at fork");
        abort();
    }

    name = (WIFEXITED(ret) && WEXITSTATUS(ret) == 0) ? "main_fortran" : "main_c";

    main_proto_t handler = dlsym(RTLD_DEFAULT, name);
    if (!handler) {
        fprintf(stderr, "main() handler for MPC not found !!\n");
        abort();
    }

    ret = handler(argc, argv);
    return ret;
}

/**
 * When the user did not specify a binary to wrap, this function will try to detect it.
 * \param[in] exe_name the argv[0]
 * \return 1 if the current process should be wrapped, 0 otherwise
 */
short sctk_check_main(char *exe_name) {
    short res = 0;
    char *tmp = NULL, *old = NULL, *hpc_syms = NULL;

    hpc_syms = getenv("MPC_SYMBOL_REGEX");
    if (!hpc_syms)
    {
        return 0;
    }

    /* temporarily unset LD_PRELOAD
     * This avoids to preload hook library for popen()
     */
    old = getenv("LD_PRELOAD");
    unsetenv("LD_PRELOAD");

    tmp = (char *)calloc(MAX_SIZE, sizeof(char));
    /* an '\' is added before the grep command for using the native commmand (no
     * aliasing) */
    snprintf(tmp, MAX_SIZE, "nm `which %s` 2>/dev/null | \\egrep -oi "
             "\"^[a-fA-F0-9 ]* [a-zA-Z] (%s)(_)?(@@.*)*$\" > "
             "/dev/null",
             exe_name, hpc_syms);
    /* retrieve the main() address */
    int ret = system(tmp);
    if (ret == -1)
    {
        perror("system at fork:");
        abort();
    }

    res = (WIFEXITED(ret) && WEXITSTATUS(ret) == 0);

    if (old)
    {
        setenv("LD_PRELOAD", old, 0);
    }

    free(tmp);
    return res;
}

#ifdef HAVE_ENVIRON_VAR
main_proto_env_t real_main = NULL;
int mpc_user_main__(int argc, char **argv, char **env) {
    return real_main(argc, argv, env);
}
#else
main_proto_t real_main = NULL;
int mpc_user_main__(int argc, char **argv) {
    return real_main(argc, argv);
}
#endif

/**
 * Purge the LD_PRELOAD env var from ourself, to avoid multiple binary wrapping
 * once the good one was found.
 */
void sctk_cleanup_preload() {
    char *old = NULL, *cur = NULL, *new = NULL;
    size_t n;

    old = getenv("LD_PRELOAD");
    n = strlen(old) + 1;

    new = calloc(n, sizeof(char));

    cur = strtok(old, " ");
    while (cur != NULL) {
        if (strstr(cur, "libmpc_hookmain.so") == NULL) {
            strcat(new, cur);
            strcat(new, " ");
        }

        cur = strtok(NULL, " ");
    }

    if (new[0] != '\0') {
        setenv("LD_PRELOAD", new, 1);
    }
}

/**
 * Called by '_start()' function, this function is inserted before the real LIBC call to reroute the process through MPC if needed.
 * This function just need to alter the first argument transmitted to the real libc call, in order to load MPC before the program.
 * The election of the current process is done by either:
 *   1. MPC_USER_BINARY env var, containing the exact program name.
 *   2. A hint about function names that may be present in the binary (ex: MPI_Init).
 * If succeeded to elect the current program, the LD_PRELOAD is purged from our library to avoid recursion.
 * \param[in] first_main the real main(), as seen by the linker when compiling the application
 * \param[in] argc the real argc
 * \param[in] ubp_av the real argv
 * \param[in] init not used
 * \param[in] fini not used
 * \param[in] rtld_fini not used
 * \param[in] stack_end not used
 * \return the real main() return value
 */
int __libc_start_main(main_proto_env_t first_main, int argc, char **ubp_av,
                      void (*init)(void), void (*fini)(void),
                      void (*rtld_fini)(void), void(*stack_end)) {
    main_proto_env_t to_execute = NULL;
    char *user_bin = NULL;

    /* retrieve the libc call */
    libc_proto_t real_libc_main = dlsym(RTLD_NEXT, "__libc_start_main");
    if (!real_libc_main) {
        fprintf(stderr, "Things went bad. We did not found the real __libc_start_main() !\n");
        abort();
    }

    user_bin = getenv("MPC_USER_BINARY");
    /* here, we made two checks. To hook the main, either one of these conditions
     * have to be true:
     * - The user does not set any preference and we check if the current binary
     * contains at least
     *   one symbol matching with the regex provided by getenv(MPC_SYMBOL_REGEX)
     * (".*" by default, which means
     *   that any symbol will match.
     * - OR the user set a specific binary to hook and we check if the current
     * binary is the chosen one.
     */
    if (user_bin) {
        if (strstr(ubp_av[0], user_bin) != NULL || strcmp(user_bin, WRAP_ALL_CONSTANT) == 0)
        {
            to_execute = sctk_unified_main;
        }
        else
        {
            to_execute = first_main;
        }
    }
    else
    {
        if (sctk_check_main(ubp_av[0]))
        {
            to_execute = sctk_unified_main;
        }
        else
        {
            to_execute = first_main;
        }
    }

    if (to_execute != first_main)
    {
        sctk_cleanup_preload();
    }
#ifdef HAVE_ENVIRON_VAR
    real_main = first_main;
#else
    real_main = (main_proto_t)first_main;
#endif
    return real_libc_main(to_execute, argc, ubp_av, init, fini, rtld_fini, stack_end);
}

#endif /* ENABLE_HOOK_MAIN */
