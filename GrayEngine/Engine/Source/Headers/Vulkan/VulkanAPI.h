#pragma once
#define VK_KHR_swapchain

#include <pch.h>
#include <glm/glm.hpp>
#include "Engine/Source/Engine.h"
#include "Engine/Source/Libs/VkMemAlloc/vk_mem_alloc.h"
#include "DrawableObj.h"

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class VulkanAPI
{
public:
	bool initVulkan(GLFWwindow* window, VulkanAPI* apiInstance);
	void destroy();
	void drawFrame();
private:
	GLFWwindow* m_parentWindow;
	static VulkanAPI* _instance;

	VkInstance _vulkan;
	VkDevice logicalDevice;
	VmaAllocator memAllocator;
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
	VkPipelineLayout pipelineLayout;
	VkRenderPass renderPass;
	VkPipeline graphicsPipeline;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;
	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
	VkQueue graphicsQueue;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

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
	bool createGraphicsPipeline();
	VkShaderModule createShaderModule(const std::vector<char>& code);

	bool createRenderPass();
	bool createFramebuffers();
	bool createCommandPool();
	bool createCommandBuffers();
	bool createSemaphores();

	static void callSwapChainUpdate(std::vector<double> para);
	void recreateSwapChain();
	void cleanupSwapChain();

	void clearDrawables();

	bool createVkBuffer(const void* bufData, uint32_t dataSize, VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkBufferUsageFlags usage, uint32_t dataStride, bool useTexture);
};