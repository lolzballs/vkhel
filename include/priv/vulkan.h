#ifndef PRIV_VULKAN_H
#define PRIV_VULKAN_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

struct vulkan_ctx {
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;

    uint32_t queue_family_index;
    VkQueue queue;

	VkPhysicalDeviceMemoryProperties memory_properties;
	uint32_t host_visible_memory_index;
	uint32_t device_local_memory_index;
};

struct vulkan_ctx *vulkan_ctx_init(struct vulkan_ctx *ini);
void vulkan_ctx_finish(struct vulkan_ctx *ctx);
void vulkan_ctx_create_fence(struct vulkan_ctx *vk, VkFence *fence,
		bool signaled);

#endif
