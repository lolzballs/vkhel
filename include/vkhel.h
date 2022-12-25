#ifndef PRIV_VKHEL_H
#define PRIV_VKHEL_H

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

struct vkhel_ctx {
	struct vulkan_ctx vk;
};

#endif
