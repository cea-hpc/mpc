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
#ifndef __SCTK___DEBUG__IO__
#define __SCTK___DEBUG__IO__

#include <unistd.h>
#include <stdlib.h>
#include <sctk_debug.h>

#ifdef __cplusplus
extern "C"
{
#endif

  static inline int sctk_unsafe_read (int fd, void *buf, ssize_t count)
  {
    char *tmp;
    ssize_t allready_readed = 0;
    ssize_t dcount;

      tmp = buf;

      dcount = read (fd, tmp, count);
    if (dcount <= 0)
      {
	return dcount;
      }
    assume (dcount > 0);

    allready_readed += dcount;

    while (allready_readed < count)
      {
	tmp += dcount;
	dcount = read (fd, tmp, count - allready_readed);
	assume (dcount >= 0);
	allready_readed += dcount;
      }
    assume (count == allready_readed);
    return allready_readed;
  }

  static inline int sctk_safe_read (int fd, void *buf, ssize_t count)
  {
    char *tmp;
    ssize_t allready_readed = 0;
    ssize_t dcount = 0;

    tmp = buf;
    while (allready_readed < count)
      {
	tmp += dcount;
	dcount = read (fd, tmp, count - allready_readed);
	if (!(dcount >= 0))
	  {
	    sctk_error ("READ %p %lu/%lu FAIL\n", tmp,
			count - allready_readed, count);
	    perror ("sctk_safe_read");
	  }
	assume (dcount >= 0);
	allready_readed += dcount;
      }
    assume (count == allready_readed);
    return allready_readed;
  }

  static inline int sctk_safe_write (int fd, const void *buf, ssize_t count)
  {
    ssize_t dcount;
    dcount = write (fd, buf, count);
    if (!(dcount == count))
      {
	sctk_error ("WRITE %p %lu/%lu FAIL\n", buf, count);
	perror ("sctk_safe_write");
      }
    assume (dcount == count);
    return dcount;
  }


#ifdef __cplusplus
}				/* end of extern "C" */
#endif
#endif
