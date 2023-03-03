#ifndef VKHEL_H
#define VKHEL_H

#include <stddef.h>
#include <stdint.h>

struct vkhel_ctx;
struct vkhel_ctx *vkhel_ctx_create();
void vkhel_ctx_destroy(struct vkhel_ctx *);

struct ntt_tables;
struct ntt_tables *ntt_tables_create(uint64_t n, uint64_t q, uint64_t w);
void ntt_tables_destroy(struct ntt_tables *);

struct vkhel_vector;
struct vkhel_vector *vkhel_vector_create(struct vkhel_ctx *, uint64_t length);
void vkhel_vector_destroy(struct vkhel_vector *);
struct vkhel_vector *vkhel_vector_dup(struct vkhel_vector *);
void vkhel_vector_copy_from_host(struct vkhel_vector *, const uint64_t *);
void vkhel_vector_map(struct vkhel_vector *, void **, size_t);
void vkhel_vector_unmap(struct vkhel_vector *);

void vkhel_vector_elemfma(struct vkhel_vector *a, struct vkhel_vector *b,
		struct vkhel_vector *result,
		uint64_t multiplier, uint64_t mod);
void vkhel_vector_elemmul(struct vkhel_vector *a, struct vkhel_vector *b,
		struct vkhel_vector *result, uint64_t mod);
void vkhel_vector_elemgtadd(struct vkhel_vector *operand,
		struct vkhel_vector *result,
		uint64_t bound, uint64_t diff);
void vkhel_vector_elemgtsub(struct vkhel_vector *operand,
		struct vkhel_vector *result,
		uint64_t bound, uint64_t diff, uint64_t mod);
void vkhel_vector_forward_transform(struct vkhel_vector *operand,
		struct ntt_tables *ntt);

#endif
