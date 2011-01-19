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
#ifndef __SCTK_TRACE_H__
#define __SCTK_TRACE_H__
#ifdef __cplusplus
extern "C"
{
#endif
  void sctk_trace_init (void);
  void sctk_trace_commit (void);
  void sctk_trace_end (void);

  /*DANGER do not change the following variable dues to LIBZ compressBound issues */
#ifdef SCTK_32_BIT_ARCH
#define NB_ENTRIES (10*1000)
#else
#define NB_ENTRIES (1*1000*1000)
#endif

  typedef struct
  {
    double date;
    char **function;
    void *arg1;
    void *arg2;
    void *arg3;
    void *arg4;
    void *th;
    int vp;
  } sctk_trace_block_t;

  typedef struct
  {
    int step;
    sctk_trace_block_t buf[NB_ENTRIES];
    char *tmp_buffer_compress;
  } sctk_trace_t;



#ifdef __cplusplus
}				/* end of extern "C" */
#endif
#endif
