#ifndef NUMBERS_H
#define NUMBERS_H

#include <stdint.h>

/* barrett reduction: alpha - beta = 64 */
static const int64_t nt_alpha = 62;
static const int64_t nt_beta __attribute__((unused)) = -2;

inline static uint64_t nt_ceil_log2(uint64_t v) {
	return 64 - __builtin_clzll(v);
}

uint64_t nt_compute_barrett_factor(uint64_t factor, uint64_t mod, uint64_t n);

#endif
