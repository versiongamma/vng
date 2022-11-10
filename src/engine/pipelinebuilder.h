#pragma once

#include <utils/types.h>
#include <vector>

struct PipelineBuilder {
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages {};
	VkPipelineVertexInputStateCreateInfo vertexInputInfo {};
	VkPipelineInputAssemblyStateCreateInfo inputAssembly {};

	std::vector<VkDynamicState> dynamicStateEnables {};
	VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
	VkPipelineRasterizationStateCreateInfo rasterizer {};
	VkPipelineColorBlendAttachmentState colorBlendAttachment {};
	VkPipelineMultisampleStateCreateInfo multisampling {};
	VkPipelineLayout pipelineLayout {};
	VkPipelineDepthStencilStateCreateInfo depthStencil {};

	VkPipeline buildPipeline(VkDevice& device, VkRenderPass pass);
};