#pragma once
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <vk_mem_alloc.h>
#include <stb_image.h>
#include "VulkanDrawable.h"
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
		bool init(void* window) override;
		void destroy() override;
		void drawFrame() override;
		VkDevice logicalDevice;

		inline VkExtent2D getExtent() { return swapChainExtent; };
		inline VmaAllocator getMemAllocator() { return memAllocator; };
		inline VkRenderPass getRenderPass() { return renderPass; };
		bool updateDrawables(uint32_t index);
		bool loadModel(const char* mesh_path, std::vector<std::string> textures_vector, std::unordered_map<std::string, std::string>* out_materials_names = nullptr) override;
		void addDummy(GrEngine::EntityInfo* out_entity = nullptr) override;
		bool loadImage(const char* image_path, int material_index = 0) override;
		void clearDrawables() override;
		void ShowGrid() override;

		static VkShaderModule m_createShaderModule(VkDevice device, const std::vector<char>& code);
		static bool m_createVkBuffer(VkDevice device, VmaAllocator allocator, const void* bufData, uint32_t dataSize, VkBufferUsageFlags usage, ShaderBuffer* shader);
		static void m_destroyShaderBuffer(VkDevice device, VmaAllocator allocator, ShaderBuffer* shaderBuf);
		static void m_destroyTexture(VkDevice device, VmaAllocator allocator, Texture* texture);

		void Update() override;
		GrEngine::DrawableObject* getDrawable() override;

	protected:
		bool allocateCommandBuffer(VkCommandBuffer* cmd, uint32_t count = 0);
		bool beginCommandBuffer(VkCommandBuffer cmd, VkCommandBufferUsageFlags usage);
		bool freeCommandBuffer(VkCommandBuffer commandBuffer);

	private:
		GLFWwindow* pParentWindow;
		VmaAllocator memAllocator;

		VkInstance _vulkan;
		VulkanDrawable grid;
		VulkanDrawable background;
		ShaderBuffer scene_test;

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

		std::vector<VulkanDrawable> drawables;

		bool loadMesh(const char* mesh_path, VulkanDrawable* target, std::vector<std::string>* out_materials = nullptr);
		bool loadTexture(const char* texture_path, VulkanDrawable* target, std::vector<int> material_indices);
		void ClickCheck();

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

		void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
		void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
		void copyImageToBuffer(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

		VkImageView depthImageView;
		AllocatedImage depthImage;
		VkFormat depthFormat;
	};
}