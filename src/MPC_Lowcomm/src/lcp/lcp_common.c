/* ############################# MPC License ############################## */
/* # Thu May  6 10:26:16 CEST 2021                                        # */
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
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - CANAT Paul pcanat@paratools.fr                                     # */
/* # - BESNARD Jean-Baptiste jbbesnard@paratools.com                      # */
/* # - MOREAU Gilles gilles.moreau@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

#include "lcp_common.h"

static uint64_t large_primes[] = {
	14476643271716824181ull, 12086978239110065677ull,
	15386586898367453843ull, 17958312454893560653ull,

	32416188191ull, 32416188793ull,
	32416189381ull, 32416190071ull,

	9929050057ull, 9929050081ull, 9929050097ull, 9929050111ull,
	9929050121ull, 9929050133ull, 9929050139ull, 9929050163ull,
	9929050207ull, 9929050217ull, 9929050249ull, 9929050253ull
};

unsigned int rand_seed;

void rand_seed_init(void) {
	struct timeval tv;

	gettimeofday(&tv, NULL);
	rand_seed = large_primes[0] * tv.tv_sec  + 
		    large_primes[1] * tv.tv_usec +
		    large_primes[2] * syscall(SYS_gettid);
}

uint64_t lcp_rand_uint64(void) {
  return rand_r(&rand_seed);
}
