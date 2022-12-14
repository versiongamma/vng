#include <utils/types.h>

namespace vkinit {
	VkCommandPoolCreateInfo commandPoolCreateInfo(
		uint32_t queueFamilyIndex, 
		VkCommandPoolCreateFlags flags = 0);

	VkCommandBufferAllocateInfo commandBufferAllocateInfo(
		VkCommandPool pool, 
		uint32_t count = 1, 
		VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo(
		VkShaderStageFlagBits stage,
		VkShaderModule shaderModule);

	VkImageCreateInfo imageCreateInfo(
		VkFormat format, 
		VkImageUsageFlags usageFlags, 
		VkExtent3D extent);

	VkImageViewCreateInfo imageviewCreateInfo(
		VkFormat format, 
		VkImage image, 
		VkImageAspectFlags aspectFlags);

	VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo(bool bDepthTest, bool bDepthWrite, VkCompareOp compareOp);

	VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = 0);
	VkSemaphoreCreateInfo semaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0);
	VkCommandBufferBeginInfo commandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0);
	VkSubmitInfo submitInfo(VkCommandBuffer* cmd);
	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo();
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo(VkPrimitiveTopology topology);
	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo(VkPolygonMode polygonMode);
	VkPipelineMultisampleStateCreateInfo multisamplingStateCreateInfo();
	VkPipelineColorBlendAttachmentState colorBlendAttachmentState();
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo();

	VkDescriptorSetLayoutBinding descriptorsetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding);
	VkWriteDescriptorSet writeDescriptorBuffer(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo* bufferInfo, uint32_t binding);
	VkSamplerCreateInfo samplerCreateInfo(VkFilter filters, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);
	VkWriteDescriptorSet writeDescriptorImage(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo* imageInfo, uint32_t binding);
}