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
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */
#include <unistd.h>
#include <sys/time.h>
#include <mpc_arch.h>

double mpc_arch_get_timestamp_gettimeofday()
{
	struct timeval tp;

	gettimeofday(&tp, NULL);
	return (double)tp.tv_sec * 1e6 + (double)tp.tv_usec;
}

#if defined(MPC_IA64_ARCH)
static inline double __get_timestamp()
{
	unsigned long t;
	__asm__ volatile ("mov %0=ar%1" : "=r" (t) : "i" (44) );

	return (double)t;
}

#elif defined(MPC_I686_ARCH)
static inline double __get_timestamp()
{
	unsigned long long t;
	__asm__ volatile ("rdtsc" : "=A" (t) );

	return (double)t;
}

#elif defined(MPC_X86_64_ARCH)
static inline double __get_timestamp()
{
	unsigned int  a;
	unsigned int  d;
	unsigned long t;
	__asm__ volatile ("rdtsc" : "=a" (a), "=d" (d) );

	t = ( (unsigned long)a) | ( ( (unsigned long)d) << 32);
	return (double)t;
}

#elif defined(MPC_ARM_ARCH)
static inline double __get_timestamp()
{
	return mpc_arch_get_timestamp_gettimeofday();
}

#else
#warning "Use get time of day for profiling"
static inline double __get_timestamp()
{
	return mpc_arch_get_timestamp_gettimeofday();
}
#endif

static double __cpu_freq = 0;

static double __begin_tsc          = 0.0;
static double __start_gettimeofday = 0.0;

void mpc_arch_tsc_freq_compute_start()
{
	__start_gettimeofday = mpc_arch_get_timestamp_gettimeofday();
	__begin_tsc          = __get_timestamp();
}

void mpc_arch_tsc_freq_compute()
{
	if(!__begin_tsc)
	{
		mpc_arch_tsc_freq_compute_start();
		usleep(10000);
	}

	double end_tsc       = __get_timestamp();
	double end_timeofday = mpc_arch_get_timestamp_gettimeofday();

	__cpu_freq = (end_tsc - __begin_tsc) / ( (end_timeofday - __start_gettimeofday) / 1e6);

	if(__cpu_freq < 1)
	{
		__cpu_freq = 1;
	}
}

double mpc_arch_tsc_freq_get()
{
	if(__cpu_freq == 0)
	{
		mpc_arch_tsc_freq_compute();
	}
	return __cpu_freq;
}

double mpc_arch_get_timestamp()
{
	double freq = mpc_arch_tsc_freq_get();

	return (__get_timestamp() - __begin_tsc) / freq;
}
