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
#ifndef __SCTK__ALLOC__MEMINFO__
#define __SCTK__ALLOC__MEMINFO__

#ifdef __cplusplus
extern "C"
{
#endif
#ifndef __SCTK__ALLOC__C__
#error "For internal use only"
#endif

#include "sctk_alloc_list.h"
#ifdef SCTK_MEMINFO
  enum
  {
    idx_malloc = 0,
    idx_realloc,
    idx_calloc,
    idx_free,
    idx_last
  };

  typedef unsigned long sctk_memusage_cntr_t;
  typedef unsigned long sctk_memusage_size_t;

  static sctk_memusage_cntr_t calls[idx_last];
  static sctk_memusage_cntr_t failed[idx_last];
  static sctk_memusage_size_t total[idx_last];
  static sctk_memusage_size_t grand_total;
  static sctk_memusage_cntr_t histogram[65536 / 16];
  static sctk_memusage_cntr_t large;
  static sctk_memusage_cntr_t calls_total;
  static sctk_memusage_cntr_t inplace;
  static sctk_memusage_cntr_t decreasing;
  static sctk_memusage_size_t current_heap;
  static sctk_memusage_size_t peak_use[3];

#define peak_heap	peak_use[0]
#define peak_stack	peak_use[1]
#define peak_total	peak_use[2]

#define sctk_atomic_increment(a) (*(a))++
#define sctk_atomic_add(a,inc) (*(a))+=inc
#define sctk_atomic_dec(a,inc) (*(a))-=inc

  static void update_data (void *ptr, int type)
  {
    unsigned long size;

    if (ptr != NULL)
      {
	size = sctk_get_chunk (ptr)->cur_size;
	if (type == 0)
	  {
	    sctk_atomic_add (&current_heap, size);
	    if (current_heap > peak_heap)
	      {
		peak_heap = current_heap;
	      }
	  }
	else
	  {
	    sctk_atomic_dec (&current_heap, size);
	  }
      }
  }

  static void __attribute__ ((destructor)) dest (void)
  {
    int percent, cnt;
    unsigned long int maxcalls;


    fprintf (stderr, "\n\
\e[01;32mMemory usage summary:\e[0;0m heap total: %llu, heap peak: %lu, stack peak: %lu\n\
\e[04;34m         total calls   total memory   failed calls\e[0m\n\
\e[00;34m malloc|\e[0m %10lu   %12llu   %s%12lu\e[00;00m\n\
\e[00;34mrealloc|\e[0m %10lu   %12llu   %s%12lu\e[00;00m   (in place: %ld, dec: %ld)\n\
\e[00;34m calloc|\e[0m %10lu   %12llu   %s%12lu\e[00;00m\n\
\e[00;34m   free|\e[0m %10lu   %12llu\n", (unsigned sctk_long_long int) grand_total, (unsigned long int) peak_heap, (unsigned long int) peak_stack, (unsigned long int) calls[idx_malloc], (unsigned sctk_long_long int) total[idx_malloc], failed[idx_malloc] ? "\e[01;41m" : "", (unsigned long int) failed[idx_malloc], (unsigned long int) calls[idx_realloc], (unsigned sctk_long_long int) total[idx_realloc], failed[idx_realloc] ? "\e[01;41m" : "", (unsigned long int) failed[idx_realloc], (unsigned long int) inplace, (unsigned long int) decreasing, (unsigned long int) calls[idx_calloc], (unsigned sctk_long_long int) total[idx_calloc], failed[idx_calloc] ? "\e[01;41m" : "", (unsigned long int) failed[idx_calloc], (unsigned long int) calls[idx_free], (unsigned sctk_long_long int) total[idx_free]);
    /* Write out a histoogram of the sizes of the allocations.  */
    fprintf (stderr, "\e[01;32mHistogram for block sizes:\e[0;0m\n");

    /* Determine the maximum of all calls for each size range.  */
    maxcalls = large;
    for (cnt = 0; cnt < chunk_sizes[max_chunk_sizes - 1];
	 cnt += step_chunk_sizes)
      if (histogram[cnt / step_chunk_sizes] > maxcalls)
	maxcalls = histogram[cnt / step_chunk_sizes];

    for (cnt = 0; cnt < chunk_sizes[max_chunk_sizes - 1];
	 cnt += step_chunk_sizes)
      /* Only write out the nonzero entries.  */
      if (histogram[cnt / step_chunk_sizes] != 0)
	{
	  percent = (histogram[cnt / step_chunk_sizes] * 100) / calls_total;
	  fprintf (stderr, "%5d-%-5d%12lu ", cnt,
		   cnt + (step_chunk_sizes - 1),
		   (unsigned long int) histogram[cnt / step_chunk_sizes]);
	  if (percent == 0)
	    fputs (" <1% \e[41;37m", stderr);
	  else
	    fprintf (stderr, "%3d%% \e[41;37m", percent);

	  /* Draw a bar with a length corresponding to the current
	     percentage.  */
	  percent = (histogram[cnt / step_chunk_sizes] * 50) / maxcalls;
	  while (percent-- > 0)
	    fputc ('=', stderr);
	  fputs ("\e[0;0m\n", stderr);
	}
    if (large != 0)
      {
	percent = (large * 100) / calls_total;
	fprintf (stderr, "   large   %12lu ", (unsigned long int) large);
	if (percent == 0)
	  fputs (" <1% \e[41;37m", stderr);
	else
	  fprintf (stderr, "%3d%% \e[41;37m", percent);
	percent = (large * 50) / maxcalls;
	while (percent-- > 0)
	  fputc ('=', stderr);
	fputs ("\e[0;0m\n", stderr);
      }

  }
#endif

#ifdef __cplusplus
}				/* end of extern "C" */
#endif
#endif
