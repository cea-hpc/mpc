#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

#include "lcp_common.h"

#include "mpc_common_debug.h"

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
	mpc_common_debug_info("LCP: random seed=%d.", rand_seed);
}

uint64_t lcp_rand_uint64(void) {
  return rand_r(&rand_seed);
}
