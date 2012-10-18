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
/* #   - Valat Sebastien sebastien.valat@cea.fr                           # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef SCTK_STDINT_H_
#define SCTK_STDINT_H_

//stdint.h is C99, if we didn't get support of it, fall-back on manual
//definitions of uint64_t... For compatibility, please use sctk_stdin.h in MPC.
#ifdef HAVE_STDINT_H
	#include <stdint.h>
#elif !defined(uint64_t)
	//select arch specific mode
	#if defined(SCTK_x86_64_ARCH_SCTK)
		//for x86 64bit
		typedef signed char             int8_t;
		typedef short int               int16_t;
		typedef int                     int32_t;
		typedef long int                int64_t;

		typedef unsigned char           uint8_t;
		typedef unsigned short int      uint16_t;
		typedef unsigned int            uint32_t;
		typedef unsigned long int       uint64_t;
	#elif defined(SCTK_i686_ARCH_SCTK)
		//for x86 32bit
		typedef signed char             int8_t;
		typedef short int               int16_t;
		typedef int                     int32_t;
		typedef long long int           int64_t;

		typedef unsigned char           uint8_t;
		typedef unsigned short int      uint16_t;
		typedef unsigned int            uint32_t;
		typedef unsigned long long int  uint64_t;
	#else
		#error "Unsupported architecture without support of stdint.h"
	#endif
#endif

#endif /* STDINT_H_ */
