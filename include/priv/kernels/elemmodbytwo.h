#ifndef PRIV_KERNELS_ELEMMODBYTWO
#define PRIV_KERNELS_ELEMMODBYTWO

#include <stdint.h>

struct vulkan_ctx;
struct vulkan_kernel;
struct vulkan_execution;
struct vkhel_vector;

void vulkan_kernel_elemmodbytwo_init(struct vulkan_ctx *);
void vulkan_kernel_elemmodbytwo_record(
		struct vulkan_ctx *vk,
		struct vulkan_kernel *kernel,
		struct vulkan_execution *execution,
		struct vkhel_vector *result,
		struct vkhel_vector *operand,
		uint64_t signed_bound);

#endif
