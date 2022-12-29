#include <assert.h>
#include "vkhel/vkhel.h"

int main() {
	struct vkhel_ctx *ctx = vkhel_ctx_create();

	const size_t vector_len = 4;

	uint64_t a_elements[4] = { 1, 2, 3, 4 };
	struct vkhel_vector *a = vkhel_vector_create(ctx, vector_len);
	vkhel_vector_copy_from_host(a, a_elements);

	uint64_t b_elements[] = { 4, 0, 0, 1 };
	struct vkhel_vector *b = vkhel_vector_create(ctx, vector_len);
	vkhel_vector_copy_from_host(b, b_elements);

	uint64_t *a_map;
	vkhel_vector_map(a, (void **) &a_map, 4);
	for (size_t i = 0; i < vector_len; i++) {
		assert(a_map[i] == a_elements[i]);
	}
	vkhel_vector_unmap(a);

	uint64_t *b_map;
	vkhel_vector_map(b, (void **) &b_map, 4);
	for (size_t i = 0; i < vector_len; i++) {
		assert(b_map[i] == b_elements[i]);
	}
	vkhel_vector_unmap(b);

	vkhel_vector_destroy(a);
	vkhel_vector_destroy(b);

	vkhel_ctx_destroy(ctx);
}
