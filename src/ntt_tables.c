#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "priv/ntt_tables.h"
#include "priv/numbers.h"

static uint64_t reverse_bits(uint64_t a, const uint64_t bit_width) {
	uint64_t rev = 0;
	for (int64_t bit = bit_width - 1; bit >= 0; bit--) {
		rev |= (a & 1) << bit;
		a >>= 1;
	}
	return rev;
}

static void compute_roots_of_unity_powers(struct ntt_tables *ntt) {
	ntt->roots_of_unity[0] = 1;
	ntt->inv_roots_of_unity[0] = nt_inverse_mod(1, ntt->q);

	const uint64_t barrett_factor = nt_compute_barrett_factor(
			(uint64_t) 1 << (nt_ceil_log2(ntt->q) + nt_alpha - 64),
			ntt->q, nt_ceil_log2(ntt->q));

	uint64_t prev_root = 1;
	for (uint64_t i = 1; i < ntt->degree; i++) {
		uint64_t idx = reverse_bits(i, nt_ceil_log2(ntt->degree) - 1);
		ntt->roots_of_unity[idx] = nt_multiply_mod(prev_root, ntt->w,
				ntt->q, barrett_factor);
		ntt->inv_roots_of_unity[idx] =
			nt_inverse_mod(ntt->roots_of_unity[idx], ntt->q);

		prev_root = ntt->roots_of_unity[idx];
	}

	for (uint64_t i = 0; i < ntt->degree; i++) {
		ntt->roots_barrett_factors[i] =
			nt_compute_barrett_factor(ntt->roots_of_unity[i],
					ntt->q, nt_ceil_log2(ntt->q));
		ntt->inv_roots_barrett_factors[i] =
			nt_compute_barrett_factor(ntt->inv_roots_of_unity[i],
					ntt->q, nt_ceil_log2(ntt->q));
	}
}

struct ntt_tables *ntt_tables_create(uint64_t degree, uint64_t q, uint64_t w) {
	struct ntt_tables *ini = calloc(1, sizeof(struct ntt_tables));
	ini->degree = degree;
	ini->q = q;
	ini->w = w;

	ini->roots_of_unity = malloc(sizeof(uint64_t) * degree);
	ini->inv_roots_of_unity = malloc(sizeof(uint64_t) * degree);
	ini->roots_barrett_factors = malloc(sizeof(uint64_t) * degree);
	ini->inv_roots_barrett_factors = malloc(sizeof(uint64_t) * degree);

	compute_roots_of_unity_powers(ini);

	return ini;
}

void ntt_tables_destroy(struct ntt_tables *ntt) {
	free(ntt->roots_of_unity);
	free(ntt->inv_roots_of_unity);
	free(ntt->roots_barrett_factors);
	free(ntt->inv_roots_barrett_factors);
}
