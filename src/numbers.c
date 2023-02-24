#include <assert.h>
#include "priv/numbers.h"

static uint64_t compute_mod(const uint64_t hi, const uint64_t lo,
		const uint64_t mod, const uint64_t barrett_factor) {
	const uint64_t n = nt_ceil_log2(mod);
	/* TODO: sanity check for gamma <= n */

	/* mu = (1 << (n + alpha)) / modulus
	* assert low 64 bits are 0 */
	assert(n + nt_alpha >= 64);
	const uint64_t mu_pre_hi = (uint64_t) 1 << (n + nt_alpha - 64);
	/* barrett reduction factor */
	const __uint128_t mu = (((__uint128_t) mu_pre_hi) << 64) / mod;
	const uint64_t mu_lo = mu;

	/* numerator constant: floor(U / 2^(n + beta)) */
	const uint64_t num_c = (hi << (64 - (n + nt_beta))) + (lo >> (n + nt_beta));
	const __uint128_t num = ((__uint128_t) num_c) * mu_lo;
	const uint64_t q_hat = num >> 64; /* alpha - beta = 64 */

	uint64_t Z = lo - q_hat * mod;
	if (Z >= mod) {
		Z = Z - mod;
	}
	return Z;
}

uint64_t nt_compute_barrett_factor(uint64_t factor, uint64_t mod, uint64_t n) {
	assert(n + nt_alpha >= 64);
	const __uint128_t mu = (((__uint128_t) factor) << 64) / mod;
	return mu;
}

uint64_t nt_multiply_mod(const uint64_t a, const uint64_t b,
		const uint64_t mod, const uint64_t barrett_factor) {
	const __uint128_t product = (__uint128_t) a * (__uint128_t) b;
	return compute_mod(product >> 64, product, mod, barrett_factor);
}

uint64_t nt_power_mod(uint64_t base, uint64_t exp, const uint64_t mod) {
	const uint64_t mod_bits = nt_ceil_log2(mod);
	const uint64_t barrett_factor = nt_compute_barrett_factor(
			(uint64_t) 1 << (mod_bits + nt_alpha - 64),
			mod, mod_bits);

	uint64_t result = 1;
	base %= mod;

	while (exp > 0) {
		if (exp & 1) {
			result = nt_multiply_mod(result, base, mod, barrett_factor);
		}
		base = nt_multiply_mod(base, base, mod, barrett_factor);
		exp >>= 1;
	}
	return result;
}
