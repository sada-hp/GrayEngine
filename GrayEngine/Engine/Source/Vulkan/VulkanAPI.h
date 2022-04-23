#pragma once
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <vk_mem_alloc.h>
#include <stb_image.h>
#include "DrawableObj.h"
#include "Engine/Source/Headers/Logger.h"
#include "Engine/Source/Headers/Renderer.h"

namespace GrEngine_Vulkan
{
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete()
		{
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	class VulkanAPI : public GrEngine::Renderer
	{
	public:
		bool init(GLFWwindow* window) override;
		void destroy() override;
		void drawFrame() override;
		VkDevice logicalDevice;

		inline VkExtent2D getExtent() { return swapChainExtent; };
		inline VmaAllocator getMemAllocator() { return memAllocator; };
		inline VkRenderPass getRenderPass() { return renderPass; };
		bool updateDrawables(uint32_t index);
		static VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);
		bool loadModel(const char* mesh_path, std::vector<std::string> textures_vector, std::string* out_materials_names = nullptr) override;
		bool loadImage(const char* image_path, int material_index = 0) override;
		void clearDrawables() override;
		static bool createVkBuffer(VkDevice device, VmaAllocator allocator, const void* bufData, uint32_t dataSize, VkBufferUsageFlags usage, ShaderBuffer* shader);
		static void destroyShaderBuffer(VkDevice device, VmaAllocator allocator, ShaderBuffer* shaderBuf);
		static void destroyTexture(VkDevice device, VmaAllocator allocator, Texture* texture);

		void Update() override;

	private:
		GLFWwindow* pParentWindow;
		VmaAllocator memAllocator;

		VkInstance _vulkan;
		VkPhysicalDeviceProperties deviceProps;
		VkQueue presentQueue;
		VkSurfaceKHR surface;
		VkPhysicalDevice physicalDevice = nullptr;
		const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		VkSwapchainKHR swapChain;
		std::vector<VkImage> swapChainImages;
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent;
		std::vector<VkImageView> swapChainImageViews;
		VkRenderPass renderPass;
		std::vector<VkFramebuffer> swapChainFramebuffers;
		VkCommandPool commandPool;
		std::vector<VkCommandBuffer> commandBuffers;
		VkSemaphore imageAvailableSemaphore;
		VkSemaphore renderFinishedSemaphore;
		VkQueue graphicsQueue;
		VkFence graphicsFence;

		VkMemoryRequirements memRequirements;

		std::vector<DrawableObj> drawables;

		bool loadMesh(const char* mesh_path, DrawableObj* target, std::string* out_materials = nullptr);
		bool loadTexture(const char* texture_path, DrawableObj* target, int material_index = 0);

		bool createVKInstance();
		bool createMemoryAllocator();
		bool createLogicalDevice();
		bool isDeviceSuitable(VkPhysicalDevice device);
		bool checkDeviceExtensionSupport(VkPhysicalDevice device);

		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

		bool createSwapChain();
		bool createImageViews();

		bool createRenderPass();
		bool createFramebuffers();
		bool createCommandPool();
		bool createCommandBuffers();
		bool createSemaphores();

		void recreateSwapChain();
		void cleanupSwapChain();

		VkCommandBuffer beginSingleTimeCommands();
		void endSingleTimeCommands(VkCommandBuffer commandBuffer);
		void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
		void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

		VkImageView depthImageView;
		AllocatedImage depthImage;
		VkFormat depthFormat;
	};
}