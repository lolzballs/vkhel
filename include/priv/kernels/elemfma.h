#ifndef PRIV_KERNELS_ELEMFMA
#define PRIV_KERNELS_ELEMFMA

#include <stdint.h>

struct vulkan_ctx;
struct vulkan_kernel;
struct vulkan_execution;
struct vkhel_vector;

void vulkan_kernel_elemfma_init(struct vulkan_ctx *);
void vulkan_kernel_elemfma_record(
		struct vulkan_ctx *vk,
		struct vulkan_kernel *kernel,
		struct vulkan_execution *execution,
		struct vkhel_vector *result,
		const struct vkhel_vector *a, const struct vkhel_vector *b,
		uint64_t multiplier, uint64_t mod);

#endif
