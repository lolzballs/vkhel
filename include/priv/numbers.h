#ifndef NUMBERS_H
#define NUMBERS_H

#include <stdbool.h>
#include <stdint.h>

/* barrett reduction: alpha - beta = 64 */
static const int64_t nt_alpha	= 62;
static const int64_t nt_beta	= -2;

inline static uint64_t nt_ceil_log2(uint64_t v) {
	return 64 - __builtin_clzll(v);
}

uint64_t nt_compute_barrett_factor(uint64_t factor, uint64_t mod, uint64_t n);
uint64_t nt_multiply_mod(const uint64_t a, const uint64_t b,
		const uint64_t mod, const uint64_t barrett_factor);
uint64_t nt_power_mod(uint64_t base, uint64_t exp,
		const uint64_t mod);
bool nt_is_primitive_root(const uint64_t root, const uint64_t degree,
		const uint64_t mod);

#endif
