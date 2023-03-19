#pragma once
#include "VulkanObject.h"
#include "VulkanSkybox.h"
#include "VulkanTerrain.h"
#include "Engine/Headers/Core/Logger.h"
#include "Engine/Headers/Virtual/Renderer.h"
#include "VulkanAPI.h"

namespace GrEngine_Vulkan
{
	class VulkanRenderer : public GrEngine::Renderer
	{
	public:
		bool init(void* window) override;
		void destroy() override;
		void RenderFrame() override;
		VkDevice logicalDevice;
		VkPhysicalDevice physicalDevice = nullptr;

		inline VkExtent2D getExtent() { return swapChainExtent; };
		inline VmaAllocator getMemAllocator() { return memAllocator; };
		inline VkRenderPass getRenderPass() { 
			return renderPass; 
		};
		bool loadModel(UINT id, const char* mesh_path, std::vector<std::string> textures_vector) override;
		bool loadModel(UINT id, const char* model_path) override;
		GrEngine::Entity* addEntity() override;
		GrEngine::Entity* addEntity(UINT ID) override;

		void addEntity(GrEngine::Entity* entity) override;
		bool assignTextures(std::vector<std::string> textures, GrEngine::Entity* target) override;
		void clearDrawables() override;
		void createSkybox(const char* East, const char* West, const char* Top, const char* Bottom, const char* North, const char* South) override;

		void SelectEntityAtCursor() override;
		std::array<byte, 3> GetPixelColorAtCursor() override;
		GrEngine::Entity* selectEntity(UINT ID) override;
		void SetHighlightingMode(bool enabled) override;
		void DeleteEntity(UINT id) override;
		void SaveScene(const char* path) override;
		void LoadScene(const char* path) override;
		void waitForRenderer() override;
		void LoadTerrain(int resolution, int width, int height, int depth, std::array<std::string, 6> maps) override;
		void LoadTerrain(const char* filepath) override;

		void Update() override;
		VkSampleCountFlagBits GetSampling() { return msaaSamples; };
		inline VulkanResourceManager& GetResourceManager() { return resources; }
		Resource<Texture*>* loadTexture(std::vector<std::string> texture_path, VkImageViewType type_view = VK_IMAGE_VIEW_TYPE_2D, VkImageType type_img = VK_IMAGE_TYPE_2D);
		bool updateTexture(GrEngine::Entity* target, int textureIndex);
		bool updateTexture(GrEngine::Entity* target, void* pixels, int textureIndex);
		bool updateResource(Texture* target, byte* pixels);
		bool updateResource(Texture* target, byte* pixels, uint32_t width, uint32_t height, uint32_t offset_x, uint32_t offset_y);

		std::optional<uint32_t> compute_bit;

		Texture position;
		Texture normal;
		Texture albedo;
		Texture identity;

		ShaderBuffer transBuffer;
		ShaderBuffer nodeBfffer;
		Texture headIndex;

		VkQueue graphicsQueue;
		VkFence graphicsFence;
		VkCommandPool commandPool;

		VkRenderPass selectionPass;
	protected:
		void SaveScreenshot(const char* filepath);
		bool updateDrawables(uint32_t index, DrawMode mode, VkExtent2D extent);
	private:
		VulkanResourceManager resources;

		GLFWwindow* pParentWindow;
		VmaAllocator memAllocator;

		VkInstance _vulkan;
		VulkanSkybox* sky;
		VulkanTerrain* terrain;

		VkPhysicalDeviceProperties deviceProps;
		VkQueue presentQueue;
		VkSurfaceKHR surface;
		const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		VkSwapchainKHR swapChain;
		std::vector<VkImage> swapChainImages;
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent;
		std::vector<VkImageView> swapChainImageViews;
		VkRenderPass renderPass;
		std::vector<VkFramebuffer> swapChainFramebuffers;
		std::vector<VkCommandBuffer> commandBuffers;
		VkSemaphore imageAvailableSemaphore;
		VkSemaphore renderFinishedSemaphore;


		VkMemoryRequirements memRequirements;

		uint32_t currentImageIndex = 0;

		void drawFrame(VkExtent2D extent);

		bool createVKInstance();
		bool isDeviceSuitable(VkPhysicalDevice device);


		bool createSwapChainImages();
		void recreateSwapChain();
		void cleanupSwapChain();
		void generateMipmaps(VkImage image, int32_t texWidth, int32_t texHeight, uint32_t mipLevels, uint32_t arrayLevels);

		VkImageView depthImageView;
		AllocatedImage depthImage;
		VkFormat depthFormat;

		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

		Texture colorImage;
		bool highlight_selection = true;

		struct Node {
			glm::vec4 color;
			float depth;
			uint32_t next;
		};

		struct GSBO {
			uint32_t count;
			uint32_t maxNodeCount;
		} geometrySBO;

		void createAttachment(VkFormat format, VkImageUsageFlags usage, Texture* attachment);
		void prepareCompositionPass();
		void prepareTransparencyPass();
		void prepareSamplingPass();
		void prepareSelectionPass();

		VkFramebuffer defferFramebuffer;
		VkFramebuffer selectionFramebuffer;

		VkDescriptorPool compositionSetPool;
		VkDescriptorSetLayout compositionSetLayout;
		VkDescriptorSet compositionSet;
		VkPipelineLayout compositionPipelineLayout;
		VkPipeline compositionPipeline;

		VkDescriptorPool transparencySetPool;
		VkDescriptorSetLayout transparencySetLayout;
		VkDescriptorSet transparencySet;
		VkPipelineLayout transparencyPipelineLayout;
		VkPipeline transparencyPipeline;

		VkDescriptorPool samplingSetPool;
		VkDescriptorSetLayout samplingSetLayout;
		VkDescriptorSet samplingSet;
		VkPipelineLayout samplingPipelineLayout;
		VkPipeline samplingPipeline;
		VkRenderPass samplingPass;
		ShaderBuffer frameInfo;
		glm::vec2 frameSize;


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
};

