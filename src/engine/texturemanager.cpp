#include "texturemanager.h"

#include "vulkankinitialisers.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <iostream>

#include "renderer.h"
#include "console.h"

void TextureManager::init(Console& console) {
	this->console = &console;
}

Texture* TextureManager::loadTexture(Renderer& renderer, const char* file) {
	auto pair = loadedTextures.find(file);
	if (pair != loadedTextures.end()) {
		return &(*pair).second;
	}

	int32_t textureWidth, textureHeight, textureChannels;
	stbi_uc* pixels = stbi_load(file, &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);

	if (!pixels) {
		console->log("Failed to load texture file " + std::string(file));
		return nullptr;
	}

	void* pixel_ptr = pixels;
	VkDeviceSize imageSize = textureWidth * textureHeight * 4;
	VkFormat image_format = VK_FORMAT_R8G8B8A8_SRGB;

	AllocatedBuffer stagingBuffer = renderer.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

	void* data;
	vmaMapMemory(renderer.allocator, stagingBuffer.allocation, &data);
	memcpy(data, pixel_ptr, static_cast<size_t>(imageSize));
	vmaUnmapMemory(renderer.allocator, stagingBuffer.allocation);
	stbi_image_free(pixels);

	VkExtent3D imageExtent;
	imageExtent.width = static_cast<uint32_t>(textureWidth);
	imageExtent.height = static_cast<uint32_t>(textureHeight);
	imageExtent.depth = 1;

	VkImageCreateInfo dimg_info = vkinit::imageCreateInfo(image_format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);
	AllocatedImage newImage;
	VmaAllocationCreateInfo dimg_allocinfo = {};
	dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	vmaCreateImage(renderer.allocator, &dimg_info, &dimg_allocinfo, &newImage.image, &newImage.allocation, nullptr);

	renderer.immediateSubmit([&](VkCommandBuffer cmd) {
		VkImageSubresourceRange range;
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.baseMipLevel = 0;
		range.levelCount = 1;
		range.baseArrayLayer = 0;
		range.layerCount = 1;

		VkImageMemoryBarrier imageBarrier_toTransfer = {};
		imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

		imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrier_toTransfer.image = newImage.image;
		imageBarrier_toTransfer.subresourceRange = range;

		imageBarrier_toTransfer.srcAccessMask = 0;
		imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		//barrier the image into the transfer-receive layout
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);

		VkBufferImageCopy copyRegion = {};
		copyRegion.bufferOffset = 0;
		copyRegion.bufferRowLength = 0;
		copyRegion.bufferImageHeight = 0;

		copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageExtent = imageExtent;

		//copy the buffer into the image
		vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, newImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

		VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;

		imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageBarrier_toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		//barrier the image into the shader readable layout
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);
	});

	renderer.mainDeletionQueue.pushFunction([=]() {
		vmaDestroyImage(renderer.allocator, newImage.image, newImage.allocation);
	});

	vmaDestroyBuffer(renderer.allocator, stagingBuffer.buffer, stagingBuffer.allocation);

	console->log("Texture " + std::string(file) + " loaded successfully");

	Texture newTexture = {};
	newTexture.image = newImage;

	VkImageViewCreateInfo imageInfo = vkinit::imageviewCreateInfo(
		VK_FORMAT_R8G8B8A8_SRGB, newTexture.image.image, VK_IMAGE_ASPECT_COLOR_BIT
	);

	vkCreateImageView(renderer.device, &imageInfo, nullptr, &newTexture.imageView);

	renderer.mainDeletionQueue.pushFunction([=]() {
		vkDestroyImageView(renderer.device, newTexture.imageView, nullptr);
	});

	loadedTextures[file] = newTexture;
	return &loadedTextures[file];
}