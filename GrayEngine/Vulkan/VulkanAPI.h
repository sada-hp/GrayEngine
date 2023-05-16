#pragma once
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <vk_mem_alloc.h>
#include <stb_image.h>
#include <stb_image_resize.h>
#include <stb_image_write.h>
#include "VulkanResourceManager.h"

namespace GrEngine_Vulkan
{
#ifdef VALIDATION
	const bool enableValidationLayers = true;
#else
	const bool enableValidationLayers = false;
#endif

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	enum DrawMode
	{
		NORMAL = 0,
		TRANSPARENCY = 1
	};

	using VulkanHandle = void*;
	typedef void Destructor(VulkanHandle);

	class VulkanAPI
	{
	public:
		static void Destroy(VkDevice logicalDevice, VmaAllocator allocator);

		static VkShaderModule m_createShaderModule(VkDevice device, const std::vector<char>& code);
		static bool m_createVkBuffer(VkDevice device, VmaAllocator allocator, const void* bufData, uint32_t dataSize, VkBufferUsageFlags usage, ShaderBuffer* shader, VkMemoryPropertyFlags memProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		static void m_destroyShaderBuffer(VkDevice device, VmaAllocator allocator, ShaderBuffer* shaderBuf);
		static void m_destroyTexture(VkDevice device, VmaAllocator allocator, Texture* texture);

		static bool CreateLogicalDevice(VkPhysicalDevice physicalDevice, VkDeviceCreateInfo* deviceInfo, VkDevice* outLogicalDevice);
		static bool CreateVulkanMemoryAllocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VmaAllocator* outAllocator);
		static bool CreateVkSwapchain(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, GLFWwindow* window,VkSurfaceKHR surface, VkSwapchainKHR* outSwapchain, VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAILBOX_KHR);
		static bool CreateRenderPass(VkDevice device, VkFormat swapchainFormat, VkFormat depthFormat, VkSampleCountFlagBits sampleCount, VkRenderPass* outRenderPass);
		static bool CreateRenderPass(VkDevice device, VkRenderPassCreateInfo* info, VkRenderPass* outRenderPass);
		static bool CreateFrameBuffer(VkDevice device, VkRenderPass renderPass, VkImageView* attachments, uint32_t attachmentsCount, VkExtent2D extent, VkFramebuffer* outFrameBuffer);
		static bool CreateFrameBuffer(VkDevice device, VkFramebufferCreateInfo* info, VkFramebuffer* outFrameBuffer);
		static bool CreateCommandPool(VkDevice device, uint32_t familyIndex, VkCommandPool* outPool);
		static bool CreateVkSemaphore(VkDevice device, VkSemaphore* outSemaphore);
		static bool CreateTimelineSemaphore(VkDevice device, VkSemaphore* outSemaphore);
		static bool CreateVkFence(VkDevice device, VkFence* outFence);
		static bool CreateImage(VmaAllocator allocator, VkImageCreateInfo* createInfo, VkImage* outImage, VmaAllocation* outAllocation, VmaMemoryUsage memUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VkMemoryPropertyFlags memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		static bool CreateImageView(VkDevice device, VkFormat format, VkImage image, VkImageSubresourceRange subRange, VkImageView* target, VkImageViewType type = VK_IMAGE_VIEW_TYPE_2D);
		static bool CreatePipelineLayout(VkDevice device, std::vector<VkPushConstantRange> pushConstants, std::vector<VkDescriptorSetLayout> descriptorLayouts, VkPipelineLayout* outLayout);
		static bool CreateGraphicsPipeline(VkDevice device, VkGraphicsPipelineCreateInfo* info, VkPipeline* outPipeline);
		static bool CreateComputePipeline(VkDevice device, VkComputePipelineCreateInfo* info, VkPipeline* outPipeline);
		static bool CreateDescriptorSetLayout(VkDevice device, std::vector<VkDescriptorSetLayoutBinding> bindings, VkDescriptorSetLayout* outLayout);
		static bool CreateDescriptorPool(VkDevice device, std::vector<VkDescriptorPoolSize> pools, VkDescriptorPool* outDescriptorPool);
		static bool CreateSampler(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkSampler* outSampler, float mipLevels = 0.f);
		static bool CreateSampler(VkDevice logicalDevice, VkSamplerCreateInfo* info, VkSampler* outSampler);

		static void DestroyLogicalDevice(VkDevice device);
		static void DestroyMemoryAllocator(VmaAllocator allocator);
		static void DestroyImageView(VkImageView view);
		static void DestroyFence(VkFence fence);
		static void DestroySemaphore(VkSemaphore semaphore);
		static void DestroyCommandPool(VkCommandPool pool);
		static void DestroyFramebuffer(VkFramebuffer frameBuffer);
		static void DestroyRenderPass(VkRenderPass renderPass);
		static void DestroySwapchainKHR(VkSwapchainKHR swapChain);
		static void DestroyPipelineLayout(VkPipelineLayout layout);
		static void DestroyImage(VkImage image);
		static void DestroyPipeline(VkPipeline pipeline);
		static void DestroyDescriptorLayout(VkDescriptorSetLayout layout);
		static void DestroyDescriptorPool(VkDescriptorPool pool);
		static void DestroySampler(VkSampler sampler);
		static void FreeCommandBuffer(VkCommandBuffer buffer);
		static void FreeCommandBuffers(VkCommandBuffer* buffers, size_t buufersCount);
		static void FreeDescriptorSet(VkDescriptorSet set);
		static void FreeDescriptorSets(VkDescriptorSet* sets, size_t setCount);

		static bool GetDeviceQueue(VkDevice device, uint32_t family, VkQueue* outQueue);
		static bool AllocateCommandBuffers(VkDevice device, VkCommandPool pool, VkCommandBuffer* outBuffers, uint32_t outBuffersCount);
		static bool AllocateDescriptorSet(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout, VkDescriptorSet* outSet);
		static bool BeginCommandBuffer(VkCommandBuffer cmd, VkCommandBufferUsageFlags usage);
		static bool EndAndSubmitCommandBuffer(VkDevice device, VkCommandPool pool, VkCommandBuffer commandBuffer, VkQueue queue, VkFence fence);
		static bool CheckDeviceExtensionSupport(const VkPhysicalDevice physicalDevice, std::vector<const char*> desired_extensions);
		static bool TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subRange, VkCommandBuffer cmd);

		static std::vector<int32_t> FindFamilyIndicies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, std::vector<VkQueueFlagBits> families);
		static VkSampleCountFlagBits GetMaxSampleCount(VkPhysicalDevice physicalDevice);
		static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, VkPresentModeKHR desiredMode);
		static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		static VkExtent2D ChooseSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities);
		static SwapChainSupportDetails QuerySwapChainSupport(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice);
		static bool CopyBufferToImage(VkDevice device, VkCommandPool pool, VkBuffer buffer, VkImage image, VkQueue graphicsQueue, VkFence graphicsFence, GrEngine::ImageInfo imgInfo, uint32_t length);
		static bool CopyBufferToImage(VkDevice device, VkCommandBuffer cmd, VkBuffer buffer, VkImage image, GrEngine::ImageInfo imgInfo, uint32_t length);

		static VkDeviceCreateInfo StructDeviceCreateInfo(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures* deviceFeatures, VkDeviceQueueCreateInfo* deviceQueues, uint32_t queuesCount, const char* const* deviceExtensions, uint32_t extensionsCount);
		static std::vector<VkDeviceQueueCreateInfo> StructQueueCreateInfo(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, std::vector<VkQueueFlagBits> families, float* priority);
		static VkPhysicalDeviceFeatures StructPhysicalDeviceFeatures();
		static VkImageCreateInfo StructImageCreateInfo(VkExtent3D extent, VkFormat format, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageType type = VK_IMAGE_TYPE_2D, VkSharingMode sharing = VK_SHARING_MODE_EXCLUSIVE);
		static VkImageSubresourceRange StructSubresourceRange(VkImageAspectFlags aspectMask, uint32_t baseMipLevel = 0, uint32_t levelCount = 1, uint32_t baseArrayLevel = 0, uint32_t arrayLevelCount = 1);

	private:
		static std::unordered_map<VulkanHandle, Destructor*> destructors;

		static std::unordered_map<VulkanHandle, VmaAllocation> allocations;
		static std::unordered_map<VulkanHandle, VkDescriptorPool> descriptors;
		static std::unordered_map<VulkanHandle, VkCommandPool> cmdPools;

		static std::unordered_map<VulkanHandle, VkDevice> devices;
		static std::unordered_map<VmaAllocation, VmaAllocator> allocators;
	};
}