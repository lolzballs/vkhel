#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "priv/kernels/elemfma.h"
#include "priv/kernels/elemmodbytwo.h"
#include "priv/kernels/elemmul.h"
#include "priv/kernels/elemmulconst.h"
#include "priv/kernels/elemgtadd.h"
#include "priv/kernels/elemgtsub.h"
#include "priv/kernels/nttfwdbutterfly.h"
#include "priv/kernels/nttrevbutterfly.h"
#include "priv/ntt_tables.h"
#include "priv/numbers.h"
#include "priv/vkhel.h"
#include "priv/vector.h"

enum backing_memory_usage {
	BACKING_MEMORY_USAGE_GPU,
	BACKING_MEMORY_USAGE_TRANSFER,
	BACKING_MEMORY_USAGE_TRANSFER_SRC,
	BACKING_MEMORY_USAGE_TRANSFER_DST,
};

static VkBufferUsageFlags get_buffer_usage_flags(
		enum backing_memory_usage usage) {
	switch (usage) {
		case BACKING_MEMORY_USAGE_GPU:
			return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
				| VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
				| VK_BUFFER_USAGE_TRANSFER_SRC_BIT
				| VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		case BACKING_MEMORY_USAGE_TRANSFER:
			return VK_BUFFER_USAGE_TRANSFER_SRC_BIT
				| VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		case BACKING_MEMORY_USAGE_TRANSFER_SRC:
			return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		case BACKING_MEMORY_USAGE_TRANSFER_DST:
			return VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}
	assert(false);
}

static VmaAllocationCreateFlags get_allocation_create_flags(
		enum backing_memory_usage usage) {
	switch (usage) {
		case BACKING_MEMORY_USAGE_GPU:
			return 0;
		case BACKING_MEMORY_USAGE_TRANSFER_SRC:
			return VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
				| VMA_ALLOCATION_CREATE_MAPPED_BIT;
		case BACKING_MEMORY_USAGE_TRANSFER:
		case BACKING_MEMORY_USAGE_TRANSFER_DST:
			return VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT
				| VMA_ALLOCATION_CREATE_MAPPED_BIT;
	}
	assert(false);
}

static VkResult allocate_backing_memory(struct vulkan_ctx *vk,
		enum backing_memory_usage usage, uint64_t size,
		struct backing_memory *memory) {
	VkResult res = VK_ERROR_UNKNOWN;

	VkBufferCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.flags = 0,
		.size = size,
		.usage = get_buffer_usage_flags(usage),
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};
	VmaAllocationCreateInfo alloc_create_info = {
		.usage = (usage == BACKING_MEMORY_USAGE_GPU 
				? VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
				: VMA_MEMORY_USAGE_AUTO_PREFER_HOST),
		.flags = get_allocation_create_flags(usage),
	};
	res = vmaCreateBuffer(vk->mem_allocator, &create_info, &alloc_create_info,
			&memory->buffer, &memory->allocation, NULL);
	if (res != VK_SUCCESS) {
		return res;
	}

	return res;
}

static void deallocate_backing_memory(struct vulkan_ctx *vk,
		struct backing_memory *memory) {
	vmaDestroyBuffer(vk->mem_allocator, memory->buffer, memory->allocation);
}

static VkResult copy_buffers(struct vulkan_ctx *vk, size_t size,
		VkBuffer from, VkBuffer to) {
	VkResult res;

	VkFence fence;
	vulkan_ctx_create_fence(vk, &fence, false);

	VkCommandBuffer cmd_buffer;
	VkCommandBufferAllocateInfo allocate_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = vk->cmd_pool,
		.commandBufferCount = 1,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	};
	res = vkAllocateCommandBuffers(vk->device, &allocate_info, &cmd_buffer);
	if (res != VK_SUCCESS) {
		return res;
	}

	VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	res = vkBeginCommandBuffer(cmd_buffer, &begin_info);
	if (res != VK_SUCCESS) {
		return res;
	}

	VkBufferCopy region = {
		.srcOffset = 0,
		.dstOffset = 0,
		.size = size,
	};
	vkCmdCopyBuffer(cmd_buffer, from, to, 1, &region);

	res = vkEndCommandBuffer(cmd_buffer);
	if (res != VK_SUCCESS) {
		return res;
	}

	VkSubmitInfo submit = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmd_buffer,
	};
	res = vkQueueSubmit(vk->queue, 1, &submit, fence);
	if (res != VK_SUCCESS) {
		return res;
	}

	vkWaitForFences(vk->device, 1, &fence, true, -1);
	vkDestroyFence(vk->device, fence, NULL);

	vkFreeCommandBuffers(vk->device, vk->cmd_pool, 1, &cmd_buffer);
	vkResetCommandPool(vk->device, vk->cmd_pool,
			VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

	return res;
}

static VkResult clear_buffer(struct vulkan_ctx *vk, VkBuffer buffer) {
	VkResult res;

	VkFence fence;
	vulkan_ctx_create_fence(vk, &fence, false);

	VkCommandBuffer cmd_buffer;
	VkCommandBufferAllocateInfo allocate_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = vk->cmd_pool,
		.commandBufferCount = 1,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	};
	res = vkAllocateCommandBuffers(vk->device, &allocate_info, &cmd_buffer);
	if (res != VK_SUCCESS) {
		return res;
	}

	VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	res = vkBeginCommandBuffer(cmd_buffer, &begin_info);
	if (res != VK_SUCCESS) {
		return res;
	}

	vkCmdFillBuffer(cmd_buffer, buffer, 0, VK_WHOLE_SIZE, 0x00000000);

	res = vkEndCommandBuffer(cmd_buffer);
	if (res != VK_SUCCESS) {
		return res;
	}

	VkSubmitInfo submit = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmd_buffer,
	};
	res = vkQueueSubmit(vk->queue, 1, &submit, fence);
	if (res != VK_SUCCESS) {
		return res;
	}

	vkWaitForFences(vk->device, 1, &fence, true, -1);
	vkDestroyFence(vk->device, fence, NULL);

	vkFreeCommandBuffers(vk->device, vk->cmd_pool, 1, &cmd_buffer);
	vkResetCommandPool(vk->device, vk->cmd_pool,
			VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

	return res;
}

struct vkhel_vector *vkhel_vector_create(struct vkhel_ctx *ctx,
		uint64_t length) {
	return vkhel_vector_create2(ctx, length, true);
}

struct vkhel_vector *vkhel_vector_create2(struct vkhel_ctx *ctx, 
		uint64_t length, bool zero) {
	struct vkhel_vector *ini = calloc(1, sizeof(struct vkhel_vector));
	ini->ctx = ctx;
	ini->length = length;

	VkResult res;

	res = allocate_backing_memory(&ctx->vk, BACKING_MEMORY_USAGE_GPU,
			length * sizeof(uint64_t), &ini->device);
	assert(res == VK_SUCCESS);

	if (zero) {
		res = clear_buffer(&ctx->vk, ini->device.buffer);
		assert(res == VK_SUCCESS);
	}

	return ini;
}

void vkhel_vector_destroy(struct vkhel_vector *vector) {
	struct vkhel_ctx *ctx = vector->ctx;
	deallocate_backing_memory(&ctx->vk, &vector->device);
	free(vector);
}

void vkhel_vector_dbgprint(const struct vkhel_vector *vector) {
	uint64_t *mapped = NULL;
	vkhel_vector_map((struct vkhel_vector *) vector, (void **) &mapped,
			vector->length * sizeof(uint64_t));
	for (size_t i = 0; i < vector->length; i++) {
		printf((i == vector->length - 1) ? ("%" PRIu64) : ("%" PRIu64 ", "),
				mapped[i]);
	}
	printf("\n");
	vkhel_vector_unmap((struct vkhel_vector *) vector);
}

struct vkhel_vector *vkhel_vector_dup(struct vkhel_vector *src) {
	VkResult res;

	struct vkhel_ctx *ctx = src->ctx;
	/* zero-initialization not required because it is about to be initialized */
	struct vkhel_vector *new = vkhel_vector_create2(src->ctx,
			src->length, false);
	res = copy_buffers(&ctx->vk, src->length * sizeof(uint64_t),
			src->device.buffer, new->device.buffer);
	assert(res == VK_SUCCESS);
	return new;
}

void vkhel_vector_copy_from_host(struct vkhel_vector *vector,
		const uint64_t *e) {
	uint64_t *mapped;
	vkhel_vector_map(vector, (void **) &mapped, vector->length * sizeof(uint64_t));
	memcpy(mapped, e, vector->length * sizeof(uint64_t));
	vkhel_vector_unmap(vector);
}

void vkhel_vector_map(struct vkhel_vector *vector, void **mem, size_t size) {
	VkResult res = VK_ERROR_UNKNOWN;
	res = allocate_backing_memory(&vector->ctx->vk,
			BACKING_MEMORY_USAGE_TRANSFER, size, &vector->host);
	assert(res == VK_SUCCESS);

	res = copy_buffers(&vector->ctx->vk, size,
			vector->device.buffer, vector->host.buffer);
	assert(res == VK_SUCCESS);

	res = vmaMapMemory(vector->ctx->vk.mem_allocator,
			vector->host.allocation, mem);
	assert(res == VK_SUCCESS);
}

void vkhel_vector_unmap(struct vkhel_vector *vector) {
	VkResult res = VK_ERROR_UNKNOWN;

	vmaUnmapMemory(vector->ctx->vk.mem_allocator,
			vector->host.allocation);

	res = copy_buffers(&vector->ctx->vk, vector->length * sizeof(uint64_t),
			vector->host.buffer, vector->device.buffer);
	assert(res == VK_SUCCESS);

	deallocate_backing_memory(&vector->ctx->vk, &vector->host);
}

void vkhel_vector_elemfma(
		const struct vkhel_vector *a,
		const struct vkhel_vector *b,
		struct vkhel_vector *result, uint64_t multiplier, uint64_t mod) {
	assert(a->ctx == b->ctx && b->ctx == result->ctx);
	struct vkhel_ctx *ctx = a->ctx;

#ifdef VKHEL_DEBUG
	printf("elemfma ("
				"multiplier: %" PRIu64
				" mod: %" PRIu64 ")\n",
				multiplier, mod);
	printf("\ta: ");
	vkhel_vector_dbgprint(a);
	printf("\tb: ");
	vkhel_vector_dbgprint(b);
#endif

	VkFence execution_fence;
	vulkan_ctx_create_fence(&ctx->vk, &execution_fence, false);

	struct vulkan_execution execution;
	vulkan_ctx_execution_begin(&ctx->vk, &execution);
	vulkan_kernel_elemfma_record(&ctx->vk,
			&ctx->vk.kernels[VULKAN_KERNEL_TYPE_ELEMFMA], &execution,
			result, a, b, multiplier, mod);
	vulkan_ctx_execution_end(&ctx->vk, &execution, execution_fence);

	vkWaitForFences(ctx->vk.device, 1, &execution_fence, true, -1);
	vkDestroyFence(ctx->vk.device, execution_fence, NULL);

	vkDestroyDescriptorPool(ctx->vk.device, execution.descriptor_pool, NULL);

	vkFreeCommandBuffers(ctx->vk.device, ctx->vk.cmd_pool, 1,
			&execution.cmd_buffer);
	vkResetCommandPool(ctx->vk.device, ctx->vk.cmd_pool,
			VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

#ifdef VKHEL_DEBUG
	printf("\tresult: ");
	vkhel_vector_dbgprint(result);
#endif
}

void vkhel_vector_elemmod(
		const struct vkhel_vector *operand,
		struct vkhel_vector *result, uint64_t mod, uint64_t q) {
	assert(operand->ctx == result->ctx);
	struct vkhel_ctx *ctx = operand->ctx;

#ifdef VKHEL_DEBUG
	printf("elemmod mod: %" PRIu64 ", q: %" PRIu64 "\n", mod, q);
	printf("\toperand: ");
	vkhel_vector_dbgprint(operand);
#endif

	VkFence execution_fence;
	vulkan_ctx_create_fence(&ctx->vk, &execution_fence, false);

	struct vulkan_execution execution;
	vulkan_ctx_execution_begin(&ctx->vk, &execution);

	if (mod == 2) {
		vulkan_kernel_elemmodbytwo_record(&ctx->vk,
				&ctx->vk.kernels[VULKAN_KERNEL_TYPE_ELEMMODBYTWO], &execution,
				result, operand, q / 2);
	} else {
		vulkan_kernel_elemgtsub_record(&ctx->vk,
				&ctx->vk.kernels[VULKAN_KERNEL_TYPE_ELEMGTSUB], &execution,
				result, operand, q / 2, q, mod);
	}

	vulkan_ctx_execution_end(&ctx->vk, &execution, execution_fence);

	vkWaitForFences(ctx->vk.device, 1, &execution_fence, true, -1);
	vkDestroyFence(ctx->vk.device, execution_fence, NULL);

	vkDestroyDescriptorPool(ctx->vk.device, execution.descriptor_pool, NULL);

	vkFreeCommandBuffers(ctx->vk.device, ctx->vk.cmd_pool, 1,
			&execution.cmd_buffer);
	vkResetCommandPool(ctx->vk.device, ctx->vk.cmd_pool,
			VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

#ifdef VKHEL_DEBUG
	printf("\tresult: ");
	vkhel_vector_dbgprint(result);
#endif
}

void vkhel_vector_elemmul(
		const struct vkhel_vector *a,
		const struct vkhel_vector *b,
		struct vkhel_vector *result, uint64_t mod) {
	assert(a->ctx == b->ctx && b->ctx == result->ctx);
	struct vkhel_ctx *ctx = a->ctx;

#ifdef VKHEL_DEBUG
	printf("elemmul mod: %" PRIu64 "\n", mod);
	printf("\ta: ");
	vkhel_vector_dbgprint(a);
	printf("\tb: ");
	vkhel_vector_dbgprint(b);
#endif

	VkFence execution_fence;
	vulkan_ctx_create_fence(&ctx->vk, &execution_fence, false);

	struct vulkan_execution execution;
	vulkan_ctx_execution_begin(&ctx->vk, &execution);
	vulkan_kernel_elemmul_record(&ctx->vk,
			&ctx->vk.kernels[VULKAN_KERNEL_TYPE_ELEMMUL], &execution,
			result, a, b, mod);
	vulkan_ctx_execution_end(&ctx->vk, &execution, execution_fence);

	vkWaitForFences(ctx->vk.device, 1, &execution_fence, true, -1);
	vkDestroyFence(ctx->vk.device, execution_fence, NULL);

	vkDestroyDescriptorPool(ctx->vk.device, execution.descriptor_pool, NULL);

	vkFreeCommandBuffers(ctx->vk.device, ctx->vk.cmd_pool, 1,
			&execution.cmd_buffer);
	vkResetCommandPool(ctx->vk.device, ctx->vk.cmd_pool,
			VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

#ifdef VKHEL_DEBUG
	printf("\tresult: ");
	vkhel_vector_dbgprint(result);
#endif
}

void vkhel_vector_elemgtadd(
		const struct vkhel_vector *operand,
		struct vkhel_vector *result, uint64_t bound, uint64_t diff) {
	assert(operand->ctx == result->ctx);
	struct vkhel_ctx *ctx = result->ctx;

#ifdef VKHEL_DEBUG
	printf("elemgtadd ("
				"bound: %" PRIu64
				" diff: %" PRIu64 ")\n",
				bound, diff);
	printf("\toperand: ");
	vkhel_vector_dbgprint(operand);
#endif

	VkFence execution_fence;
	vulkan_ctx_create_fence(&ctx->vk, &execution_fence, false);

	struct vulkan_execution execution;
	vulkan_ctx_execution_begin(&ctx->vk, &execution);
	vulkan_kernel_elemgtadd_record(&ctx->vk,
			&ctx->vk.kernels[VULKAN_KERNEL_TYPE_ELEMGTADD], &execution,
			result, operand, bound, diff);
	vulkan_ctx_execution_end(&ctx->vk, &execution, execution_fence);

	vkWaitForFences(ctx->vk.device, 1, &execution_fence, true, -1);
	vkDestroyFence(ctx->vk.device, execution_fence, NULL);

	vkDestroyDescriptorPool(ctx->vk.device, execution.descriptor_pool, NULL);

	vkFreeCommandBuffers(ctx->vk.device, ctx->vk.cmd_pool, 1,
			&execution.cmd_buffer);
	vkResetCommandPool(ctx->vk.device, ctx->vk.cmd_pool,
			VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

#ifdef VKHEL_DEBUG
	printf("\tresult: ");
	vkhel_vector_dbgprint(result);
#endif
}

void vkhel_vector_elemgtsub(
		const struct vkhel_vector *operand,
		struct vkhel_vector *result,
		uint64_t bound, uint64_t diff, uint64_t mod) {
	assert(operand->ctx == result->ctx);
	struct vkhel_ctx *ctx = result->ctx;

#ifdef VKHEL_DEBUG
	printf("elemgtsub ("
				"bound: %" PRIu64
				" diff: %" PRIu64
				" mod: %" PRIu64 ")\n",
				bound, diff, mod);
	printf("\toperand: ");
	vkhel_vector_dbgprint(operand);
#endif

	VkFence execution_fence;
	vulkan_ctx_create_fence(&ctx->vk, &execution_fence, false);

	struct vulkan_execution execution;
	vulkan_ctx_execution_begin(&ctx->vk, &execution);
	vulkan_kernel_elemgtsub_record(&ctx->vk,
			&ctx->vk.kernels[VULKAN_KERNEL_TYPE_ELEMGTSUB], &execution,
			result, operand, bound, diff, mod);
	vulkan_ctx_execution_end(&ctx->vk, &execution, execution_fence);

	vkWaitForFences(ctx->vk.device, 1, &execution_fence, true, -1);
	vkDestroyFence(ctx->vk.device, execution_fence, NULL);

	vkDestroyDescriptorPool(ctx->vk.device, execution.descriptor_pool, NULL);

	vkFreeCommandBuffers(ctx->vk.device, ctx->vk.cmd_pool, 1,
			&execution.cmd_buffer);
	vkResetCommandPool(ctx->vk.device, ctx->vk.cmd_pool,
			VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

#ifdef VKHEL_DEBUG
	printf("\tresult: ");
	vkhel_vector_dbgprint(result);
#endif
}

void vkhel_vector_forward_transform(
		const struct vkhel_vector *operand,
		struct vkhel_vector *result,
		struct vkhel_ntt_tables *ntt) {
	assert(operand->ctx == result->ctx);
	struct vkhel_ctx *ctx = operand->ctx;

#ifdef VKHEL_DEBUG
	printf("forward transform ("
				"degree: %" PRIu64
				" mod: %" PRIu64
				" omega: %" PRIu64 ")\n",
				ntt->n, ntt->q, ntt->w);
	printf("\toperand: ");
	vkhel_vector_dbgprint(operand);
#endif

	VkFence execution_fence;
	vulkan_ctx_create_fence(&ctx->vk, &execution_fence, false);

	const struct vkhel_vector *input = operand;

	struct vulkan_execution execution;
	uint64_t t = ntt->n / 2;
	for (uint64_t m = 1; m < ntt->n; m *= 2) {
		uint64_t offset = 0;
		for (size_t i = 0; i < m; i++) {
			const uint64_t W = ntt->roots_of_unity[m + i];

			vulkan_ctx_execution_begin(&ctx->vk, &execution);
			vulkan_kernel_nttfwdbutterfly_record(&ctx->vk,
					&ctx->vk.kernels[VULKAN_KERNEL_TYPE_NTTFWDBUTTERFLY],
					&execution, ntt, ntt->q, W, t, offset, input, result);
			vulkan_ctx_execution_end(&ctx->vk, &execution, execution_fence);
			vkWaitForFences(ctx->vk.device, 1, &execution_fence, true, -1);
			vkResetFences(ctx->vk.device, 1, &execution_fence);

			vkFreeCommandBuffers(ctx->vk.device, ctx->vk.cmd_pool, 1,
					&execution.cmd_buffer);
			vkResetCommandPool(ctx->vk.device, ctx->vk.cmd_pool,
					VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

			vkDestroyDescriptorPool(ctx->vk.device, execution.descriptor_pool, NULL);

			offset += t * 2;
		}

		t /= 2;
		input = result;
	}

	vkDestroyFence(ctx->vk.device, execution_fence, NULL);

#ifdef VKHEL_DEBUG
	printf("\tresult: ");
	vkhel_vector_dbgprint(result);
#endif
}

void vkhel_vector_inverse_transform(
		const struct vkhel_vector *operand,
		struct vkhel_vector *result,
		struct vkhel_ntt_tables *ntt) {
	assert(operand->ctx == result->ctx);
	struct vkhel_ctx *ctx = operand->ctx;

#ifdef VKHEL_DEBUG
	printf("inverse transform ("
				"degree: %" PRIu64
				" mod: %" PRIu64
				" omega: %" PRIu64 ")\n",
				ntt->n, ntt->q, ntt->w);
	printf("\toperand: ");
	vkhel_vector_dbgprint(operand);
#endif

	VkFence execution_fence;
	vulkan_ctx_create_fence(&ctx->vk, &execution_fence, false);

	const struct vkhel_vector *input = operand;

	struct vulkan_execution execution;
	uint64_t t = 1;
	for (uint64_t m = ntt->n / 2; m >= 1; m /= 2) {
		uint64_t offset = 0;
		for (size_t i = 0; i < m; i++) {
			const uint64_t W = ntt->inv_roots_of_unity[m + i];

			vulkan_ctx_execution_begin(&ctx->vk, &execution);
			vulkan_kernel_nttrevbutterfly_record(&ctx->vk,
					&ctx->vk.kernels[VULKAN_KERNEL_TYPE_NTTREVBUTTERFLY],
					&execution, ntt, ntt->q, W, t, offset, input, result);
			vulkan_ctx_execution_end(&ctx->vk, &execution, execution_fence);

			vkWaitForFences(ctx->vk.device, 1, &execution_fence, true, -1);
			vkResetFences(ctx->vk.device, 1, &execution_fence);

			vkDestroyDescriptorPool(ctx->vk.device, execution.descriptor_pool, NULL);

			vkFreeCommandBuffers(ctx->vk.device, ctx->vk.cmd_pool, 1,
					&execution.cmd_buffer);
			vkResetCommandPool(ctx->vk.device, ctx->vk.cmd_pool,
					VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

			offset += t * 2;
		}

		t *= 2;
		input = result;
	}

	/* need to adjust all elements by inv(N) */
	const uint64_t inv_n = nt_inverse_mod(ntt->n, ntt->q);

	vulkan_ctx_execution_begin(&ctx->vk, &execution);
	vulkan_kernel_elemmulconst_record(&ctx->vk, 
			&ctx->vk.kernels[VULKAN_KERNEL_TYPE_ELEMMULCONST],
			&execution, result, result, inv_n, ntt->q);
	vulkan_ctx_execution_end(&ctx->vk, &execution, execution_fence);

	vkWaitForFences(ctx->vk.device, 1, &execution_fence, true, -1);
	vkResetFences(ctx->vk.device, 1, &execution_fence);

	vkDestroyDescriptorPool(ctx->vk.device, execution.descriptor_pool, NULL);

	vkFreeCommandBuffers(ctx->vk.device, ctx->vk.cmd_pool, 1,
			&execution.cmd_buffer);
	vkResetCommandPool(ctx->vk.device, ctx->vk.cmd_pool,
			VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

	vkDestroyFence(ctx->vk.device, execution_fence, NULL);

#ifdef VKHEL_DEBUG
	printf("\tresult: ");
	vkhel_vector_dbgprint(operand);
#endif
}
