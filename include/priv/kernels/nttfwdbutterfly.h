#ifndef PRIV_KERNELS_NTTFWDBUTTERFLY
#define PRIV_KERNELS_NTTFWDBUTTERFLY

#include <stdint.h>

struct vulkan_ctx;
struct vulkan_kernel;
struct vulkan_execution;
struct vkhel_vector;
struct ntt_tables;

void vulkan_kernel_nttfwdbutterfly_init(struct vulkan_ctx *);
void vulkan_kernel_nttfwdbutterfly_record(
		struct vulkan_ctx *vk,
		struct vulkan_kernel *kernel,
		struct vulkan_execution *execution,
		struct ntt_tables *ntt,
		uint64_t mod, uint64_t root_of_unity,
		uint64_t transform_size, uint64_t offset,
		struct vkhel_vector *operand);

#endif
