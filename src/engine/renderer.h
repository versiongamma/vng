#pragma once

class Window;
class Console;
struct Model;

#include <utils/types.h>
#include <glm/glm.hpp>
#include <imgui_impl_vulkan.h>

#include <vector>
#include <deque>
#include <functional>

#include "camera.h"
#include "meshmanager.h"
#include "texturemanager.h"

struct GPUCameraData {
	glm::mat4 view;
	glm::mat4 projection;
	glm::mat4 matrix;
};

struct GPUSceneData {
	glm::vec4 fogColor;
	glm::vec4 fogDistance;
	glm::vec4 ambientColor;
	glm::vec4 sunDirection;
	glm::vec4 sunColor;
};

struct GPUModelData {
	glm::mat4 matrix;
};

struct FrameData {
	VkSemaphore presentSemaphore, renderSemaphore;
	VkFence renderFence;

	VkCommandPool commandPool;
	VkCommandBuffer mainCommandBuffer;

	AllocatedBuffer cameraBuffer;
	AllocatedBuffer modelBuffer;
	VkDescriptorSet globalDescriptor;
	VkDescriptorSet modelDescriptor;
};

struct UploadContext {
	VkFence uploadFence;
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;
};

struct DeletionQueue {
	std::deque<std::function<void()>> deletors;

	void pushFunction(std::function<void()>&& function) {
		deletors.push_back(function);
	}

	void flush() {
		// reverse iterate the deletion queue to execute all the functions
		for (auto deletor = deletors.rbegin(); deletor != deletors.rend(); deletor++) {
			(*deletor)(); //call the function
		}

		deletors.clear();
	}
};

struct LoadModelInfo {
	std::string filePath;
	bool textured{ false };
	std::string texturePath;
};

constexpr uint32_t FRAME_OVERLAP = 2;

class Renderer {
public:
	void init(Window& window, uint32_t* pFrameNumber, Console& console);
	void draw();
	void drawDebug();
	void cleanup();

	void loadModel(Model& model, LoadModelInfo info);
	void addToModelQueue(Model& model);
	void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

	VkInstance instance; // Vulkan library handle
	VkDebugUtilsMessengerEXT debugMessenger; // Vulkan debug output handle
	VkPhysicalDevice GPU; // GPU chosen as the default device
	VkPhysicalDeviceProperties GPU_props;
	VkDevice device; // Vulkan device for commands
	VkSurfaceKHR surface; // Vulkan window surface

	bool isInitialised{ false };
	bool framebufferResized{ false };
	Window* window;
	Console* console;
	Camera camera;
	VmaAllocator allocator;
	AllocatedBuffer createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
	DeletionQueue mainDeletionQueue;

protected:
	void initVulkan();
	void initIMGUI();
	void initSwapchain();
	void initCommands();
	void initDefaultRenderpass();
	void initFramebuffers();
	void initSyncStructure();
	void initDescriptors();
	void initPipelines();

	void recreateSwapchain();
	void cleanupSwapchain();
	void cleanupFramebuffers();

	Mesh* loadMesh(const char* filename);

	bool loadShaderModule(const char* filePath, VkShaderModule* outShaderModule);
	void uploadMesh(Mesh& mesh);
	FrameData& getCurrentFrame();

	void drawModelsInQueue(VkCommandBuffer cmd);
	size_t padUniformBufferSize(size_t originalSize);

	ImGui_ImplVulkanH_Window ImGuiWindowData;


	VkSwapchainKHR swapchain; // from other articles
	VkFormat swapchainImageFormat;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;

	VkQueue graphicsQueue;
	uint32_t graphicsQueueFamily;

	GPUSceneData sceneProps;
	AllocatedBuffer scenePropsBuffer;

	FrameData frames[FRAME_OVERLAP];
	uint32_t* pFrameNumber;

	VkRenderPass renderPass;
	std::vector<VkFramebuffer> framebuffers;
	VkImageView depthImageView;
	AllocatedImage depthImage;
	VkFormat depthFormat;

	VkDescriptorSetLayout globalSetLayout;
	VkDescriptorSetLayout modelSetLayout;
	VkDescriptorSetLayout singleTextureSetLayout;
	VkDescriptorPool descriptorPool;

	UploadContext uploadContext;

	MeshManager meshManager;
	TextureManager textureManager;
	std::vector<Model*> modelQueue;
};