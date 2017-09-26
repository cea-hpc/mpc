#ifndef __MPCOMP_ENUM_MACROS_H__
#define __MPCOMP_ENUM_MACROS_H__

#define SCTK_OMP_VERSION_MAJOR 3
#define SCTK_OMP_VERSION_MINOR 1

#define MPCOMP_TASK 1
#define MPCOMP_USE_INTEL_ABI 1

#define MPCOMP_OPENMP_3_0

#if defined( MPCOMP_OPENMP_3_0 )
//#warning "MPC_OpenMP use MPCOMP_OPENMP_3_0 support"
#define MPCOMP_TASK 1
#endif /* MPCOMP_OPENMP_3_0 */


/* Maximum number of threads for each team of a parallel region */
#define MPCOMP_MAX_THREADS 256
/* Number of threads per microVP */
#define MPCOMP_MAX_THREADS_PER_MICROVP 1

/* Maximum number of alive 'for dynamic' and 'for guided'  construct */
#define MPCOMP_MAX_ALIVE_FOR_DYN 3

#define MPCOMP_NOWAIT_STOP_SYMBOL (-2)

/* Uncomment to enable coherency checking */
#define MPCOMP_COHERENCY_CHECKING 0
#define MPCOMP_OVERFLOW_CHECKING 0

/* MACRO FOR PERFORMANCE */
#define MPCOMP_MALLOC_ON_NODE 1
#define MPCOMP_TRANSFER_INFO_ON_NODES 0
#define MPCOMP_ALIGN 1

#define MPCOMP_CHUNKS_NOT_AVAIL 1
#define MPCOMP_CHUNKS_AVAIL 2

/* Define macro MPCOMP_MIC in case of Xeon Phi compilation */
#if __MIC__ || __MIC2__
#define MPCOMP_MIC 1
#endif /* __MIC__ || __MIC2__ */

#endif /* __MPCOMP_ENUM_MACROS_H__ */
