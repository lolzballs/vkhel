#include <assert.h>
#include "priv/kernels/elemadd.h"
#include "priv/vkhel.h"
#include "priv/vector.h"

struct vkhel_vector *vkhel_vector_create(struct vkhel_ctx *ctx, 
		uint64_t length) {
	struct vkhel_vector *ini = calloc(1, sizeof(struct vkhel_vector));
	ini->ctx = ctx;
	ini->length = length;

	VkResult res;

	VkMemoryAllocateInfo allocate_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = length * sizeof(uint64_t),
		.memoryTypeIndex = ctx->vk.host_visible_memory_index,
	};
	res = vkAllocateMemory(ctx->vk.device, &allocate_info, NULL, &ini->memory);
	assert(res == VK_SUCCESS);

	VkBufferCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.flags = 0,
		.size = length * sizeof(uint64_t),
		.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices = &ctx->vk.queue_family_index,
	};
	res = vkCreateBuffer(ctx->vk.device, &create_info, NULL, &ini->buffer);
	assert(res == VK_SUCCESS);

	vkBindBufferMemory(ctx->vk.device, ini->buffer, ini->memory, 0);

	return ini;
}

void vkhel_vector_destroy(struct vkhel_vector *vector) {
	struct vkhel_ctx *ctx = vector->ctx;
	vkDestroyBuffer(ctx->vk.device, vector->buffer, NULL);
	vkFreeMemory(ctx->vk.device, vector->memory, NULL);
	free(vector);
}

void vkhel_vector_copy_from_host(struct vkhel_vector *vector, uint64_t *e) {
	uint64_t *mapped;
	vkhel_vector_map(vector, (void **) &mapped, vector->length);
	for (size_t i = 0; i < vector->length; i++) {
		mapped[i] = e[i];
	}
	vkhel_vector_unmap(vector);
}

void vkhel_vector_map(struct vkhel_vector *vector, void **mem, size_t size) {
	VkResult res = VK_ERROR_UNKNOWN;
	res = vkMapMemory(vector->ctx->vk.device, vector->memory, 0, size, 0, mem);
	assert(res == VK_SUCCESS);
}

void vkhel_vector_unmap(struct vkhel_vector *vector) {
	vkUnmapMemory(vector->ctx->vk.device, vector->memory);
}

void vkhel_vector_elemadd(struct vkhel_vector *a, struct vkhel_vector *b,
		struct vkhel_vector *c, uint64_t mod) {
	assert(a->ctx == b->ctx && b->ctx == c->ctx);
	struct vkhel_ctx *ctx = a->ctx;

	VkFence execution_fence;
	vulkan_ctx_create_fence(&ctx->vk, &execution_fence, false);

	struct vulkan_execution execution;
	vulkan_ctx_execution_begin(&ctx->vk, &execution);
	vulkan_kernel_elemadd_record(&ctx->vk,
			&ctx->vk.kernels[VULKAN_KERNEL_TYPE_ELEMADD], &execution,
			a, b, c, mod);
	vulkan_ctx_execution_end(&ctx->vk, &execution, execution_fence);

	vkWaitForFences(ctx->vk.device, 1, &execution_fence, true, -1);
	vkDestroyFence(ctx->vk.device, execution_fence, NULL);

	vkDestroyDescriptorPool(ctx->vk.device, execution.descriptor_pool, NULL);
}
