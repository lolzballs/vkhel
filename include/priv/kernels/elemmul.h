#ifndef PRIV_KERNELS_ELEMMUL
#define PRIV_KERNELS_ELEMMUL

#include <stdint.h>

struct vulkan_ctx;
struct vulkan_kernel;
struct vulkan_execution;
struct vkhel_vector;

void vulkan_kernel_elemmul_init(struct vulkan_ctx *);
void vulkan_kernel_elemmul_record(
		struct vulkan_ctx *vk,
		struct vulkan_kernel *kernel,
		struct vulkan_execution *execution,
		struct vkhel_vector *result,
		struct vkhel_vector *a, struct vkhel_vector *b, uint64_t mod);

#endif
