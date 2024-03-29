/* ---------------------------------------------------------------- */
/* (C)Copyright IBM Corp.  2007, 2008                               */
/* ---------------------------------------------------------------- */
/**
 * \file ad_tuning.h
 * \brief Defines common performance tuning env var options
 */

/*---------------------------------------------------------------------
 * ad_tuning.h
 *
 * declares common global variables and functions for performance tuning
 *---------------------------------------------------------------------*/

#ifndef AD_TUNING_H_INCLUDED
#define AD_TUNING_H_INCLUDED

#include "adio.h"


/*-----------------------------------------
 *  Global variables for the control of performance tuning.
 *-----------------------------------------*/

/*MPC PATCH*/
#if 0
/* corresponds to environment variables to select optimizations */
extern int romio_write_aggmethod;
extern int romio_read_aggmethod;
extern int romio_onesided_no_rmw;
extern int romio_onesided_always_rmw;
extern int romio_onesided_inform_rmw;
extern int romio_tunegather;
#else
extern int mpc_romio_write_aggmethod;
extern int mpc_romio_read_aggmethod;
extern int mpc_romio_onesided_no_rmw;
extern int mpc_romio_onesided_always_rmw;
extern int mpc_romio_onesided_inform_rmw;
extern int mpc_romio_tunegather;
#endif

/* set internal variables for tuning environment variables */
void ad_get_env_vars(void);

#endif /* AD_TUNING_H_INCLUDED */
