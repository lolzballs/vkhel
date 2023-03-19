#ifndef PRIV_VECTOR_H
#define PRIV_VECTOR_H

#include <stdlib.h>
#include <vk_mem_alloc.h>

struct vkhel_ctx;

struct backing_memory {
	VmaAllocation allocation;
	VkBuffer buffer;
};

struct vkhel_vector {
	struct vkhel_ctx *ctx;

	size_t length;
	struct backing_memory device;
	struct backing_memory host;
};

void vkhel_vector_dbgprint(const struct vkhel_vector *);

#endif
