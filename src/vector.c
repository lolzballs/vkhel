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

struct vkhel_vector *vkhel_vector_create(struct vkhel_ctx *ctx, 
		uint64_t length) {
	struct vkhel_vector *ini = calloc(1, sizeof(struct vkhel_vector));
	ini->ctx = ctx;
	ini->length = length;

	VkResult res;

	VkBufferCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.flags = 0,
		.size = length * sizeof(uint64_t),
		.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
			| VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
			| VK_BUFFER_USAGE_TRANSFER_SRC_BIT
			| VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices = &ctx->vk.queue_family_index,
	};
	res = vkCreateBuffer(ctx->vk.device, &create_info, NULL, &ini->buffer);
	assert(res == VK_SUCCESS);

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(ctx->vk.device, ini->buffer,
			&memory_requirements);

	VkMemoryAllocateInfo allocate_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memory_requirements.size,
		.memoryTypeIndex = ctx->vk.host_visible_memory_index,
	};
	res = vkAllocateMemory(ctx->vk.device, &allocate_info, NULL, &ini->memory);
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

void vkhel_vector_dbgprint(struct vkhel_vector *vector) {
	struct vkhel_ctx *ctx = vector->ctx;

	uint64_t *mapped = NULL;
	vkMapMemory(ctx->vk.device, vector->memory, 0,
			vector->length * sizeof(uint64_t), 0, (void **) &mapped);
	for (size_t i = 0; i < vector->length; i++) {
		printf((i == vector->length - 1) ? ("%" PRIu64) : ("%" PRIu64 ", "),
				mapped[i]);
	}
	printf("\n");
	vkUnmapMemory(ctx->vk.device, vector->memory);
}

struct vkhel_vector *vkhel_vector_dup(struct vkhel_vector *src) {
	VkResult res;

	struct vkhel_ctx *ctx = src->ctx;
	struct vkhel_vector *new = vkhel_vector_create(src->ctx, src->length);

	VkFence fence;
	vulkan_ctx_create_fence(&ctx->vk, &fence, false);

	VkCommandBuffer cmd_buffer;
	VkCommandBufferAllocateInfo allocate_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = ctx->vk.cmd_pool,
		.commandBufferCount = 1,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	};
	res = vkAllocateCommandBuffers(ctx->vk.device, &allocate_info, &cmd_buffer);
	assert(res == VK_SUCCESS);

	VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	res = vkBeginCommandBuffer(cmd_buffer, &begin_info);
	assert(res == VK_SUCCESS);

	VkBufferCopy region = {
		.srcOffset = 0,
		.dstOffset = 0,
		.size = src->length * sizeof(uint64_t),
	};
	vkCmdCopyBuffer(cmd_buffer, src->buffer, new->buffer, 1, &region);

	res = vkEndCommandBuffer(cmd_buffer);
	assert(res == VK_SUCCESS);

	VkSubmitInfo submit = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmd_buffer,
	};
	res = vkQueueSubmit(ctx->vk.queue, 1, &submit, fence);

	vkWaitForFences(ctx->vk.device, 1, &fence, true, -1);
	vkDestroyFence(ctx->vk.device, fence, NULL);

	vkFreeCommandBuffers(ctx->vk.device, ctx->vk.cmd_pool, 1, &cmd_buffer);
	return new;
}

void vkhel_vector_copy_from_host(struct vkhel_vector *vector,
		const uint64_t *e) {
	uint64_t *mapped;
	vkhel_vector_map(vector, (void **) &mapped, vector->length);
	memcpy(mapped, e, vector->length * sizeof(uint64_t));
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

void vkhel_vector_elemfma(
		const struct vkhel_vector *a,
		const struct vkhel_vector *b,
		struct vkhel_vector *result, uint64_t multiplier, uint64_t mod) {
	assert(a->ctx == b->ctx && b->ctx == result->ctx);
	struct vkhel_ctx *ctx = a->ctx;

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
}

void vkhel_vector_elemmod(
		const struct vkhel_vector *operand,
		struct vkhel_vector *result, uint64_t mod, uint64_t q) {
	assert(operand->ctx == result->ctx);
	struct vkhel_ctx *ctx = operand->ctx;

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
}

void vkhel_vector_elemmul(
		const struct vkhel_vector *a,
		const struct vkhel_vector *b,
		struct vkhel_vector *result, uint64_t mod) {
	assert(a->ctx == b->ctx && b->ctx == result->ctx);
	struct vkhel_ctx *ctx = a->ctx;

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
}

void vkhel_vector_elemgtadd(
		const struct vkhel_vector *operand,
		struct vkhel_vector *result, uint64_t bound, uint64_t diff) {
	assert(operand->ctx == result->ctx);
	struct vkhel_ctx *ctx = result->ctx;

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
}

void vkhel_vector_elemgtsub(
		const struct vkhel_vector *operand,
		struct vkhel_vector *result,
		uint64_t bound, uint64_t diff, uint64_t mod) {
	assert(operand->ctx == result->ctx);
	struct vkhel_ctx *ctx = result->ctx;

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
}

void vkhel_vector_forward_transform(
		const struct vkhel_vector *operand,
		struct vkhel_vector *result,
		struct vkhel_ntt_tables *ntt) {
	assert(operand->ctx == result->ctx);
	struct vkhel_ctx *ctx = operand->ctx;

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

			vkDestroyDescriptorPool(ctx->vk.device, execution.descriptor_pool, NULL);

			offset += t * 2;
		}

		t /= 2;
		input = result;
	}

	vkDestroyFence(ctx->vk.device, execution_fence, NULL);
}

void vkhel_vector_inverse_transform(
		const struct vkhel_vector *operand,
		struct vkhel_vector *result,
		struct vkhel_ntt_tables *ntt) {
	assert(operand->ctx == result->ctx);
	struct vkhel_ctx *ctx = operand->ctx;

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

	vkDestroyFence(ctx->vk.device, execution_fence, NULL);
}
