#pragma once
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <vk_mem_alloc.h>
#include <stb_image.h>
#include <stb_image_resize.h>
#include "VulkanResourceManager.h"
#include "VulkanObject.h"
#include "VulkanSkybox.h"
#include "Engine/Headers/Core/Logger.h"
#include "Engine/Headers/Virtual/Renderer.h"

namespace GrEngine_Vulkan
{

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

	typedef void (*ImageReadCallback)(VkDeviceMemory, VkSubresourceLayout);

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

	enum DrawMode
	{
		NORMAL = 0,
		IDS = 1
	};

	class VulkanAPI : public GrEngine::Renderer
	{
	public:
		bool init(void* window) override;
		void destroy() override;
		void RenderFrame() override;
		void drawFrame(DrawMode mode, bool Show = true);
		VkDevice logicalDevice;

		inline VkExtent2D getExtent() { return swapChainExtent; };
		inline VmaAllocator getMemAllocator() { return memAllocator; };
		inline VkRenderPass getRenderPass() { return renderPass; };
		bool loadModel(UINT id, const char* mesh_path, std::vector<std::string> textures_vector) override;
		bool loadModel(UINT id, const char* model_path) override;
		GrEngine::Entity* addEntity() override;
		bool assignTextures(std::vector<std::string> textures, GrEngine::Entity* target) override;
		void clearDrawables() override;
		void createSkybox(const char* East, const char* West, const char* Top, const char* Bottom, const char* North, const char* South) override;

		static VkShaderModule m_createShaderModule(VkDevice device, const std::vector<char>& code);
		static bool m_createVkBuffer(VkDevice device, VmaAllocator allocator, const void* bufData, uint32_t dataSize, VkBufferUsageFlags usage, ShaderBuffer* shader);
		static void m_destroyShaderBuffer(VkDevice device, VmaAllocator allocator, ShaderBuffer* shaderBuf);
		static void m_destroyTexture(VkDevice device, VmaAllocator allocator, Texture* texture);
		void transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subres, VkCommandBuffer cmd = nullptr);
		void SelectEntityAtCursor();
		GrEngine::Entity* selectEntity(UINT ID) override;
		void SetHighlightingMode(bool enabled) override;
		void DeleteEntity(UINT id) override;
		void SaveScene(const char* path) override;
		void LoadScene(const char* path) override;
		void waitForRenderer() override;

		void Update() override;
		VkSampleCountFlagBits GetSampling() { return msaaSamples; };
		inline VulkanResourceManager& GetResourceManager() { return resources; }

	protected:
		GrEngine::Entity* addEntity(UINT ID);
		bool allocateCommandBuffer(VkCommandBuffer* cmd, uint32_t count = 0);
		bool beginCommandBuffer(VkCommandBuffer cmd, VkCommandBufferUsageFlags usage);
		bool freeCommandBuffer(VkCommandBuffer commandBuffer);
		void SaveScreenshot(const char* filepath);
		bool updateDrawables(uint32_t index, DrawMode mode);
		DrawMode cur_mode = DrawMode::NORMAL;
	private:
		VulkanResourceManager resources;

		GLFWwindow* pParentWindow;
		VmaAllocator memAllocator;

		VkInstance _vulkan;
		VulkanObject grid;
		VulkanSkybox* sky;

		VkPhysicalDeviceProperties deviceProps;
		VkQueue presentQueue;
		VkSurfaceKHR surface;
		VkPhysicalDevice physicalDevice = nullptr;
		const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		VkSwapchainKHR swapChain;
		std::vector<VkImage> swapChainImages;
		std::vector<VkDeviceMemory> swapChainMemory;
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

		uint32_t currentImageIndex = 0;

		bool loadTexture(std::vector<std::string> texture_path, VulkanDrawable* target, VkImageViewType type_view = VK_IMAGE_VIEW_TYPE_2D, VkImageType type_img = VK_IMAGE_TYPE_2D);

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
		VkSampleCountFlagBits getMaxUsableSampleCount();

		bool createSwapChain();
		bool createImageViews(VkFormat format, VkImageViewType type, VkImage image, VkImageView* target, int array_layers = 1, int base_layer = 0);

		bool createRenderPass();
		bool createFramebuffers();
		bool createCommandPool();
		bool createCommandBuffers();
		bool createSemaphores();

		void recreateSwapChain();
		void cleanupSwapChain();

		void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t channels, uint32_t length);
		void copyImageToBuffer(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t channels, uint32_t length);

		VkImageView depthImageView;
		AllocatedImage depthImage;
		VkFormat depthFormat;

		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
		VkDeviceMemory colorImageMemory;
		VkImageView colorImageView;
		AllocatedImage samplingImage;
		bool highlight_selection = true;

#ifdef _DEBUG

		VkDebugUtilsMessengerEXT debugMessenger;
		const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

		VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
			auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
			if (func != nullptr) {
				return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
			}
			else {
				return VK_ERROR_EXTENSION_NOT_PRESENT;
			}
		}

		void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
			auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
			if (func != nullptr) {
				func(instance, debugMessenger, pAllocator);
			}
		}

		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
			createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			createInfo.pfnUserCallback = debugCallback;
		}

		void setupDebugMessenger() {
			if (!enableValidationLayers) return;

			VkDebugUtilsMessengerCreateInfoEXT createInfo;
			populateDebugMessengerCreateInfo(createInfo);

			if (CreateDebugUtilsMessengerEXT(_vulkan, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
				throw std::runtime_error("failed to set up debug messenger!");
			}
		}

		std::vector<const char*> getRequiredExtensions() {
			uint32_t glfwExtensionCount = 0;
			const char** glfwExtensions;
			glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

			std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

			if (enableValidationLayers) {
				extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			}

			return extensions;
		}

		bool checkValidationLayerSupport() {
			uint32_t layerCount;
			vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

			std::vector<VkLayerProperties> availableLayers(layerCount);
			vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

			for (const char* layerName : validationLayers) {
				bool layerFound = false;

				for (const auto& layerProperties : availableLayers) {
					if (strcmp(layerName, layerProperties.layerName) == 0) {
						layerFound = true;
						break;
					}
				}

				if (!layerFound) {
					return false;
				}
			}

			return true;
		}

		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
			std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

			return VK_FALSE;
		}
#endif
	};
}