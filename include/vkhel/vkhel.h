#ifndef VKHEL_H
#define VKHEL_H

#include <stddef.h>
#include <stdint.h>

struct vkhel_ctx;
struct vkhel_ctx *vkhel_ctx_create();
void vkhel_ctx_destroy(struct vkhel_ctx *);

struct vkhel_vector;
struct vkhel_vector *vkhel_vector_create(struct vkhel_ctx *, uint64_t length);
void vkhel_vector_destroy(struct vkhel_vector *);
struct vkhel_vector *vkhel_vector_dup(struct vkhel_vector *);
void vkhel_vector_copy_from_host(struct vkhel_vector *, const uint64_t *);
void vkhel_vector_map(struct vkhel_vector *, void **, size_t);
void vkhel_vector_unmap(struct vkhel_vector *);

void vkhel_vector_elemadd(struct vkhel_vector *a, struct vkhel_vector *b,
		struct vkhel_vector *c, uint64_t mod);

#endif
