#include <assert.h>
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
#include "priv/vulkan.h"

typedef void (*vulkan_kernel_init_fn)(struct vulkan_ctx *);
static const vulkan_kernel_init_fn vulkan_kernel_inits[VULKAN_KERNEL_TYPE_MAX] = {
	[VULKAN_KERNEL_TYPE_ELEMFMA] = vulkan_kernel_elemfma_init,
	[VULKAN_KERNEL_TYPE_ELEMMUL] = vulkan_kernel_elemmul_init,
	[VULKAN_KERNEL_TYPE_ELEMGTADD] = vulkan_kernel_elemgtadd_init,
	[VULKAN_KERNEL_TYPE_ELEMGTSUB] = vulkan_kernel_elemgtsub_init,
	[VULKAN_KERNEL_TYPE_NTTFWDBUTTERFLY] = vulkan_kernel_nttfwdbutterfly_init,
	[VULKAN_KERNEL_TYPE_NTTREVBUTTERFLY] = vulkan_kernel_nttrevbutterfly_init,
	[VULKAN_KERNEL_TYPE_ELEMMULCONST] = vulkan_kernel_elemmulconst_init,
	[VULKAN_KERNEL_TYPE_ELEMMODBYTWO] = vulkan_kernel_elemmodbytwo_init,
};

static void vulkan_kernel_finish(struct vulkan_ctx *vk,
		struct vulkan_kernel *kernel) {
	vkDestroyDescriptorSetLayout(vk->device, kernel->set_layout, NULL);
	vkDestroyPipelineLayout(vk->device, kernel->pipeline_layout, NULL);
	vkDestroyPipeline(vk->device, kernel->pipeline, NULL);
	vkDestroyShaderModule(vk->device, kernel->shader, NULL);
}

static uint32_t find_compute_queue(VkPhysicalDevice device) {
    uint32_t count = 8;
    VkQueueFamilyProperties properties[8];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, properties);

    int32_t candidate = -1;
    for (int32_t i = 0; i < count; i++) {
        if (properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            return i;
        }
    }
    return candidate;
}

static uint32_t find_memory_index(
		VkPhysicalDeviceMemoryProperties *memory_properties,
		VkMemoryPropertyFlags flags) {
	for (uint32_t i = 0; i < memory_properties->memoryTypeCount; i++) {
		if (memory_properties->memoryTypes[i].propertyFlags & flags) {
			return i;
		}
	}
	return -1;
}

static VkResult create_vulkan_instance(VkInstance *instance) {
    VkResult res = VK_ERROR_UNKNOWN;

    uint32_t layer_count = 32;
    VkLayerProperties layers[32];

    res = vkEnumerateInstanceLayerProperties(&layer_count, layers);
    if (res == VK_INCOMPLETE) {
        fprintf(stderr, "warning: there were more instance layers "
                "than the max supported (32)\n");
    } else if (res != VK_SUCCESS) {
        return res;
    }

    VkApplicationInfo application_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = VK_API_VERSION_1_2,
    };

    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &application_info,
    };

#ifdef VKHEL_DEBUG
    const char *validation_layer = "VK_LAYER_KHRONOS_validation";
    bool validation_layer_present = false;
    for (uint32_t layer = 0; layer < layer_count; layer++) {
        if (strncmp(layers[layer].layerName,
                    validation_layer, VK_MAX_EXTENSION_NAME_SIZE) == 0) {
            validation_layer_present = true;
            break;
        }
    }

    if (validation_layer_present) {
        create_info.enabledLayerCount = 1;
        create_info.ppEnabledLayerNames = &validation_layer;
    } else {
        fprintf(stderr, "warning: validation layer is not present\n");
    }
#endif

	create_info.enabledExtensionCount = 0;
	create_info.ppEnabledExtensionNames = NULL;

    res = vkCreateInstance(&create_info, NULL, instance);
    if (res != VK_SUCCESS) {
        return res;
    }

    return res;
}

static VkResult create_vulkan_device(struct vulkan_ctx *ini) {
    int32_t queue_index = find_compute_queue(ini->physical_device);
    float queue_priority = 1.0;
    assert(queue_index >= 0);

    /* cast to uint32_t is safe due to assert */
    ini->queue_family_index = (uint32_t) queue_index;

    VkDeviceQueueCreateInfo queue_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queue_index,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
    };

	const char *extensions[] = {};
    VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_create_info,
		.enabledExtensionCount = sizeof(extensions) / sizeof(const char *),
		.ppEnabledExtensionNames = extensions,
    };

	VkPhysicalDeviceFeatures vulkan10_features = {
		.shaderInt64 = true,
	};
	device_create_info.pEnabledFeatures = &vulkan10_features;

	VkResult res = VK_ERROR_UNKNOWN;
    res = vkCreateDevice(ini->physical_device, &device_create_info, NULL, &ini->device);
    if (res != VK_SUCCESS) {
        return res;
    }

    vkGetDeviceQueue(ini->device, queue_index, 0, &ini->queue);
    return res;
}

struct vulkan_ctx *vulkan_ctx_init(struct vulkan_ctx *ini) {
    VkResult res = VK_ERROR_UNKNOWN;

    res = create_vulkan_instance(&ini->instance);
    assert(res == VK_SUCCESS);

    uint32_t physical_device_count = 1;
    res = vkEnumeratePhysicalDevices(ini->instance, &physical_device_count,
			&ini->physical_device);
    if (res == VK_INCOMPLETE) {
        fprintf(stderr, "warning: more than one VkPhysicalDevice present\n");
        res = VK_SUCCESS;
    }
    assert(res == VK_SUCCESS);

    VkPhysicalDeviceProperties physical_device_properties;
    vkGetPhysicalDeviceProperties(ini->physical_device,
			&physical_device_properties);
    printf("using physical device 0: %s\n",
			physical_device_properties.deviceName);

    res = create_vulkan_device(ini);
    assert(res == VK_SUCCESS);

	vkGetPhysicalDeviceMemoryProperties(ini->physical_device,
			&ini->memory_properties);
	ini->device_local_memory_index = find_memory_index(&ini->memory_properties,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	ini->host_visible_memory_index = find_memory_index(&ini->memory_properties,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	VmaAllocatorCreateInfo allocator_create_info = {
		.physicalDevice = ini->physical_device,
		.device = ini->device,
		.instance = ini->instance,
	};
	res = vmaCreateAllocator(&allocator_create_info, &ini->mem_allocator);
	assert(res == VK_SUCCESS);

	VkCommandPoolCreateInfo cmd_pool_create_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
		.queueFamilyIndex = ini->queue_family_index,
	};
	vkCreateCommandPool(ini->device, &cmd_pool_create_info, NULL,
			&ini->cmd_pool);

	/* initialize kernels */
	for (size_t i = 0; i < VULKAN_KERNEL_TYPE_MAX; i++) {
		vulkan_kernel_inits[i](ini);
	}

    return ini;
}

void vulkan_ctx_finish(struct vulkan_ctx *ctx) {
	for (size_t i = 0; i < VULKAN_KERNEL_TYPE_MAX; i++) {
		vulkan_kernel_finish(ctx, &ctx->kernels[i]);
	}

	vkDestroyCommandPool(ctx->device, ctx->cmd_pool, NULL);

	vmaDestroyAllocator(ctx->mem_allocator);

    ctx->queue = VK_NULL_HANDLE;
    ctx->physical_device = VK_NULL_HANDLE;

    vkDestroyDevice(ctx->device, NULL);
    ctx->device = VK_NULL_HANDLE;

    vkDestroyInstance(ctx->instance, NULL);
    ctx->instance = VK_NULL_HANDLE;
}

void vulkan_ctx_create_fence(struct vulkan_ctx *vk, VkFence *fence,
		bool signaled) {
	VkFenceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0,
	};
	VkResult res = vkCreateFence(vk->device, &create_info, NULL, fence);
	assert(res == VK_SUCCESS);
}

void vulkan_ctx_execution_begin(struct vulkan_ctx *vk,
		struct vulkan_execution *execution, size_t set_count) {
	VkResult res = VK_ERROR_UNKNOWN;

	VkCommandBufferAllocateInfo cmd_buffer_allocate_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = vk->cmd_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
	res = vkAllocateCommandBuffers(vk->device, &cmd_buffer_allocate_info,
			&execution->cmd_buffer);
	assert(res == VK_SUCCESS);

	VkDescriptorPoolCreateInfo descriptor_pool_create_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = set_count,
		.poolSizeCount = 1,
		.pPoolSizes = &(const VkDescriptorPoolSize) {
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 3 * set_count,
		},
	};
	res = vkCreateDescriptorPool(vk->device, &descriptor_pool_create_info,
			NULL, &execution->descriptor_pool);
	assert(res == VK_SUCCESS);

	VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	res = vkBeginCommandBuffer(execution->cmd_buffer, &begin_info);
	assert(res == VK_SUCCESS);
}

void vulkan_ctx_execution_end(struct vulkan_ctx *vk,
		struct vulkan_execution *execution,
		VkFence fence) {
	VkResult res = VK_ERROR_UNKNOWN;

	res = vkEndCommandBuffer(execution->cmd_buffer);
	assert(res == VK_SUCCESS);

	VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &execution->cmd_buffer,
	};
	vkQueueSubmit(vk->queue, 1, &submit_info, fence);
}
