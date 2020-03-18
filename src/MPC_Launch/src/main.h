#ifndef MPC_MAIN_H
#define MPC_MAIN_H

#include <mpc_config.h>

#ifdef HAVE_ENVIRON_VAR
#include <stdlib.h>
extern char **environ;
#endif

#ifdef HAVE_ENVIRON_VAR
        #define CALL_MAIN(main, argc, argv) main( argc, argv, environ)
#else
        #define CALL_MAIN(main, argc, argv) main( argc, argv)
#endif





#endif /* MPC_MAIN_H */