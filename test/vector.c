#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include "vkhel/vkhel.h"

#define RUN_TEST(name) ({\
		test_##name();\
		printf("%s passed\n", #name);\
	})

static struct vkhel_ctx *g_ctx;

static void assert_vector_contents_equal(struct vkhel_vector *vec,
		const uint64_t *contents, size_t length) {
	uint64_t *mapped;
	vkhel_vector_map(vec, (void **) &mapped, length);
	for (size_t i = 0; i < length; i++) {
		assert(mapped[i] == contents[i]);
	}
	vkhel_vector_unmap(vec);
}

void test_copy_from_host() {
	const size_t vector_len = 10;
	uint64_t elements[vector_len];

	for (size_t i = 0; i < vector_len; i++) {
		elements[i] = i;
	}

	struct vkhel_vector *v = vkhel_vector_create(g_ctx, vector_len);
	vkhel_vector_copy_from_host(v, elements);

	assert_vector_contents_equal(v, elements, vector_len);

	vkhel_vector_destroy(v);
}

void test_elemadd() {
	const size_t vector_len = 4;
	const uint64_t a_elements[] = { 1, 2, 3, 4 };
	const uint64_t b_elements[] = { 4, 0, 0, 1 };

	struct vkhel_vector *a = vkhel_vector_create(g_ctx, vector_len);
	vkhel_vector_copy_from_host(a, a_elements);

	struct vkhel_vector *b = vkhel_vector_create(g_ctx, vector_len);
	vkhel_vector_copy_from_host(b, b_elements);

	/* mod 17 */
	const uint64_t c_expected[] = { 5, 2, 3, 5 };
	struct vkhel_vector *c = vkhel_vector_create(g_ctx, vector_len);
	vkhel_vector_elemadd(a, b, c, 17);
	assert_vector_contents_equal(c, c_expected, vector_len);
	vkhel_vector_destroy(c);

	/* mod 2 */
	const uint64_t d_expected[] = { 1, 0, 1, 1 };
	struct vkhel_vector *d = vkhel_vector_create(g_ctx, vector_len);
	vkhel_vector_elemadd(a, b, d, 2);
	assert_vector_contents_equal(d, d_expected, vector_len);
	vkhel_vector_destroy(d);

	vkhel_vector_destroy(a);
	vkhel_vector_destroy(b);
}

void test_elemgtadd() {
	const size_t vector_len = 10;
	const uint64_t a_elements[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

	struct vkhel_vector *a = vkhel_vector_create(g_ctx, vector_len);
	vkhel_vector_copy_from_host(a, a_elements);

	/* > 5, + 10 */
	const uint64_t c_expected[] = { 1, 2, 3, 4, 5, 16, 17, 18, 19, 20 };
	struct vkhel_vector *c = vkhel_vector_create(g_ctx, vector_len);
	vkhel_vector_elemgtadd(a, c, 5, 10);
	assert_vector_contents_equal(c, c_expected, vector_len);
	vkhel_vector_destroy(c);

	/* > 2, + 2 */
	const uint64_t d_expected[] = { 1, 2, 5, 6, 7, 8, 9, 10, 11, 12 };
	struct vkhel_vector *d = vkhel_vector_create(g_ctx, vector_len);
	vkhel_vector_elemgtadd(a, d, 2, 2);
	assert_vector_contents_equal(d, d_expected, vector_len);
	vkhel_vector_destroy(d);

	vkhel_vector_destroy(a);
}

void test_dup() {
	const size_t vector_len = 64;
	uint64_t elements[vector_len];
	for (size_t i = 0; i < vector_len; i++) {
		elements[i] = 5;
	}

	struct vkhel_vector *a = vkhel_vector_create(g_ctx, vector_len);
	vkhel_vector_copy_from_host(a, elements);

	struct vkhel_vector *b = vkhel_vector_dup(a);
	assert_vector_contents_equal(b, elements, vector_len);

	vkhel_vector_destroy(a);
	vkhel_vector_destroy(b);
}

int main() {
	g_ctx = vkhel_ctx_create();

	RUN_TEST(copy_from_host);
	RUN_TEST(elemadd);
	RUN_TEST(elemgtadd);
	RUN_TEST(dup);

	vkhel_ctx_destroy(g_ctx);
}
