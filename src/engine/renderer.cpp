#pragma warning( disable : 6011 )

#include "renderer.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <VkBootstrap.h>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include <glm/gtx/transform.hpp>
#include <imgui_impl_sdl.h>
#include <imgui_impl_vulkan.h>

#include <fstream>
#include <iostream>
#include <cmath>

#include "vulkankinitialisers.h"
#include "pipelinebuilder.h"
#include "window.h"
#include "console.h"
#include "model.h"
#include "mesh.h"

constexpr uint32_t ONE_SECOND = 1000000000;
constexpr uint32_t MAX_RENDERABLE_OBJECTS = 10000;

void VK_CHECK(VkResult err, Console& console) {
	do {                                                                  
		if (err) {                                                              
			std::cout << "Detected Vulkan error: " << err << std::endl; 
			console.log("Detected Vulkan error: " + err);
			abort();
		}                                                   
	} while (0);
}


void Renderer::init(Window& window, uint32_t* pFrameNumber, Console& console) {
	this->window = &window;
	this->pFrameNumber = pFrameNumber;
	this->console = &console;

	initVulkan();

	initSwapchain();
	mainDeletionQueue.pushFunction([=]() {
		cleanupSwapchain();
	});

	initCommands();
	initDefaultRenderpass();
	initFramebuffers();
	mainDeletionQueue.pushFunction([=]() {
		cleanupFramebuffers();
	});

	initSyncStructure();
	initDescriptors();
	initPipelines();
	initIMGUI();

	camera.init();
	meshManager.init(console);
	textureManager.init(console);

	isInitialised = true;
}

void Renderer::draw() {
	VK_CHECK(vkWaitForFences(device, 1, &getCurrentFrame().renderFence, true, ONE_SECOND), *console);

	uint32_t swapchainImageIndex;
	VkResult result = vkAcquireNextImageKHR(device, swapchain, ONE_SECOND, getCurrentFrame().presentSemaphore, nullptr, &swapchainImageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
		recreateSwapchain();
		framebufferResized = false;
		return;
	}

	VK_CHECK(vkResetFences(device, 1, &getCurrentFrame().renderFence), *console);

	VK_CHECK(vkResetCommandBuffer(getCurrentFrame().mainCommandBuffer, 0), *console);
	VkCommandBuffer cmd = getCurrentFrame().mainCommandBuffer;

	//begin the command buffer recording. We will use this command buffer exactly once, so we want to let Vulkan know that
	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo), *console);
	VkClearValue colorClearValue;
	colorClearValue.color = { { 0.f, 0.f, 0.f, 1.f } };

	VkClearValue depthClearValue;
	depthClearValue.depthStencil.depth = 1.f;

	//start the main renderpass.
	//We will use the clear color from above, and the framebuffer of the index the swapchain gave us
	VkRenderPassBeginInfo rpInfo = {};
	rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpInfo.pNext = nullptr;

	rpInfo.renderPass = renderPass;
	rpInfo.renderArea.offset.x = 0;
	rpInfo.renderArea.offset.y = 0;
	rpInfo.renderArea.extent = window->extent;
	rpInfo.framebuffer = framebuffers[swapchainImageIndex];

	VkClearValue clearValues[] = { colorClearValue, depthClearValue };
	rpInfo.clearValueCount = 2;
	rpInfo.pClearValues = &clearValues[0];

	vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport {};
	viewport.x = 0.f;
	viewport.y = 0.f;
	viewport.width = window->extent.width;
	viewport.height = window->extent.height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;
	VkRect2D scissor{ {0, 0}, window->extent };
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	drawModelsInQueue(cmd);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

	vkCmdEndRenderPass(cmd);
	VK_CHECK(vkEndCommandBuffer(cmd), *console);

	//prepare the submission to the queue.
	//we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
	//we will signal the _renderSemaphore, to signal that rendering has finished

	VkSubmitInfo submit = {};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.pNext = nullptr;

	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	submit.pWaitDstStageMask = &waitStage;

	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &getCurrentFrame().presentSemaphore;

	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &getCurrentFrame().renderSemaphore;

	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &cmd;

	//submit command buffer to the queue and execute it.
	// _renderFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submit, getCurrentFrame().renderFence), *console);

	// this will put the image we just rendered into the visible window.
	// we want to wait on the _renderSemaphore for that,
	// as it's necessary that drawing commands have finished before the image is displayed to the user
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;

	presentInfo.pSwapchains = &swapchain;
	presentInfo.swapchainCount = 1;

	presentInfo.pWaitSemaphores = &getCurrentFrame().renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices = &swapchainImageIndex;

	VK_CHECK(vkQueuePresentKHR(graphicsQueue, &presentInfo), *console);
}

void Renderer::drawDebug() {
	ImGui::Begin("Renderer");
	ImGui::Text("Camera Position: {%.3f, %.3f, %.3f}", camera.position.x, camera.position.y, camera.position.z);
	ImGui::Text("Camera Rotation: {%.3f, %.3f}", camera.rotation.x, camera.rotation.y);
	ImGui::End();
}

void Renderer::addToModelQueue(Model& model) {
	modelQueue.push_back(&model);
}

void Renderer::drawModelsInQueue(VkCommandBuffer cmd) {
	if (modelQueue.size() == 0) {
		return;
	}

	glm::mat4 view = camera.view();
	glm::mat4 projection = camera.projection();

	Mesh* lastMesh = nullptr;
	Material* lastMaterial = nullptr;
	int frameIndex = *pFrameNumber % FRAME_OVERLAP;

	int8_t* sceneData;
	vmaMapMemory(allocator, scenePropsBuffer.allocation, (void**)&sceneData);
	sceneData += padUniformBufferSize(sizeof(GPUSceneData)) * frameIndex;
	memcpy(sceneData, &sceneProps, sizeof(GPUSceneData));
	vmaUnmapMemory(allocator, scenePropsBuffer.allocation);

	GPUCameraData cameraData;
	cameraData.view = view;
	cameraData.projection = projection;
	cameraData.matrix = camera.matrix();
	void* data;
	vmaMapMemory(allocator, getCurrentFrame().cameraBuffer.allocation, &data);
	memcpy(data, &cameraData, sizeof(GPUCameraData));
	vmaUnmapMemory(allocator, getCurrentFrame().cameraBuffer.allocation);

	void* modelData;
	vmaMapMemory(allocator, getCurrentFrame().modelBuffer.allocation, &modelData);
	GPUModelData* modelSSBO = (GPUModelData*)modelData;
	for (uint32_t modelIndex = 0; modelIndex < modelQueue.size(); ++modelIndex) {
		modelSSBO[modelIndex].matrix = modelQueue[modelIndex]->transformMatrix;
	}
	vmaUnmapMemory(allocator, getCurrentFrame().modelBuffer.allocation);

	for (uint32_t modelIndex = 0; modelIndex < modelQueue.size(); ++modelIndex) {
		Model& model = *modelQueue[modelIndex];
		if (model.material != lastMaterial) {
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, model.material->pipeline);

			uint32_t uniformOffset = padUniformBufferSize(sizeof(GPUSceneData)) * frameIndex;
			vkCmdBindDescriptorSets(
				cmd, 
				VK_PIPELINE_BIND_POINT_GRAPHICS, 
				model.material->pipelineLayout, 
				0, 1, 
				&getCurrentFrame().globalDescriptor, 1, &uniformOffset);

			vkCmdBindDescriptorSets(
				cmd,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				model.material->pipelineLayout,
				1, 1,
				&getCurrentFrame().modelDescriptor, 0, nullptr);

			if (model.material->textureSet != VK_NULL_HANDLE) {
				//texture descriptor
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, model.material->pipelineLayout, 2, 1, &model.material->textureSet, 0, nullptr);

			}

			lastMaterial = model.material;
		}

		if (model.mesh != lastMesh) {
			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(cmd, 0, 1, &model.mesh->vertexBuffer.buffer, &offset);
			lastMesh = model.mesh;
		}

		vkCmdDraw(cmd, (uint32_t)model.mesh->vertices.size(), 1, 0, modelIndex);
	}

	modelQueue.clear();
}

void Renderer::initVulkan() {
	vkb::InstanceBuilder builder;

	//make the Vulkan instance, with basic debug features
	auto inst_ret = builder.set_app_name("VNG")
		.request_validation_layers(true)
		.require_api_version(1, 3, 0)
		.use_default_debug_messenger()
		.build();

	vkb::Instance vkb_inst = inst_ret.value();

	//store the instance
	instance = vkb_inst.instance;
	//store the debug messenger
	debugMessenger = vkb_inst.debug_messenger;

	// get the surface of the window we opened with SDL
	SDL_Vulkan_CreateSurface(window->SDL_window, instance, &surface);

	//use vkbootstrap to select a GPU.
	//We want a GPU that can write to the SDL surface and supports Vulkan 1.1
	vkb::PhysicalDeviceSelector selector{ vkb_inst };
	vkb::PhysicalDevice physicalDevice = selector
		.set_minimum_version(1, 3)
		.set_surface(surface)
		.select()
		.value();

	//create the final Vulkan device
	vkb::DeviceBuilder deviceBuilder{ physicalDevice };
	VkPhysicalDeviceShaderDrawParametersFeatures shader_draw_parameters_features = {};
	shader_draw_parameters_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
	shader_draw_parameters_features.pNext = nullptr;
	shader_draw_parameters_features.shaderDrawParameters = VK_TRUE;
	vkb::Device vkbDevice = deviceBuilder.add_pNext(&shader_draw_parameters_features).build().value();

	// Get the VkDevice handle used in the rest of a Vulkan application
	device = vkbDevice.device;
	GPU = physicalDevice.physical_device;
	GPU_props = vkbDevice.physical_device.properties;

	graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

	VmaAllocatorCreateInfo allocatorInfo = {};

	allocatorInfo.physicalDevice = GPU;
	allocatorInfo.device = device;
	allocatorInfo.instance = instance;
	vmaCreateAllocator(&allocatorInfo, &allocator);
}

void Renderer::initIMGUI() {
	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	VkDescriptorPool imguiPool;
	VK_CHECK(vkCreateDescriptorPool(device, &pool_info, nullptr, &imguiPool), *console);

	ImGui::CreateContext();
	ImGui_ImplSDL2_InitForVulkan(window->SDL_window);

	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = instance;
	init_info.PhysicalDevice = GPU;
	init_info.Device = device;
	init_info.Queue = graphicsQueue;
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&init_info, renderPass);

	//execute a gpu command to upload imgui font textures
	immediateSubmit([&](VkCommandBuffer cmd) {
		ImGui_ImplVulkan_CreateFontsTexture(cmd);
	});

	ImGui_ImplVulkan_DestroyFontUploadObjects();

	mainDeletionQueue.pushFunction([=]() {
		vkDestroyDescriptorPool(device, imguiPool, nullptr);
		ImGui_ImplVulkan_Shutdown();
	});
}

void Renderer::initSwapchain() {
	vkb::SwapchainBuilder swapchainBuilder{ GPU, device, surface };

	vkb::Swapchain vkbSwapchain = swapchainBuilder
		.use_default_format_selection()
		//use vsync present mode
		.set_desired_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR)
		.set_desired_extent(window->extent.width, window->extent.height)
		.build()
		.value();

	//store swapchain and its related images
	swapchain = vkbSwapchain.swapchain;
	swapchainImages = vkbSwapchain.get_images().value();
	swapchainImageViews = vkbSwapchain.get_image_views().value();
	swapchainImageFormat = vkbSwapchain.image_format;

	VkExtent3D depthImageExtent = {
		window->extent.width,
		window->extent.height,
		1
	};

	//hardcoding the depth format to 32 bit float
	depthFormat = VK_FORMAT_D32_SFLOAT;

	//the depth image will be an image with the format we selected and Depth Attachment usage flag
	VkImageCreateInfo dimgInfo = vkinit::imageCreateInfo(depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);

	//for the depth image, we want to allocate it from GPU local memory
	VmaAllocationCreateInfo dimg_allocinfo = {};
	dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	dimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//allocate and create the image
	vmaCreateImage(allocator, &dimgInfo, &dimg_allocinfo, &depthImage.image, &depthImage.allocation, nullptr);
	VkImageViewCreateInfo dviewInfo = vkinit::imageviewCreateInfo(depthFormat, depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

	VK_CHECK(vkCreateImageView(device, &dviewInfo, nullptr, &depthImageView), *console);
}

void Renderer::recreateSwapchain() {
	cleanupSwapchain();
	cleanupFramebuffers();

	initSwapchain();
	initFramebuffers();
}

void Renderer::cleanupSwapchain() {
	vkDestroySwapchainKHR(device, swapchain, nullptr);
	vkDestroyImageView(device, depthImageView, nullptr);
	vmaDestroyImage(allocator, depthImage.image, depthImage.allocation);
}

void Renderer::initCommands() {
	//create a command pool for commands submitted to the graphics queue.
	//we also want the pool to allow for resetting of individual command buffers
	VkCommandPoolCreateInfo commandPoolInfo = vkinit::commandPoolCreateInfo(graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	for (uint8_t i = 0; i < FRAME_OVERLAP; ++i) {
		VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &frames[i].commandPool), *console);
		VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::commandBufferAllocateInfo(frames[i].commandPool, 1);
		VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &frames[i].mainCommandBuffer), *console);

		mainDeletionQueue.pushFunction([=]() {
			vkDestroyCommandPool(device, frames[i].commandPool, nullptr);
		});
	}

	VkCommandPoolCreateInfo uploadCommandPoolInfo = vkinit::commandPoolCreateInfo(graphicsQueueFamily);
	VK_CHECK(vkCreateCommandPool(device, &uploadCommandPoolInfo, nullptr, &uploadContext.commandPool), *console);

	mainDeletionQueue.pushFunction([=]() {
		vkDestroyCommandPool(device, uploadContext.commandPool, nullptr);
	});

	//allocate the default command buffer that we will use for the instant commands
	VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::commandBufferAllocateInfo(uploadContext.commandPool, 1);
	VkCommandBuffer cmd;

	VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &uploadContext.commandBuffer), *console);
}

void Renderer::initDefaultRenderpass() {
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapchainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.flags = 0;
	depthAttachment.format = depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkAttachmentDescription attachments[2] = { colorAttachment, depthAttachment };

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkSubpassDependency depthDependency = {};
	depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	depthDependency.dstSubpass = 0;
	depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	depthDependency.srcAccessMask = 0;
	depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkSubpassDependency dependencies[2] = { dependency, depthDependency };

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = &attachments[0];
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 2;
	renderPassInfo.pDependencies = &dependencies[0];

	VK_CHECK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass), *console);

	mainDeletionQueue.pushFunction([=]() {
		vkDestroyRenderPass(device, renderPass, nullptr);
	});
}

void Renderer::initFramebuffers() {
	//create the framebuffers for the swapchain images. This will connect the render-pass to the images for rendering
	VkFramebufferCreateInfo fb_info = {};
	fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fb_info.pNext = nullptr;

	fb_info.renderPass = renderPass;
	fb_info.attachmentCount = 1;
	fb_info.width = window->extent.width;
	fb_info.height = window->extent.height;
	fb_info.layers = 1;

	//grab how many images we have in the swapchain
	const uint32_t swapchain_imagecount = (uint32_t)swapchainImages.size();
	framebuffers = std::vector<VkFramebuffer>(swapchain_imagecount);

	//create framebuffers for each of the swapchain image views
	for (uint32_t i = 0; i < swapchain_imagecount; i++) {
		VkImageView attachments[2];
		attachments[0] = swapchainImageViews[i];
		attachments[1] = depthImageView;

		fb_info.pAttachments = attachments;
		fb_info.attachmentCount = 2;
		VK_CHECK(vkCreateFramebuffer(device, &fb_info, nullptr, &framebuffers[i]), *console);
	}
}

void Renderer::cleanupFramebuffers() {
	const uint32_t swapchain_imagecount = (uint32_t)swapchainImages.size();

	for (uint32_t i = 0; i < swapchain_imagecount; i++) {
		vkDestroyFramebuffer(device, framebuffers[i], nullptr);
		vkDestroyImageView(device, swapchainImageViews[i], nullptr);
	}
}

void Renderer::initSyncStructure() {
	VkFenceCreateInfo fenceCreateInfo = vkinit::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphoreCreateInfo();

	VkFenceCreateInfo uploadFenceCreateInfo = vkinit::fenceCreateInfo();

	VK_CHECK(vkCreateFence(device, &uploadFenceCreateInfo, nullptr, &uploadContext.uploadFence), *console);
	mainDeletionQueue.pushFunction([=]() {
		vkDestroyFence(device, uploadContext.uploadFence, nullptr);
	});

	for (uint8_t i = 0; i < FRAME_OVERLAP; ++i) {
		VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &frames[i].renderFence), *console);

		mainDeletionQueue.pushFunction([=]() {
			vkDestroyFence(device, frames[i].renderFence, nullptr);
		});

		VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].presentSemaphore), *console);
		VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].renderSemaphore), *console);

		mainDeletionQueue.pushFunction([=]() {
			vkDestroySemaphore(device, frames[i].presentSemaphore, nullptr);
			vkDestroySemaphore(device, frames[i].renderSemaphore, nullptr);
		});
	}
}

void Renderer::initDescriptors() {
	std::vector<VkDescriptorPoolSize> sizes =
	{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 }
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = 0;
	pool_info.maxSets = 10;
	pool_info.poolSizeCount = (uint32_t)sizes.size();
	pool_info.pPoolSizes = sizes.data();

	vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptorPool);

	VkDescriptorSetLayoutBinding cameraBufferBinding = vkinit::descriptorsetLayoutBinding(
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0
	);

	VkDescriptorSetLayoutBinding scenePropsBufferBinding = vkinit::descriptorsetLayoutBinding(
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1
	);

	VkDescriptorSetLayoutBinding modelBufferBinding = vkinit::descriptorsetLayoutBinding(
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		VK_SHADER_STAGE_VERTEX_BIT, 0
	);

	VkDescriptorSetLayoutBinding bindings[] = { cameraBufferBinding, scenePropsBufferBinding };

	VkDescriptorSetLayoutCreateInfo setinfo = {};
	setinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	setinfo.pNext = nullptr;
	setinfo.bindingCount = 2;
	setinfo.flags = 0;
	setinfo.pBindings = bindings;

	VkDescriptorSetLayoutCreateInfo modelSetInfo = {};
	modelSetInfo.bindingCount = 1;
	modelSetInfo.flags = 0;
	modelSetInfo.pNext = nullptr;
	modelSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	modelSetInfo.pBindings = &modelBufferBinding;

	vkCreateDescriptorSetLayout(device, &setinfo, nullptr, &globalSetLayout);
	vkCreateDescriptorSetLayout(device, &modelSetInfo, nullptr, &modelSetLayout);

	const size_t scenePropsBufferSize = FRAME_OVERLAP * padUniformBufferSize(sizeof(GPUSceneData));
	scenePropsBuffer = createBuffer(scenePropsBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	for (uint8_t i = 0; i < FRAME_OVERLAP; ++i) {
		frames[i].cameraBuffer = createBuffer(
			sizeof(GPUCameraData), 
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
			VMA_MEMORY_USAGE_CPU_TO_GPU
		);

		frames[i].modelBuffer = createBuffer(
			sizeof(GPUModelData) * MAX_RENDERABLE_OBJECTS,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU
		);

		VkDescriptorSetAllocateInfo allocateInfo = {};
		allocateInfo.pNext = nullptr;
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorPool = descriptorPool;
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.pSetLayouts = &globalSetLayout;

		vkAllocateDescriptorSets(device, &allocateInfo, &frames[i].globalDescriptor);

		VkDescriptorSetAllocateInfo modelSetAllocateInfo = {};
		modelSetAllocateInfo.pNext = nullptr;
		modelSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		modelSetAllocateInfo.descriptorPool = descriptorPool;
		modelSetAllocateInfo.descriptorSetCount = 1;
		modelSetAllocateInfo.pSetLayouts = &modelSetLayout;

		vkAllocateDescriptorSets(device, &modelSetAllocateInfo, &frames[i].modelDescriptor);

		VkDescriptorBufferInfo cameraInfo;
		cameraInfo.buffer = frames[i].cameraBuffer.buffer;
		cameraInfo.offset = 0;
		cameraInfo.range = sizeof(GPUCameraData);

		VkDescriptorBufferInfo sceneInfo;
		sceneInfo.buffer = scenePropsBuffer.buffer;
		sceneInfo.offset = 0;
		sceneInfo.range = sizeof(GPUSceneData);

		VkDescriptorBufferInfo objectBufferInfo;
		objectBufferInfo.buffer = frames[i].modelBuffer.buffer;
		objectBufferInfo.offset = 0;
		objectBufferInfo.range = sizeof(GPUModelData) * MAX_RENDERABLE_OBJECTS;

		VkWriteDescriptorSet cameraWrite = vkinit::writeDescriptorBuffer(
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
			frames[i].globalDescriptor, &cameraInfo, 0
		);

		VkWriteDescriptorSet sceneWrite = vkinit::writeDescriptorBuffer(
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			frames[i].globalDescriptor, &sceneInfo, 1
		);

		VkWriteDescriptorSet modelWrite = vkinit::writeDescriptorBuffer(
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 
			frames[i].modelDescriptor, 
			&objectBufferInfo, 0
		);


		VkWriteDescriptorSet setWrites[] = { cameraWrite, sceneWrite, modelWrite };
		vkUpdateDescriptorSets(device, 3, setWrites, 0, nullptr);
	}

	VkDescriptorSetLayoutBinding textureBind = vkinit::descriptorsetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);

	VkDescriptorSetLayoutCreateInfo textureSetInfo = {};
	textureSetInfo.bindingCount = 1;
	textureSetInfo.flags = 0;
	textureSetInfo.pNext = nullptr;
	textureSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	textureSetInfo.pBindings = &textureBind;

	vkCreateDescriptorSetLayout(device, &textureSetInfo, nullptr, &singleTextureSetLayout);

	mainDeletionQueue.pushFunction([&]() {
		vkDestroyDescriptorSetLayout(device, globalSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, modelSetLayout, nullptr);
		vkDestroyDescriptorPool(device, descriptorPool, nullptr);

		for (uint8_t i = 0; i < FRAME_OVERLAP; ++i)
		{
			vmaDestroyBuffer(allocator, frames[i].cameraBuffer.buffer, frames[i].cameraBuffer.allocation);
			vmaDestroyBuffer(allocator, frames[i].modelBuffer.buffer, frames[i].modelBuffer.allocation);
		}

		vmaDestroyBuffer(allocator, scenePropsBuffer.buffer, scenePropsBuffer.allocation);
	});
}

void Renderer::initPipelines() {

	VkPipelineLayoutCreateInfo meshPipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo();
	VkDescriptorSetLayout setLayouts[] = { globalSetLayout, modelSetLayout, singleTextureSetLayout };
	meshPipelineLayoutInfo.setLayoutCount = 3;
	meshPipelineLayoutInfo.pSetLayouts = setLayouts;

	VkPipelineLayout meshPipelineLayout;
	VK_CHECK(vkCreatePipelineLayout(device, &meshPipelineLayoutInfo, nullptr, &meshPipelineLayout), *console);

	PipelineBuilder pipelineBuilder;

	//vertex input controls how to read vertices from vertex buffers. We aren't using it yet
	pipelineBuilder.vertexInputInfo = vkinit::vertexInputStateCreateInfo();

	//input assembly is the configuration for drawing triangle lists, strips, or individual points.
	//we are just going to draw triangle list
	pipelineBuilder.inputAssembly = vkinit::inputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

	pipelineBuilder.rasterizer = vkinit::rasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
	pipelineBuilder.multisampling = vkinit::multisamplingStateCreateInfo();
	pipelineBuilder.colorBlendAttachment = vkinit::colorBlendAttachmentState();
	pipelineBuilder.depthStencil = vkinit::depthStencilCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

	VertexInputDescription vertexDescription = Vertex::getVertexDescription();

	//connect the pipeline builder vertex input info to the one we get from Vertex
	pipelineBuilder.vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
	pipelineBuilder.vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)vertexDescription.attributes.size();

	pipelineBuilder.vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
	pipelineBuilder.vertexInputInfo.vertexBindingDescriptionCount = (uint32_t)vertexDescription.bindings.size();


	VkShaderModule meshVertShader;
	if (!loadShaderModule("shaders/default.vert.spv", &meshVertShader))
	{
		console->log("Error when building the mesh vertex shader module");
	}
	else {
		console->log("Mesh vertex shader successfully loaded");
	}

	VkShaderModule meshFragShader;
	if (!loadShaderModule("shaders/default.frag.spv", &meshFragShader))
	{
		console->log("Error when building the mesh fragment shader module");
	}
	else {
		console->log("Mesh fragment shader successfully loaded");
	}

	pipelineBuilder.shaderStages.push_back(
		vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, meshVertShader));

	pipelineBuilder.shaderStages.push_back(
		vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, meshFragShader));


	pipelineBuilder.pipelineLayout = meshPipelineLayout;

	VkPipeline meshPipeline;
	meshPipeline = pipelineBuilder.buildPipeline(device, renderPass);

	//deleting all of the vulkan shaders
	vkDestroyShaderModule(device, meshVertShader, nullptr);
	vkDestroyShaderModule(device, meshFragShader, nullptr);

	meshManager.loadMaterial({ "default", meshPipeline, meshPipelineLayout, nullptr });

	//adding the pipelines to the deletion queue
	mainDeletionQueue.pushFunction([=]() {
		vkDestroyPipeline(device, meshPipeline, nullptr);
		vkDestroyPipelineLayout(device, meshPipelineLayout, nullptr);
	});
}

bool Renderer::loadShaderModule(const char* filePath, VkShaderModule* outShaderModule) {
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		return false;
	}

	size_t fileSize = (size_t)file.tellg();

	//spirv expects the buffer to be on uint32, so make sure to reserve an int vector big enough for the entire file
	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
	file.seekg(0);
	file.read((char*)buffer.data(), fileSize);
	file.close();

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = nullptr;

	//codeSize has to be in bytes, so multiply the ints in the buffer by size of int to know the real size of the buffer
	createInfo.codeSize = buffer.size() * sizeof(uint32_t);
	createInfo.pCode = buffer.data();

	//check that the creation goes well.
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		return false;
	}

	*outShaderModule = shaderModule;
	return true;
}

void Renderer::loadModel(Model& model, LoadModelInfo info) {
	model.mesh = loadMesh(info.filePath.c_str());

	if (info.textured) {
		Texture* texture = textureManager.loadTexture(*this, info.texturePath.c_str());

		Material* defaultMaterial = meshManager.loadMaterial({ "default" });
		CreateMaterialInfo materialInfo = {
			.name = info.filePath + info.texturePath,
			.pipeline = defaultMaterial->pipeline,
			.layout = defaultMaterial->pipelineLayout,
		};

		model.material = meshManager.loadMaterial(materialInfo);

		VkSamplerCreateInfo samplerInfo = vkinit::samplerCreateInfo(VK_FILTER_NEAREST);
		VkSampler sampler;
		vkCreateSampler(device, &samplerInfo, nullptr, &sampler);

		mainDeletionQueue.pushFunction([=]() {
			vkDestroySampler(device, sampler, nullptr);
		});

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.pNext = nullptr;
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &singleTextureSetLayout;

		vkAllocateDescriptorSets(device, &allocInfo, &model.material->textureSet);

		VkDescriptorImageInfo imageBufferInfo;
		imageBufferInfo.sampler = sampler;
		imageBufferInfo.imageView = texture->imageView;
		imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet textureDescriptorSet = vkinit::writeDescriptorImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, model.material->textureSet, &imageBufferInfo, 0);
		vkUpdateDescriptorSets(device, 1, &textureDescriptorSet, 0, nullptr);
	}
}

Mesh* Renderer::loadMesh(const char* filename) {
	Mesh* mesh = meshManager.loadMesh(filename);
	uploadMesh(*mesh);
	return mesh;
}

AllocatedBuffer Renderer::createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) {
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext = nullptr;

	bufferInfo.size = allocSize;
	bufferInfo.usage = usage;


	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = memoryUsage;

	AllocatedBuffer newBuffer;

	//allocate the buffer
	VK_CHECK(vmaCreateBuffer(
		allocator, 
		&bufferInfo, 
		&vmaallocInfo, 
		&newBuffer.buffer,
		&newBuffer.allocation,
		nullptr), *console);

	return newBuffer;
}

void Renderer::uploadMesh(Mesh& mesh) {
	const size_t bufferSize = mesh.vertices.size() * sizeof(Vertex);
	//allocate staging buffer
	VkBufferCreateInfo stagingBufferInfo = {};
	stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingBufferInfo.pNext = nullptr;

	stagingBufferInfo.size = bufferSize;
	stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	//let the VMA library know that this data should be on CPU RAM
	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

	AllocatedBuffer stagingBuffer;

	//allocate the buffer
	VK_CHECK(vmaCreateBuffer(allocator, &stagingBufferInfo, &vmaallocInfo,
		&stagingBuffer.buffer,
		&stagingBuffer.allocation,
		nullptr), *console);

	void* data;
	vmaMapMemory(allocator, stagingBuffer.allocation, &data);
	memcpy(data, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));
	vmaUnmapMemory(allocator, stagingBuffer.allocation);

	VkBufferCreateInfo vertexBufferInfo = {};
	vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexBufferInfo.pNext = nullptr;
	vertexBufferInfo.size = bufferSize;
	vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	//allocate the buffer
	VK_CHECK(vmaCreateBuffer(allocator, &vertexBufferInfo, &vmaallocInfo,
		&mesh.vertexBuffer.buffer,
		&mesh.vertexBuffer.allocation,
		nullptr), *console);

	immediateSubmit([=](VkCommandBuffer cmd) {
		VkBufferCopy copy;
		copy.dstOffset = 0;
		copy.srcOffset = 0;
		copy.size = bufferSize;
		vkCmdCopyBuffer(cmd, stagingBuffer.buffer, mesh.vertexBuffer.buffer, 1, &copy);
	});

	mainDeletionQueue.pushFunction([=]() {
		vmaDestroyBuffer(allocator, mesh.vertexBuffer.buffer, mesh.vertexBuffer.allocation);
	});

	vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);
}

FrameData& Renderer::getCurrentFrame()
{
	return frames[*pFrameNumber % FRAME_OVERLAP];
}

void Renderer::cleanup() {
	if (isInitialised) {
		for (uint8_t i = 0; i < FRAME_OVERLAP; ++i) {
			vkWaitForFences(device, 1, &frames[i].renderFence, true, ONE_SECOND);
		}

		mainDeletionQueue.flush();

		vmaDestroyAllocator(allocator);
		vkDestroyDevice(device, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkb::destroy_debug_utils_messenger(instance, debugMessenger);
		vkDestroyInstance(instance, nullptr);
		window->cleanup();
	}
}

void Renderer::immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function) {
	VkCommandBuffer cmd = uploadContext.commandBuffer;

	//begin the command buffer recording. We will use this command buffer exactly once before resetting, so we tell vulkan that
	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo), *console);

	//execute the function
	function(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd), *console);

	VkSubmitInfo submit = vkinit::submitInfo(&cmd);
	VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submit, uploadContext.uploadFence), *console);

	vkWaitForFences(device, 1, &uploadContext.uploadFence, true, ONE_SECOND);
	vkResetFences(device, 1, &uploadContext.uploadFence);

	// reset the command buffers inside the command pool
	vkResetCommandPool(device, uploadContext.commandPool, 0);
}

size_t Renderer::padUniformBufferSize(size_t originalSize) {
	size_t minUboAlignment = GPU_props.limits.minUniformBufferOffsetAlignment;
	size_t alignedSize = originalSize;
	if (minUboAlignment > 0) {
		alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
	}

	return alignedSize;
}