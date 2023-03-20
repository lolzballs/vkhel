#ifndef PRIV_VULKAN_H
#define PRIV_VULKAN_H

#include <stdbool.h>
#include <vk_mem_alloc.h>

struct vulkan_ctx;

struct vulkan_kernel {
	VkDescriptorSetLayout set_layout;
	VkPipelineLayout pipeline_layout;
	VkPipeline pipeline;
	VkShaderModule shader;
};

enum vulkan_kernel_type {
	VULKAN_KERNEL_TYPE_ELEMFMA			= 0,
	VULKAN_KERNEL_TYPE_ELEMMUL			= 1,
	VULKAN_KERNEL_TYPE_ELEMGTADD		= 2,
	VULKAN_KERNEL_TYPE_ELEMGTSUB		= 3,
	VULKAN_KERNEL_TYPE_NTTFWDBUTTERFLY	= 4,
	VULKAN_KERNEL_TYPE_NTTREVBUTTERFLY	= 5,
	VULKAN_KERNEL_TYPE_ELEMMULCONST		= 6,
	VULKAN_KERNEL_TYPE_ELEMMODBYTWO		= 7,
	VULKAN_KERNEL_TYPE_MAX,
};

struct vulkan_execution {
	VkDescriptorPool descriptor_pool;
	VkCommandBuffer cmd_buffer;
};

struct vulkan_ctx {
	VkInstance instance;
	VkPhysicalDevice physical_device;
	VkDevice device;

	uint32_t queue_family_index;
	VkQueue queue;

	VkPhysicalDeviceMemoryProperties memory_properties;
	uint32_t host_visible_memory_index;
	uint32_t device_local_memory_index;
	VmaAllocator mem_allocator;

	VkCommandPool cmd_pool;
	struct vulkan_kernel kernels[VULKAN_KERNEL_TYPE_MAX];
};

struct vulkan_ctx *vulkan_ctx_init(struct vulkan_ctx *ini);
void vulkan_ctx_finish(struct vulkan_ctx *ctx);
void vulkan_ctx_create_fence(struct vulkan_ctx *vk, VkFence *fence,
		bool signaled);
void vulkan_ctx_execution_begin(struct vulkan_ctx *vk,
		struct vulkan_execution *execution, size_t set_count);
void vulkan_ctx_execution_end(struct vulkan_ctx *vk,
		struct vulkan_execution *execution, VkFence fence);

#endif
