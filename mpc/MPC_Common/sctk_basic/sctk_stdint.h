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

#ifndef SCTK_STDINT_H
#define SCTK_STDINT_H

//stdint.h is C99, if we didn't get support of it, fall-back on manual
//definitions of sctk_uint64_t... For compatibility, please use sctk_stdin.h in MPC
//in place of stdint.h
#ifdef HAVE_STDINT_H
	//use standard defs
	#include <stdint.h>
	typedef int8_t       sctk_int8_t;
	typedef int16_t      sctk_int16_t;
	typedef int32_t      sctk_int32_t;
	typedef int64_t      sctk_int64_t;

	typedef uint8_t      sctk_uint8_t;
	typedef uint16_t     sctk_uint16_t;
	typedef uint32_t     sctk_uint32_t;
	typedef uint64_t     sctk_uint64_t;
#else //HAVE_STDINT_H
	//select arch specific mode
	#if defined(SCTK_x86_64_ARCH_SCTK) || defined(SCTK_i686_ARCH_SCTK)
		//for x86 64bit
		typedef signed char             sctk_int8_t;
		typedef short int               sctk_int16_t;
		typedef int                     sctk_int32_t;
		typedef long int                sctk_int64_t;

		typedef unsigned char           sctk_uint8_t;
		typedef unsigned short int      sctk_uint16_t;
		typedef unsigned int            sctk_uint32_t;
		//"long long int" is 64bit for 32bit and 64bit arch
		//not as "long int" which change.
		//Also OK for windows which use 32bit long int and
		//64bit long long on x86_64
		typedef unsigned long long int  sctk_uint64_t;
	#else //arch
		#error "Unsupported architecture without stdint.h"
	#endif //arch
#endif //HAVE_STDINT_H

#endif /* STDINT_H_ */
