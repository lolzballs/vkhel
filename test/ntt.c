#include <assert.h>
#include "priv/vkhel.h"
#include "priv/ntt_tables.h"

void assert_array_eq(uint64_t *a, uint64_t *b, size_t len) {
	for (size_t i = 0; i < len; i++) {
		assert(a[i] == b[i]);
	}
}

void assert_inv_roots(uint64_t mod, uint64_t *roots, uint64_t *inv_roots,
		size_t len) {
	for (size_t i = 0; i < len; i++) {
		assert((roots[i] * inv_roots[i]) % mod);
	}
}

int main() {
	struct ntt_tables *ntt = ntt_tables_create(4, 113, 18);
	assert_array_eq(ntt->roots_of_unity,
			(uint64_t[]) { 1, 98, 18, 69 }, ntt->n);
	assert_inv_roots(ntt->q, ntt->roots_of_unity, ntt->inv_roots_of_unity,
			ntt->n);
	ntt_tables_destroy(ntt);
}
