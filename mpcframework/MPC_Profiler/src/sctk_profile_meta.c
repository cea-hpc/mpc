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
/* #   - BESNARD Jean-Baptiste jean-baptiste.besnard@cea.fr               # */
/* #                                                                      # */
/* ######################################################################## */

#include "sctk_profile_meta.h"

#include "sctk_debug.h"
#include "mpc_mpi.h"
#include "mpc_common_asm.h"


void sctk_profile_meta_calibrate_begin(struct sctk_profile_meta *meta)
{
			meta->begin_ts = sctk_get_time_stamp();
	        gettimeofday( &meta->calibration_time , NULL );
}


void sctk_profile_meta_calibrate_end(struct sctk_profile_meta *meta)
{
        struct timeval e_tv;

        uint64_t end = sctk_get_time_stamp();
        gettimeofday( &e_tv , NULL );

        double t_begin, t_end;

        t_begin = meta->calibration_time.tv_sec + meta->calibration_time.tv_usec/1000000;
        t_end = e_tv.tv_sec + e_tv.tv_usec/1000000;

        meta->ticks_per_second = (end-meta->begin_ts)/(t_end-t_begin);

}


void sctk_profile_meta_init(struct sctk_profile_meta *meta)
{

	MPC_Comm_size(SCTK_COMM_WORLD, &meta->task_count);
	MPC_Node_number( &meta->node_count );
	MPC_Process_number( &meta->process_count );


	char *cmd = getenv( "MPC_LAUNCH_COMMAND" );

	if( cmd )
	{
		sprintf(meta->command, "%s", cmd);
	}
	else
	{
		sprintf(meta->command, "Not found");
	}

	meta->lock = 0;
	meta->status = 0;
}


void sctk_profile_meta_release(struct sctk_profile_meta *meta)
{


}


void sctk_profile_meta_begin_compute(struct sctk_profile_meta *meta)
{
	mpc_common_spinlock_lock( &meta->lock );
	{
		if( meta->status == 0)
		{
			/* Do it only once */
			sctk_profile_meta_calibrate_begin(meta);
			meta->status = 1;
		}
	}
	mpc_common_spinlock_unlock( &meta->lock );
}


void sctk_profile_meta_end_compute(struct sctk_profile_meta *meta)
{
	mpc_common_spinlock_lock( &meta->lock );
	{
		if( meta->status == 1)
		{
			/* Do it only once */

			uint64_t now = sctk_get_time_stamp();

			meta->walltime = now - meta->begin_ts;

			if( now - meta->begin_ts < 1e9 )
			{
				sctk_info("Calibrating timestamps...");
				sleep(1);
			}

			sctk_profile_meta_calibrate_end(meta);


			meta->status = 2;
		}
	}
	mpc_common_spinlock_unlock( &meta->lock );

	int rank = 0;
	MPC_Comm_rank(SCTK_COMM_WORLD, &rank);

	if( rank == 0)
        sctk_info("Program running at %g ticks per sec", meta->ticks_per_second);

}
