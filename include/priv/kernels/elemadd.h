#ifndef PRIV_KERNELS_ELEMADD
#define PRIV_KERNELS_ELEMADD

#include <stdint.h>

struct vulkan_ctx;
struct vulkan_kernel;
struct vulkan_execution;
struct vkhel_vector;

void vulkan_kernel_elemadd_init(struct vulkan_ctx *);
void vulkan_kernel_elemadd_record(
		struct vulkan_ctx *vk,
		struct vulkan_kernel *kernel,
		struct vulkan_execution *execution,
		struct vkhel_vector *a, struct vkhel_vector *b,
		struct vkhel_vector *c, uint64_t mod);

#endif
