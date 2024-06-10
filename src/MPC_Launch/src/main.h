#ifndef MPC_MAIN_H
#define MPC_MAIN_H

#include <mpc_config.h>

/********************************************
 * HANDLING OF MAIN FORWARDING WITH ENVIRON *
 ********************************************/

#ifdef HAVE_ENVIRON_VAR
#include <stdlib.h>
extern char **environ;
#endif

#ifdef HAVE_ENVIRON_VAR
/**
 * @brief Macro used to forward main Environ is added if present
 */
#define CALL_MAIN(main, argc, argv) main( argc, argv, environ)
#else
#define CALL_MAIN(main, argc, argv) main( argc, argv)
#endif

/**********************************************
 * THIS IS THE MAIN INSIDE THE TARGET PROGRAM *
 **********************************************/

#ifdef HAVE_ENVIRON_VAR
/**
 * @brief Definition of the user-main
 */
extern int mpc_user_main__ (int, char **,char**);
#else
extern int mpc_user_main__ (int, char **);
#endif

/************************
 * FORTRAN ENTRY POINTS *
 ************************/

/**
 * @brief Fortran entry point in MPC
 *
 */
void mpc_start_ (void);


/**
 * @brief Fortran entry point in MPC
 *
 */
void mpc_start__ (void);

#endif /* MPC_MAIN_H */
