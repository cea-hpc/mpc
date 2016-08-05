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
/* #   - ADAM Julien adamj@paratools.com                                  # */
/* #                                                                      # */
/* ######################################################################## */
#include <cuda.h>

extern CUresult cuInit(unsigned int);
CUresult sctk_cuInit(unsigned int flag) {return (int) cuInit(flag);}

extern CUresult cuCtxCreate(CUcontext*, unsigned int, CUdevice);
CUresult sctk_cuCtxCreate(CUcontext* c, unsigned int f, CUdevice d) { return (int) cuCtxCreate(c, f, d);}

extern CUresult cuCtxPopCurrent(CUcontext*);
CUresult sctk_cuCtxPopCurrent(CUcontext* c) {return (int) cuCtxPopCurrent(c);}

extern CUresult cuCtxPushCurrent(CUcontext);
CUresult sctk_cuCtxPushCurrent(CUcontext c) {return (int) cuCtxPushCurrent(c);}

extern CUresult cuDeviceGetByPCIBusId(CUdevice*, const char*);
CUresult sctk_cuDeviceGetByPCIBusId(CUdevice* d, const char * b) {return (int) cuDeviceGetByPCIBusId(d, b);}
