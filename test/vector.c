#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <vkhel.h>

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

void test_elemfma() {
	const size_t vector_len = 8;
	const uint64_t a_elements[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	const uint64_t b_elements[] = { 9, 10, 11, 12, 13, 14, 15, 16 };
	const uint64_t modulus = 769;

	struct vkhel_vector *a = vkhel_vector_create(g_ctx, vector_len);
	vkhel_vector_copy_from_host(a, a_elements);

	struct vkhel_vector *b = vkhel_vector_create(g_ctx, vector_len);
	vkhel_vector_copy_from_host(b, b_elements);

	{
		const uint64_t multiplier = 1;

		const uint64_t c_expected[] = { 10, 12, 14, 16, 18, 20, 22, 24 };
		struct vkhel_vector *c = vkhel_vector_create(g_ctx, vector_len);
		vkhel_vector_elemfma(a, b, c, multiplier, modulus);
		assert_vector_contents_equal(c, c_expected, vector_len);
		vkhel_vector_destroy(c);
	}

	{
		const uint64_t multiplier = 2;

		const uint64_t c_expected[] = { 11, 14, 17, 20, 23, 26, 29, 32 };
		struct vkhel_vector *c = vkhel_vector_create(g_ctx, vector_len);
		vkhel_vector_elemfma(a, b, c, multiplier, modulus);
		assert_vector_contents_equal(c, c_expected, vector_len);
		vkhel_vector_destroy(c);
	}

	{
		const uint64_t multiplier = 100;

		const uint64_t c_expected[] = { 109, 210, 311, 412, 513, 614, 715, 47 };
		struct vkhel_vector *c = vkhel_vector_create(g_ctx, vector_len);
		vkhel_vector_elemfma(a, b, c, multiplier, modulus);
		assert_vector_contents_equal(c, c_expected, vector_len);
		vkhel_vector_destroy(c);
	}

	vkhel_vector_destroy(a);
	vkhel_vector_destroy(b);
}

void test_elemmul() {
	const size_t vector_len = 9;

	{
		const uint64_t modulus = 769;
		const uint64_t a_elements[] = { 1, 2, 3, 1, 1, 1, 0, 1, 0 };
		const uint64_t b_elements[] = { 1, 1, 1, 1, 2, 3, 1, 0, 0 };

		struct vkhel_vector *a = vkhel_vector_create(g_ctx, vector_len);
		vkhel_vector_copy_from_host(a, a_elements);

		struct vkhel_vector *b = vkhel_vector_create(g_ctx, vector_len);
		vkhel_vector_copy_from_host(b, b_elements);

		const uint64_t c_expected[] = { 1, 2, 3, 1, 2, 3, 0, 0, 0 };
		struct vkhel_vector *c = vkhel_vector_create(g_ctx, vector_len);
		vkhel_vector_elemmul(a, b, c, modulus);
		assert_vector_contents_equal(c, c_expected, vector_len);
		vkhel_vector_destroy(c);

		vkhel_vector_destroy(a);
		vkhel_vector_destroy(b);
	}

	{
		const uint64_t modulus = 1125891450734593;
		const uint64_t a_elements[] = {
			706712574074152, 943467560561867, 1115920708919443,
			515713505356094, 525633777116309, 910766532971356,
			757086506562426, 799841520990167, 1,
		};
		const uint64_t b_elements[] = {
			515910833966633, 96924929169117, 537587376997453,
			41829060600750, 205864998008014, 463185427411646,
			965818279134294, 1075778049568657, 1,
		};

		struct vkhel_vector *a = vkhel_vector_create(g_ctx, vector_len);
		vkhel_vector_copy_from_host(a, a_elements);

		struct vkhel_vector *b = vkhel_vector_create(g_ctx, vector_len);
		vkhel_vector_copy_from_host(b, b_elements);

		const uint64_t c_expected[] = {
			231838787758587, 618753612121218, 1116345967490421,
			409735411065439, 25680427818594, 950138933882289,
			554128714280822, 1465109636753, 1,
		};
		struct vkhel_vector *c = vkhel_vector_create(g_ctx, vector_len);
		vkhel_vector_elemmul(a, b, c, modulus);
		assert_vector_contents_equal(c, c_expected, vector_len);
		vkhel_vector_destroy(c);

		vkhel_vector_destroy(a);
		vkhel_vector_destroy(b);
	}
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

void test_elemgtsub() {
	const size_t vector_len = 8;
	const uint64_t a_elements[] = { 1, 2, 3, 4, 5, 6, 7, 8 };

	struct vkhel_vector *a = vkhel_vector_create(g_ctx, vector_len);
	vkhel_vector_copy_from_host(a, a_elements);

	/* > 5, - 7 (mod 10) */
	const uint64_t c_expected[] = { 1, 2, 3, 4, 5, 9, 0, 1 };
	struct vkhel_vector *c = vkhel_vector_create(g_ctx, vector_len);
	vkhel_vector_elemgtsub(a, c, 5, 7, 10);
	assert_vector_contents_equal(c, c_expected, vector_len);
	vkhel_vector_destroy(c);

	/* > 1, - 3 (mod 5) */
	const uint64_t d_expected[] = { 1, 4, 0, 1, 2, 3, 4, 0 };
	struct vkhel_vector *d = vkhel_vector_create(g_ctx, vector_len);
	vkhel_vector_elemgtsub(a, d, 1, 3, 5);
	assert_vector_contents_equal(d, d_expected, vector_len);
	vkhel_vector_destroy(d);

	/* > 1, - 8 (mod 5) */
	struct vkhel_vector *e = vkhel_vector_create(g_ctx, vector_len);
	vkhel_vector_elemgtsub(a, e, 1, 8, 5);
	assert_vector_contents_equal(e, d_expected, vector_len);
	vkhel_vector_destroy(e);

	vkhel_vector_destroy(a);
}

void test_elemmod() {
	const size_t vector_len = 8;
	const uint64_t a_elements[] = { 1, 2, 3, 4, 5, 6, 7, 8 };

	struct vkhel_vector *a = vkhel_vector_create(g_ctx, vector_len);
	vkhel_vector_copy_from_host(a, a_elements);

	const uint64_t c_expected[] = { 1, 0, 1, 0, 1, 0, 1, 0 };
	struct vkhel_vector *c = vkhel_vector_create(g_ctx, vector_len);
	vkhel_vector_elemmod(a, c, 2);
	assert_vector_contents_equal(c, c_expected, vector_len);
	vkhel_vector_destroy(c);

	const uint64_t d_expected[] = { 1, 2, 0, 1, 2, 0, 1, 2 };
	struct vkhel_vector *d = vkhel_vector_create(g_ctx, vector_len);
	vkhel_vector_elemmod(a, d, 3);
	assert_vector_contents_equal(d, d_expected, vector_len);
	vkhel_vector_destroy(d);

	vkhel_vector_destroy(a);
}

void test_forward_transform() {
	const size_t vector_len = 4;
	const uint64_t operand[] = { 94, 109, 11, 18 };
	const uint64_t expected[] = { 82, 2, 81, 98 };

	struct vkhel_ntt_tables *ntt_tables = vkhel_ntt_tables_create(
			vector_len, 113, 18);

	struct vkhel_vector *a = vkhel_vector_create(g_ctx, vector_len);
	vkhel_vector_copy_from_host(a, operand);

	vkhel_vector_forward_transform(a, ntt_tables);
	assert_vector_contents_equal(a, expected, vector_len);

	vkhel_vector_destroy(a);

	vkhel_ntt_tables_destroy(ntt_tables);
}

void test_inverse_transform() {
	const size_t vector_len = 4;
	const uint64_t operand[] = { 82, 2, 81, 98 };
	const uint64_t expected[] = { 94, 109, 11, 18 };

	struct vkhel_ntt_tables *ntt_tables = vkhel_ntt_tables_create(
			vector_len, 113, 18);

	struct vkhel_vector *a = vkhel_vector_create(g_ctx, vector_len);
	vkhel_vector_copy_from_host(a, operand);

	vkhel_vector_inverse_transform(a, ntt_tables);
	assert_vector_contents_equal(a, expected, vector_len);

	vkhel_vector_destroy(a);

	vkhel_ntt_tables_destroy(ntt_tables);
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
	RUN_TEST(dup);
	RUN_TEST(elemfma);
	RUN_TEST(elemmod);
	RUN_TEST(elemmul);
	RUN_TEST(elemgtadd);
	RUN_TEST(elemgtsub);
	RUN_TEST(forward_transform);
	RUN_TEST(inverse_transform);

	vkhel_ctx_destroy(g_ctx);
}
