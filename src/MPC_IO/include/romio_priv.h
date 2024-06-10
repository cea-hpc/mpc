#ifndef ROMIO_PRIV_H_
#define ROMIO_PRIV_H_

#include <mpc_thread_romio_ctx.h>

/**********
 * VOID * *
 **********/

#define ADIOI_Datarep_head ( MPC_THREAD_ROMIO_CTX_GET( ADIOI_Datarep_head, ADIOI_Datarep*) )
#define ADIOI_Ftable ( MPC_THREAD_ROMIO_CTX_GET( ADIOI_Ftable, ADIO_File*) )
#define ADIOI_Reqtable ( MPC_THREAD_ROMIO_CTX_GET( ADIOI_Reqtable, ADIO_Request*) )
#define MPIR_Infotable ( MPC_THREAD_ROMIO_CTX_GET( MPIR_Infotable, MPI_Info*) )
#define flatBuf_global ( MPC_THREAD_ROMIO_CTX_GET( flatBuf_global, ADIOI_Flatlist_node*) )

#define stripeWriteLens ( MPC_THREAD_ROMIO_CTX_GET( stripeWriteLens, ADIO_Offset*) )
#define stripeWriteOffsets ( MPC_THREAD_ROMIO_CTX_GET( stripeWriteOffsets, ADIO_Offset*) )

#define token_ptr ( MPC_THREAD_ROMIO_CTX_GET( token_ptr, char*) )
#define yylval ( MPC_THREAD_ROMIO_CTX_GET( yylval, char*) )

/*******
 * INT *
 *******/

#define ADIOI_Ftable_ptr ( MPC_THREAD_ROMIO_CTX_GET( ADIOI_Ftable_ptr, int) )
#define ADIOI_Ftable_max ( MPC_THREAD_ROMIO_CTX_GET( ADIOI_Ftable_max, int) )
#define ADIOI_Reqtable_ptr ( MPC_THREAD_ROMIO_CTX_GET( ADIOI_Reqtable_ptr, int) )
#define ADIOI_Reqtable_max ( MPC_THREAD_ROMIO_CTX_GET( ADIOI_Reqtable_max, int) )
#define MPIR_Infotable_ptr ( MPC_THREAD_ROMIO_CTX_GET( MPIR_Infotable_ptr, int) )
#define MPIR_Infotable_max ( MPC_THREAD_ROMIO_CTX_GET( MPIR_Infotable_max, int) )
#define ADIOI_Direct_read ( MPC_THREAD_ROMIO_CTX_GET( ADIOI_Direct_read, int) )
#define ADIOI_Direct_write ( MPC_THREAD_ROMIO_CTX_GET( ADIOI_Direct_write, int) )
#define ADIOI_cb_config_list_keyval ( MPC_THREAD_ROMIO_CTX_GET( ADIOI_cb_config_list_keyval, int) )
#define ADIOI_Flattened_type_keyval ( MPC_THREAD_ROMIO_CTX_GET( ADIOI_Flattened_type_keyval, int) )
#define ADIO_Init_keyval ( MPC_THREAD_ROMIO_CTX_GET( ADIO_Init_keyval, int) )

#define amountOfStripedDataExpected ( MPC_THREAD_ROMIO_CTX_GET( amountOfStripedDataExpected, int) )

#define gpfsmpio_balancecontig ( MPC_THREAD_ROMIO_CTX_GET( gpfsmpio_balancecontig, int) )
#define gpfsmpio_bg_nagg_pset ( MPC_THREAD_ROMIO_CTX_GET( gpfsmpio_bg_nagg_pset, int) )
#define gpfsmpio_bridgeringagg ( MPC_THREAD_ROMIO_CTX_GET( gpfsmpio_bridgeringagg, int) )
#define gpfsmpio_comm ( MPC_THREAD_ROMIO_CTX_GET( gpfsmpio_comm, int) )
#define gpfsmpio_devnullio ( MPC_THREAD_ROMIO_CTX_GET( gpfsmpio_devnullio, int) )
#define gpfsmpio_p2pcontig ( MPC_THREAD_ROMIO_CTX_GET( gpfsmpio_p2pcontig, int) )
#define gpfsmpio_pthreadio ( MPC_THREAD_ROMIO_CTX_GET( gpfsmpio_pthreadio, int) )
#define gpfsmpio_timing ( MPC_THREAD_ROMIO_CTX_GET( gpfsmpio_timing, int) )
#define gpfsmpio_timing2 ( MPC_THREAD_ROMIO_CTX_GET( gpfsmpio_timing2, int) )
#define gpfsmpio_timing_cw_level ( MPC_THREAD_ROMIO_CTX_GET( gpfsmpio_timing_cw_level, int) )
#define gpfsmpio_tuneblocking ( MPC_THREAD_ROMIO_CTX_GET( gpfsmpio_tuneblocking, int) )

#define iWasUsedStripingAgg ( MPC_THREAD_ROMIO_CTX_GET( iWasUsedStripingAgg, int) )

#define lastDataTypeExtent ( MPC_THREAD_ROMIO_CTX_GET( lastDataTypeExtent, int) )
#define lastFlatBufIndice ( MPC_THREAD_ROMIO_CTX_GET( lastFlatBufIndice, int) )

#define numStripesUsed ( MPC_THREAD_ROMIO_CTX_GET( numStripesUsed, int) )

#define romio_onesided_always_rmw ( MPC_THREAD_ROMIO_CTX_GET( romio_onesided_always_rmw, int) )
#define romio_onesided_inform_rmw ( MPC_THREAD_ROMIO_CTX_GET( romio_onesided_inform_rmw, int) )
#define romio_onesided_no_rmw ( MPC_THREAD_ROMIO_CTX_GET( romio_onesided_no_rmw, int) )
#define romio_read_aggmethod ( MPC_THREAD_ROMIO_CTX_GET( romio_read_aggmethod, int) )
#define romio_tunegather ( MPC_THREAD_ROMIO_CTX_GET( romio_tunegather, int) )
#define romio_write_aggmethod ( MPC_THREAD_ROMIO_CTX_GET( romio_write_aggmethod, int) )

/* MPI_Err_handler == INT */
#define ADIOI_DFLT_ERR_HANDLER ( MPC_THREAD_ROMIO_CTX_GET( ADIOI_DFLT_ERR_HANDLER, MPI_Errhandler) )

/* MPI_Info == MPI_Err_handler == INT */
#define ADIOI_syshints ( MPC_THREAD_ROMIO_CTX_GET( ADIOI_syshints, MPI_Info) )

/* MPI_Op^== INT */
#define ADIO_same_amode ( MPC_THREAD_ROMIO_CTX_GET( ADIO_same_amode, MPI_Op) )

/* MPIX_Grequest_class == INT */
#define ADIOI_GEN_greq_class ( MPC_THREAD_ROMIO_CTX_GET( ADIOI_GEN_greq_class, MPIX_Grequest_class) )

/********
 * LONG *
 ********/

#define bglocklessmpio_f_type ( MPC_THREAD_ROMIO_CTX_GET( bglocklessmpio_f_type, long) )

/**********
 * SIZE_T *
 **********/

#define bufTypeExtent_global ( MPC_THREAD_ROMIO_CTX_GET( bufTypeExtent_global, MPI_Aint) )
#define lastIndiceOffset ( MPC_THREAD_ROMIO_CTX_GET( lastIndiceOffset, ADIO_Offset) )

/**********
 * DOUBLE *
 **********/

/* 21 */
#define gpfsmpio_prof_cw ( MPC_THREAD_ROMIO_CTX_GET( gpfsmpio_prof_cw, double) )
/* 21 */
#define gpfsmpio_prof_cr ( MPC_THREAD_ROMIO_CTX_GET( gpfsmpio_prof_cr, double) )



#endif /* ROMIO_PRIV_H_ */
