#include <assert.h>
#include "priv/vkhel.h"
#include "elemgtadd.comp.h"

#define SHADER_LOCAL_SIZE_X 64

struct push_constants {
	uint64_t length;
	uint64_t bound;
	uint64_t diff;
};

static const VkPushConstantRange push_constants_range = {
	.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
	.offset = 0,
	.size = sizeof(struct push_constants),
};

static const VkShaderModuleCreateInfo shader_module_create_info = {
	.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
	.pCode = elemgtadd_comp_data,
	.codeSize = sizeof(elemgtadd_comp_data),
};

static const VkDescriptorSetLayoutBinding descriptor_bindings[] = {
	/* input buffers */
	{
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
	},
	/* output buffer */
	{
		.binding = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
	},
};

static const VkDescriptorSetLayoutCreateInfo descriptor_set_create_info = {
	.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
	.bindingCount = sizeof(descriptor_bindings) 
		/ sizeof(VkDescriptorSetLayoutBinding),
	.pBindings = descriptor_bindings,
};

void vulkan_kernel_elemgtadd_init(struct vulkan_ctx *vk) {
	struct vulkan_kernel *ini = &vk->kernels[VULKAN_KERNEL_TYPE_ELEMGTADD];
	VkResult res = VK_ERROR_UNKNOWN;

	res = vkCreateShaderModule(vk->device, &shader_module_create_info, NULL,
			&ini->shader);
	assert(res == VK_SUCCESS);

	res = vkCreateDescriptorSetLayout(vk->device, &descriptor_set_create_info,
			NULL, &ini->set_layout);
	assert(res == VK_SUCCESS);

	VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &ini->set_layout,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &push_constants_range,
	};
	res = vkCreatePipelineLayout(vk->device, &pipeline_layout_create_info,
			NULL, &ini->pipeline_layout);
	assert(res == VK_SUCCESS);

	VkComputePipelineCreateInfo pipeline_create_info = {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.stage = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
			.module = ini->shader,
			.pName = "main",
		},
		.layout = ini->pipeline_layout,
	};
	res = vkCreateComputePipelines(vk->device, NULL, 1, &pipeline_create_info,
			NULL, &ini->pipeline);
	assert(res == VK_SUCCESS);
}

void vulkan_kernel_elemgtadd_record(
		struct vulkan_ctx *vk,
		struct vulkan_kernel *kernel,
		struct vulkan_execution *execution,
		struct vkhel_vector *result, struct vkhel_vector *operand,
		uint64_t bound, uint64_t diff) {
	VkResult res = VK_ERROR_UNKNOWN;

	VkDescriptorSet descriptor_set;
	VkDescriptorSetAllocateInfo descriptor_allocate_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = execution->descriptor_pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &kernel->set_layout,
	};
	res = vkAllocateDescriptorSets(vk->device, &descriptor_allocate_info, 
			&descriptor_set);
	assert(res == VK_SUCCESS);

	const VkWriteDescriptorSet write_descriptor_sets[] = {
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptor_set,
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.pBufferInfo = (const VkDescriptorBufferInfo[]) {
				{
					.buffer = operand->buffer,
					.offset = 0,
					.range = operand->length * sizeof(uint64_t),
				},
			},
		},
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptor_set,
			.dstBinding = 1,
			.dstArrayElement = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.pBufferInfo = (const VkDescriptorBufferInfo[]) {
				{
					.buffer = result->buffer,
					.offset = 0,
					.range = result->length * sizeof(uint64_t),
				},
			},
		},
	};
	vkUpdateDescriptorSets(vk->device,
			sizeof(write_descriptor_sets) / sizeof(VkWriteDescriptorSet),
			write_descriptor_sets, 0, NULL);

	vkCmdBindPipeline(execution->cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
			kernel->pipeline);
	vkCmdBindDescriptorSets(execution->cmd_buffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			kernel->pipeline_layout, 0, 1, &descriptor_set, 0, NULL);

	const struct push_constants push = {
		.length = result->length,
		.bound = bound,
		.diff = diff,
	};
	vkCmdPushConstants(execution->cmd_buffer, kernel->pipeline_layout,
			VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(struct push_constants),
			&push);

	vkCmdDispatch(execution->cmd_buffer,
			DIV_CEIL(result->length, SHADER_LOCAL_SIZE_X), 1, 1);
}
