#ifndef PRIV_VECTOR_H
#define PRIV_VECTOR_H

#include <stdlib.h>
#include <vulkan/vulkan.h>

struct vkhel_ctx;

struct vkhel_vector {
	struct vkhel_ctx *ctx;

	VkDeviceMemory memory;
	VkBuffer buffer;
	size_t length;
};

#endif
