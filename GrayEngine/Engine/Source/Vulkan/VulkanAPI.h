#pragma once
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <tiny_obj_loader.h>
#include <vk_mem_alloc.h>
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
		bool init(GLFWwindow* window, Renderer* apiInstance) override;
		void destroy() override;
		void drawFrame() override;
		VkDevice logicalDevice;

		inline VkExtent2D getExtent() { return swapChainExtent; };
		inline static Renderer* m_getRenderer() { return pInstance; };
		inline VmaAllocator getMemAllocator() { return memAllocator; };
		inline VkRenderPass getRenderPass() { return renderPass; };
		bool updateDrawables(uint32_t index);
		static VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);
		bool loadModel(const char* model_path) override;
		void clearDrawables() override;
		static std::vector<char> readFile(const std::string& filename);
		static bool createVkBuffer(VkDevice device, VmaAllocator allocator, const void* bufData, uint32_t dataSize, VkBufferUsageFlags usage, ShaderBuffer* shader);
		static void destroyShaderBuffer(VkDevice device, VmaAllocator allocator, ShaderBuffer* shader);

		void Update() override;

	private:
		GLFWwindow* pParentWindow;
		static VulkanAPI* pInstance;
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

		VkMemoryRequirements memRequirements;

		std::vector<DrawableObj> drawables;

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

		VkImageView depthImageView;
		AllocatedImage depthImage;
		VkFormat depthFormat;
	};
}