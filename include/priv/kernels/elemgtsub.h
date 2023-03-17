#ifndef PRIV_KERNELS_ELEMGTSUB
#define PRIV_KERNELS_ELEMGTSUB

#include <stdint.h>

struct vulkan_ctx;
struct vulkan_kernel;
struct vulkan_execution;
struct vkhel_vector;

void vulkan_kernel_elemgtsub_init(struct vulkan_ctx *);
void vulkan_kernel_elemgtsub_record(
		struct vulkan_ctx *vk,
		struct vulkan_kernel *kernel,
		struct vulkan_execution *execution,
		struct vkhel_vector *result, const struct vkhel_vector *operand,
		uint64_t bound, uint64_t diff, uint64_t mod);

#endif
