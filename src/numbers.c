#include <assert.h>
#include "priv/numbers.h"

uint64_t nt_compute_barrett_factor(uint64_t factor, uint64_t mod, uint64_t n) {
	assert(n + nt_alpha >= 64);
	const __uint128_t mu = (((__uint128_t) factor) << 64) / mod;
	return mu;
}
