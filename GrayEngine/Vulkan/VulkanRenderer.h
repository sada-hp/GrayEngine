#ifdef _DEBUG
#define VALIDATION
#endif

#pragma once
#include "VulkanObject.h"
#include "VulkanSkybox.h"
#include "VulkanTerrain.h"
#include "VulkanSpotlight.h"
#include "VulkanPointLight.h"
#include "VulkanOmniLight.h"
#include "VulkanCascade.h"
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
		void VSyncState(bool state) override;
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
		GrEngine::Entity* CloneEntity(UINT id) override;
		GrEngine::Object* InitDrawableObject(GrEngine::Entity* ownerEntity) override;
		GrEngine::LightObject* InitSpotlightObject(GrEngine::Entity* ownerEntity) override;
		GrEngine::LightObject* InitCascadeLightObject(GrEngine::Entity* ownerEntity) override;
		GrEngine::LightObject* InitPointLightObject(GrEngine::Entity* ownerEntity) override;
		GrEngine::LightObject* InitOmniLightObject(GrEngine::Entity* ownerEntity) override;
		void SetUseDynamicLighting(bool state) override;

		void SaveScreenshot(const char* filepath);
		void addEntity(GrEngine::Entity* entity) override;
		bool assignTextures(std::vector<std::string> textures, GrEngine::Entity* target, GrEngine::TextureType type, bool update_object = true) override;
		void clearDrawables() override;
		void createSkybox(const char* East, const char* West, const char* Top, const char* Bottom, const char* North, const char* South) override;
		void UpdateFogParameters(FogSettings para) override;

		void SelectEntityAtCursor() override;
		std::array<byte, 3> GetPixelColorAtCursor() override;
		GrEngine::Entity* selectEntity(UINT ID) override;
		void SetHighlightingMode(bool enabled) override;
		void DeleteEntity(UINT id) override;
		void SaveScene(const char* path) override;
		void LoadScene(const char* path) override;
		void waitForRenderer() override;
		void LoadTerrain(int resolution, int width, int height, int depth, std::array<std::string, 6> maps, std::array<std::string, 4> normals, std::array<std::string, 4> displacement) override;
		void LoadTerrain(const char* filepath) override;
		std::vector<std::string> GetMaterialNames(const char* mesh_path) override;

		void Update() override;
		VkSampleCountFlagBits GetSampling() { return msaaSamples; };
		inline VulkanResourceManager& GetResourceManager() { return resources; }
		Resource<Texture*>* loadTexture(std::vector<std::string> texture_path, GrEngine::TextureType type, VkImageViewType type_view = VK_IMAGE_VIEW_TYPE_2D, VkImageType type_img = VK_IMAGE_TYPE_2D, VkFormat format_img = VK_FORMAT_R8G8B8A8_SRGB, bool default_to_black = false);
		bool updateTexture(GrEngine::Entity* target, int textureIndex);
		bool updateResource(Texture* target, int textureIndex);
		bool updateResource(Texture* target, byte* pixels);
		bool updateResource(Texture* target, byte* pixels, uint32_t width, uint32_t height, uint32_t offset_x, uint32_t offset_y);

		std::optional<uint32_t> compute_bit;
		std::optional<uint32_t> graphics_bit;

		Texture position;
		Texture normal;
		Texture albedo;
		Texture identity;
		Texture headIndex;
		Texture shadowMap;
		Texture colorImage;

		ShaderBuffer transBuffer;
		ShaderBuffer nodeBfffer;

		VkQueue graphicsQueue;
		VkCommandPool commandPool;

		VkRenderPass selectionPass;

		struct ViewProjection {
			glm::mat4 view;
			glm::mat4 proj;
			glm::vec3 pos;
		} vpUBO;
		ShaderBuffer viewProjUBO;


		struct Cascade
		{
			float splitDepth;
			glm::mat4 viewProjMatrix;
		};
		VulkanSpotlight::ShadowProjection cascadePrj;

		VkRenderPass shadowPass;
		ShaderBuffer shadowBuffer;

		int cascade_count = 0;
		int omni_count = 0;

		const uint32_t lightsCount()
		{
			return lights.size() + cascade_count * (SHADOW_MAP_CASCADE_COUNT - 1) + omni_count * 5;
		}
	protected:
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
		std::vector<VkSemaphore> imageAvailableSemaphore;
		std::vector<VkSemaphore> renderFinishedSemaphore;
		std::vector<VkFence> renderFence;
		VkFence transitionFence;

		uint32_t currentFrame = 0;
		uint32_t previousFrame = 0;

		VkMemoryRequirements memRequirements;

		uint32_t currentImageIndex = 0;

		void drawFrame(VkExtent2D extent);

		bool createVKInstance();
		bool isDeviceSuitable(VkPhysicalDevice device);

		void initSkyEntity();
		void initDefaultViewport();
		bool createSwapChainImages();
		void recreateSwapChain();
		void updateShadowResources();
		void cleanupSwapChain();
		void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels, uint32_t arrayLevels);

		VkImageView depthImageView;
		AllocatedImage depthImage;
		VkFormat depthFormat;
		VkFence loadFence;

		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

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
		void updateCompositionResources();
		void prepareTransparencyPass();
		void updateTransparencyResources();
		void prepareSamplingPass();
		void updateSamplingResources();
		void prepareSelectionPass();
		void prepareShadowPass();

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

		VkFramebuffer shadowFramebuffer;
		ShaderBuffer cascadeBuffer;
		ShaderBuffer fogBuffer;
		FogSettings fogParams;

		uint8_t max_async_frames = 1;
		bool vsync = false;

#ifdef VALIDATION

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
			createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
			createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
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

