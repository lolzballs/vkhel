#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <vkhel.h>

static void print_vector(struct vkhel_vector *vec, size_t vec_length) {
	uint64_t *mapped;
	vkhel_vector_map(vec, (void **) &mapped, vec_length);
	printf("{");
	for (size_t i = 0; i < vec_length; i++) {
		printf("%" PRIu64 ", ", mapped[i]);
	}
	printf("}\n");
	vkhel_vector_unmap(vec);

}

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

	struct vkhel_vector *c = vkhel_vector_create(ctx, vector_len);
	vkhel_vector_elemmul(a, b, c, 17);
	print_vector(c, vector_len);

	struct vkhel_vector *d = vkhel_vector_dup(c);
	vkhel_vector_destroy(c);

	vkhel_vector_elemmul(a, b, d, 17);
	print_vector(d, vector_len);
	vkhel_vector_destroy(d);

	vkhel_vector_destroy(a);
	vkhel_vector_destroy(b);

	vkhel_ctx_destroy(ctx);
}
