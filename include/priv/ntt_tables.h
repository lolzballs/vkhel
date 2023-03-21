#ifndef PRIV_NTT_TABLES_H
#define PRIV_NTT_TABLES_H

#include <stdint.h>

struct vkhel_ntt_tables {
	uint64_t n; /* degree */
	uint64_t q; /* modulus */
	uint64_t w; /* root of unity */

	uint64_t *roots_of_unity;
	uint64_t *inv_roots_of_unity;
	uint64_t *roots_barrett_factors;
	uint64_t *inv_roots_barrett_factors;
};

void vkhel_ntt_tables_dbgprint(struct vkhel_ntt_tables *);

#endif
