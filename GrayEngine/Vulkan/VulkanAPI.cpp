#include <pch.h>
#define GLFW_INCLUDE_VULKAN
#define VMA_IMPLEMENTATION
#define VK_KHR_swapchain
#include "VulkanAPI.h"

namespace GrEngine_Vulkan
{
	std::unordered_map<VulkanHandle, Destructor*> VulkanAPI::destructors;
	std::unordered_map<VulkanHandle, VmaAllocation> VulkanAPI::allocations;
	std::unordered_map<VulkanHandle, VkDevice> VulkanAPI::devices;
	std::unordered_map<VmaAllocation, VmaAllocator> VulkanAPI::allocators;
	std::unordered_map<VulkanHandle, VkDescriptorPool> VulkanAPI::descriptors;
	std::unordered_map<VulkanHandle, VkCommandPool> VulkanAPI::cmdPools;

	void VulkanAPI::Destroy(VkDevice logicalDevice, VmaAllocator allocator)
	{
		vkDeviceWaitIdle(logicalDevice);

		int offset = destructors.size() - 1;
		while (offset >= 0)
		{
			std::unordered_map<void*, Destructor*>::iterator itt = destructors.begin();

			if (offset < destructors.size())
				std::advance(itt, offset);
			
			if (devices.count((*itt).first) > 0 && devices[(*itt).first] == logicalDevice || allocations.count((*itt).first) > 0 && allocators.count(allocations[(*itt).first]) > 0 && allocators[allocations[(*itt).first]] == allocator)
			{
				(*itt).second((*itt).first);
			}

			offset--;
		}

		Logger::Out("Removed device %p", OutputType::Log, logicalDevice);
		VulkanAPI::DestroyMemoryAllocator(allocator);
		VulkanAPI::DestroyLogicalDevice(logicalDevice);
	}

	VkShaderModule VulkanAPI::m_createShaderModule(VkDevice device, const std::vector<char>& code)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
			throw std::runtime_error("Failed to create shader module!");

		return shaderModule;
	}

	bool VulkanAPI::m_createVkBuffer(VkDevice device, VmaAllocator allocator, const void* bufData, uint32_t dataSize, VkBufferUsageFlags usage, ShaderBuffer* shaderBuffer, VkMemoryPropertyFlags memProperty)
	{
		/*this function may be called in async manner, so we must check if allocator is currently free, otherwise wait until its free*/
		static bool is_in_use;
		while (is_in_use) {};
		is_in_use = true;

		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = dataSize;
		bufferCreateInfo.usage = usage;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bufferCreateInfo.queueFamilyIndexCount = 0;
		bufferCreateInfo.pQueueFamilyIndices = NULL;

		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		vmaallocInfo.preferredFlags = memProperty;

		vmaCreateBuffer(allocator, &bufferCreateInfo, &vmaallocInfo, &shaderBuffer->Buffer, &shaderBuffer->Allocation, nullptr);
		vkGetBufferMemoryRequirements(device, shaderBuffer->Buffer, &shaderBuffer->MemoryRequirements);

		void* data;
		if (bufData != nullptr)
		{
			vmaMapMemory(allocator, shaderBuffer->Allocation, (void**)&data);
			memcpy(data, bufData, dataSize);
			vmaUnmapMemory(allocator, shaderBuffer->Allocation);
		}

		shaderBuffer->BufferInfo.buffer = shaderBuffer->Buffer;
		shaderBuffer->BufferInfo.offset = 0;
		shaderBuffer->BufferInfo.range = dataSize;

		shaderBuffer->MappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		shaderBuffer->MappedMemoryRange.memory = reinterpret_cast<VkDeviceMemory>(shaderBuffer->Allocation);
		shaderBuffer->MappedMemoryRange.offset = 0;
		shaderBuffer->MappedMemoryRange.size = dataSize;
		shaderBuffer->initialized = true;

		is_in_use = false;

		return true;
	}

	void VulkanAPI::m_destroyShaderBuffer(VkDevice device, VmaAllocator allocator, ShaderBuffer* shader)
	{
		if (shader != nullptr && shader->initialized != false)
		{
			vkDestroyBuffer(device, shader->Buffer, NULL);
			//vmaUnmapMemory(allocator, shader->Allocation);
			vmaFreeMemory(allocator, shader->Allocation);
			vmaFlushAllocation(allocator, shader->Allocation, 0, shader->MappedMemoryRange.size);
			//vkFlushMappedMemoryRanges(device, 1, &(shader->MappedMemoryRange));
			shader->initialized = false;
		}
	}

	void VulkanAPI::m_destroyTexture(VkDevice device, VmaAllocator allocator, Texture* texture)
	{
		static bool is_in_use;
		while (is_in_use) {};
		is_in_use = true;
		
		if (texture->initialized == true)
		{
			VulkanAPI::DestroySampler(texture->textureSampler);
			VulkanAPI::DestroyImageView(texture->textureImageView);
			VulkanAPI::DestroyImage(texture->newImage.allocatedImage);
			texture->textureSampler = nullptr;
			texture->textureImageView = nullptr;
			texture->initialized = false;
			//texture->texture_collection.clear();
		}

		is_in_use = false;
	}

	bool VulkanAPI::CreateLogicalDevice(VkPhysicalDevice physicalDevice, VkDeviceCreateInfo* deviceInfo, VkDevice* outLogicalDevice)
	{
		return vkCreateDevice(physicalDevice, deviceInfo, nullptr, outLogicalDevice) == VK_SUCCESS;
	}

	bool VulkanAPI::CreateVulkanMemoryAllocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VmaAllocator* outAllocator)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		VkDeviceSize heapSize;
		heapSize = memProperties.memoryHeapCount;

		VmaAllocatorCreateInfo vmaInfo{};
		vmaInfo.physicalDevice = physicalDevice;
		vmaInfo.device = logicalDevice;
		vmaInfo.instance = instance;
		vmaInfo.vulkanApiVersion = VK_API_VERSION_1_0;
		vmaInfo.flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT;
		vmaInfo.pHeapSizeLimit = &heapSize;
		vmaInfo.pTypeExternalMemoryHandleTypes = &memProperties.memoryTypeCount;
		vmaInfo.preferredLargeHeapBlockSize = 0;

		return vmaCreateAllocator(&vmaInfo, outAllocator) == VK_SUCCESS;
	}

	bool VulkanAPI::CreateVkSwapchain(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, GLFWwindow* window, VkSurfaceKHR surface, VkSwapchainKHR* outSwapchain, VkPresentModeKHR presentMode)
	{
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(surface, physicalDevice);

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
		presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes, presentMode);
		VkExtent2D extent = ChooseSwapExtent(window, swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		std::vector<uint32_t> queueFamilyIndices;

		float queuePriority = 1.f;
		std::vector<VkDeviceQueueCreateInfo> deviceQueues = VulkanAPI::StructQueueCreateInfo(physicalDevice, surface, { VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_COMPUTE_BIT }, &queuePriority);

		for (VkDeviceQueueCreateInfo info : deviceQueues)
		{
			if (info.queueFamilyIndex >= 0)
			{
				queueFamilyIndices.push_back(info.queueFamilyIndex);
			}
		}

		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = queueFamilyIndices.size();
		createInfo.pQueueFamilyIndices = queueFamilyIndices.data();

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		bool res = vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, outSwapchain) == VK_SUCCESS;

		destructors.insert_or_assign(destructors.begin(), *outSwapchain, (Destructor*)DestroySwapchainKHR);
		devices[*outSwapchain] = logicalDevice;
		
		return res;
	}

	bool VulkanAPI::CreateRenderPass(VkDevice device, VkFormat swapchainFormat, VkFormat depthFormat, VkSampleCountFlagBits sampleCount, VkRenderPass* outRenderPass)
	{
		std::array<VkAttachmentDescription, 5> attachments{};
		// Color attachment
		attachments[0].format = swapchainFormat;
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		// Deferred attachments
		// Position
		attachments[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		// Normals
		attachments[2].format = VK_FORMAT_R16G16B16A16_SFLOAT;
		attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		// Albedo
		attachments[3].format = swapchainFormat;
		attachments[3].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[3].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[3].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		// Depth attachment
		attachments[4].format = depthFormat;
		attachments[4].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[4].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[4].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[4].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[4].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[4].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[4].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		std::array<VkSubpassDescription, 4> subpassDescriptions{};

		// First subpass: Fill G-Buffer components
		// ----------------------------------------------------------------------------------------

		VkAttachmentReference colorReferences[4];
		colorReferences[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		colorReferences[1] = { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		colorReferences[2] = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		colorReferences[3] = { 3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		VkAttachmentReference depthReference = { 4, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

		subpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescriptions[0].colorAttachmentCount = 4;
		subpassDescriptions[0].pColorAttachments = colorReferences;
		subpassDescriptions[0].pDepthStencilAttachment = &depthReference;

		// Second subpass: Final composition (using G-Buffer components)
		// ----------------------------------------------------------------------------------------

		VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

		VkAttachmentReference inputReferences[3];
		inputReferences[0] = { 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		inputReferences[1] = { 2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		inputReferences[2] = { 3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

		uint32_t preserveAttachmentIndex = 1;

		subpassDescriptions[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescriptions[1].colorAttachmentCount = 1;
		subpassDescriptions[1].pColorAttachments = &colorReference;
		subpassDescriptions[1].pDepthStencilAttachment = &depthReference;
		// Use the color attachments filled in the first pass as input attachments
		subpassDescriptions[1].inputAttachmentCount = 3;
		subpassDescriptions[1].pInputAttachments = inputReferences;

		// Third subpass: Forward transparency
		// ----------------------------------------------------------------------------------------
		inputReferences[0] = { 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

		subpassDescriptions[2].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescriptions[2].colorAttachmentCount = 0;
		subpassDescriptions[2].pColorAttachments = nullptr;
		subpassDescriptions[2].pDepthStencilAttachment = &depthReference;
		// Use the color/depth attachments filled in the first pass as input attachments
		subpassDescriptions[2].inputAttachmentCount = 1;
		subpassDescriptions[2].pInputAttachments = inputReferences;

		subpassDescriptions[3].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescriptions[3].colorAttachmentCount = 1;
		subpassDescriptions[3].pColorAttachments = &colorReference;
		subpassDescriptions[3].pDepthStencilAttachment = &depthReference;

		// Subpass dependencies for layout transitions
		std::array<VkSubpassDependency, 6> dependencies;

		// This makes sure that writes to the depth image are done before we try to write to it again
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].dstSubpass = 0;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		// This dependency transitions the input attachment from color attachment to shader read
		dependencies[2].srcSubpass = 0;
		dependencies[2].dstSubpass = 1;
		dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[2].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[2].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[3].srcSubpass = 1;
		dependencies[3].dstSubpass = 2;
		dependencies[3].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[3].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[3].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[3].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[4].srcSubpass = 2;
		dependencies[4].dstSubpass = 3;
		dependencies[4].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[4].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[4].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[4].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[4].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[5].srcSubpass = 3;
		dependencies[5].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[5].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[5].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[5].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[5].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		dependencies[5].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = static_cast<uint32_t>(subpassDescriptions.size());
		renderPassInfo.pSubpasses = subpassDescriptions.data();
		renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
		renderPassInfo.pDependencies = dependencies.data();

		VkResult res = vkCreateRenderPass(device, &renderPassInfo, nullptr, outRenderPass);

		if (res != VK_SUCCESS)
		{
			Logger::Out("[VK] Failed to create render pass with code %d", OutputType::Error, res);
			return false;
		}

		destructors.insert_or_assign(destructors.begin(), *outRenderPass, (Destructor*)DestroyRenderPass);
		devices[*outRenderPass] = device;

		return res == VK_SUCCESS;
	}

	bool VulkanAPI::CreateRenderPass(VkDevice device, VkRenderPassCreateInfo* info, VkRenderPass* outRenderPass)
	{
		VkResult res = vkCreateRenderPass(device, info, nullptr, outRenderPass);

		if (res != VK_SUCCESS)
		{
			Logger::Out("[VK] Failed to create render pass with code %d", OutputType::Error, res);
			return false;
		}

		destructors.insert_or_assign(destructors.begin(), *outRenderPass, (Destructor*)DestroyRenderPass);
		devices[*outRenderPass] = device;

		return res == VK_SUCCESS;
	}

	bool VulkanAPI::CreateFrameBuffer(VkDevice device, VkRenderPass renderPass, VkImageView* attachments, uint32_t attachmentsCount, VkExtent2D extent, VkFramebuffer* outFrameBuffer)
	{
		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = attachmentsCount;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = extent.width;
		framebufferInfo.height = extent.height;
		framebufferInfo.layers = 1;

		VkResult res = vkCreateFramebuffer(device, &framebufferInfo, nullptr, outFrameBuffer);

		if (res != VK_SUCCESS)
		{
			Logger::Out("[VK] Failed to create framebuffer with code %d", OutputType::Error, res);
			return false;
		}

		destructors.insert_or_assign(destructors.begin(), *outFrameBuffer, (Destructor*)DestroyFramebuffer);
		devices[*outFrameBuffer] = device;

		return res == VK_SUCCESS;
	}

	bool VulkanAPI::CreateFrameBuffer(VkDevice device, VkFramebufferCreateInfo* info, VkFramebuffer* outFrameBuffer)
	{
		VkResult res = vkCreateFramebuffer(device, info, nullptr, outFrameBuffer);

		if (res != VK_SUCCESS)
		{
			Logger::Out("[VK] Failed to create framebuffer with code %d", OutputType::Error, res);
			return false;
		}

		destructors.insert_or_assign(destructors.begin(), *outFrameBuffer, (Destructor*)DestroyFramebuffer);
		devices[*outFrameBuffer] = device;

		return res == VK_SUCCESS;
	}

	bool VulkanAPI::CreateCommandPool(VkDevice device, uint32_t familyIndex, VkCommandPool* outPool)
	{
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = familyIndex;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		VkResult res = vkCreateCommandPool(device, &poolInfo, nullptr, outPool);

		if (res != VK_SUCCESS)
		{
			Logger::Out("[VK] Failed to create command pool with code %d", OutputType::Error, res);
			return false;
		}

		destructors.insert_or_assign(destructors.begin(), *outPool, (Destructor*)DestroyCommandPool);
		devices[*outPool] = device;

		return res == VK_SUCCESS;
	}

	bool VulkanAPI::CreateVkSemaphore(VkDevice device, VkSemaphore* outSemaphore)
	{
		VkSemaphoreTypeCreateInfo  type{};
		type.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
		type.initialValue = 0;
		type.semaphoreType = VK_SEMAPHORE_TYPE_BINARY;
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreInfo.pNext = &type;
		VkResult res = vkCreateSemaphore(device, &semaphoreInfo, nullptr, outSemaphore);

		if (res != VK_SUCCESS)
		{
			Logger::Out("[VK] Failed to create semaphore with code %d", OutputType::Error, res);
			return false;
		}

		destructors.insert_or_assign(destructors.begin(), *outSemaphore, (Destructor*)DestroySemaphore);
		devices[*outSemaphore] = device;

		return res == VK_SUCCESS;
	}

	bool VulkanAPI::CreateTimelineSemaphore(VkDevice device, VkSemaphore* outSemaphore)
	{
		VkSemaphoreTypeCreateInfo  type{};
		type.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
		type.initialValue = 0;
		type.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreInfo.pNext = &type;
		VkResult res = vkCreateSemaphore(device, &semaphoreInfo, nullptr, outSemaphore);

		if (res != VK_SUCCESS)
		{
			Logger::Out("[VK] Failed to create semaphore with code %d", OutputType::Error, res);
			return false;
		}

		destructors.insert_or_assign(destructors.begin(), *outSemaphore, (Destructor*)DestroySemaphore);
		devices[*outSemaphore] = device;

		return res == VK_SUCCESS;
	}

	bool VulkanAPI::CreateVkFence(VkDevice device, VkFence* outFence)
	{
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		VkResult res = vkCreateFence(device, &fenceInfo, nullptr, outFence);

		if (res != VK_SUCCESS)
		{
			Logger::Out("[VK] Failed to create fence with code %d", OutputType::Error, res);
			return false;
		}

		destructors.insert_or_assign(destructors.begin(), *outFence, (Destructor*)DestroyFence);
		devices[*outFence] = device;

		return res == VK_SUCCESS;
	}

	bool VulkanAPI::CreateImage(VmaAllocator allocator, VkImageCreateInfo* createInfo, VkImage* outImage, VmaAllocation* outAllocation, VmaMemoryUsage memUsage, VkMemoryPropertyFlags memFlags)
	{
		VmaAllocationCreateInfo imageAllocationCreateInfo{};
		imageAllocationCreateInfo.usage = memUsage;
		imageAllocationCreateInfo.preferredFlags = memFlags;
		imageAllocationCreateInfo.requiredFlags = 0; //CHECK THIS LATER

		VkResult res = vmaCreateImage(allocator, createInfo, &imageAllocationCreateInfo, outImage, outAllocation, nullptr);

		if (res != VK_SUCCESS)
		{
			Logger::Out("[VK] Failed to create image with code %d", OutputType::Error, res);
			return false;
		}

		destructors.insert_or_assign(destructors.begin(), *outImage, (Destructor*)DestroyImage);
		allocations[*outImage] = *outAllocation;
		allocators[*outAllocation] = allocator;

		return res == VK_SUCCESS;
	}

	bool VulkanAPI::CreateImageView(VkDevice device, VkFormat format, VkImage image, VkImageSubresourceRange subRange, VkImageView* target, VkImageViewType type)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.viewType = type;
		createInfo.format = format;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange = subRange;
		createInfo.image = image;

		VkResult res = vkCreateImageView(device, &createInfo, nullptr, target);
		if (res != VK_SUCCESS)
		{
			Logger::Out("[VK] Failed to create image with code %d", OutputType::Error, res);
			return false;
		}

		destructors.insert_or_assign(destructors.begin(), *target, (Destructor*)DestroyImageView);
		devices[*target] = device;

		return  res == VK_SUCCESS;
	}

	bool VulkanAPI::CreatePipelineLayout(VkDevice device, std::vector<VkPushConstantRange> pushConstants, std::vector<VkDescriptorSetLayout> descriptorLayouts, VkPipelineLayout* outLayout)
	{
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = descriptorLayouts.size();
		pipelineLayoutInfo.pSetLayouts = descriptorLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = pushConstants.size();
		pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();

		VkResult res = vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, outLayout);

		if (res != VK_SUCCESS)
		{
			Logger::Out("[VK] Failed to create pipeline layout with code %d", OutputType::Error, res);
			return false;
		}

		destructors.insert_or_assign(destructors.begin(), *outLayout, (Destructor*)DestroyPipelineLayout);
		devices[*outLayout] = device;

		return res == VK_SUCCESS;
	}

	bool VulkanAPI::CreateGraphicsPipeline(VkDevice device, VkGraphicsPipelineCreateInfo* info, VkPipeline* outPipeline)
	{
		VkResult res = vkCreateGraphicsPipelines(device, NULL, 1, info, nullptr, outPipeline);

		if (res != VK_SUCCESS)
		{
			Logger::Out("[VK] Failed to create pipeline with code %d", OutputType::Error, res);
			return false;
		}

		destructors.insert_or_assign(destructors.begin(), *outPipeline, (Destructor*)DestroyPipeline);
		devices[*outPipeline] = device;

		return res == VK_SUCCESS;
	}

	bool VulkanAPI::CreateComputePipeline(VkDevice device, VkComputePipelineCreateInfo* info, VkPipeline* outPipeline)
	{
		VkResult res = vkCreateComputePipelines(device, NULL, 1, info, nullptr, outPipeline);

		if (res != VK_SUCCESS)
		{
			Logger::Out("[VK] Failed to create pipeline with code %d", OutputType::Error, res);
			return false;
		}

		destructors.insert_or_assign(destructors.begin(), *outPipeline, (Destructor*)DestroyPipeline);
		devices[*outPipeline] = device;

		return res == VK_SUCCESS;
	}

	bool VulkanAPI::CreateDescriptorSetLayout(VkDevice device, std::vector<VkDescriptorSetLayoutBinding> bindings, VkDescriptorSetLayout* outLayout)
	{
		VkDescriptorSetLayoutCreateInfo descriptorLayout{};
		descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorLayout.pNext = NULL;
		descriptorLayout.bindingCount = bindings.size();
		descriptorLayout.pBindings = bindings.data();

		bool res = vkCreateDescriptorSetLayout(device, &descriptorLayout, NULL, outLayout) == VK_SUCCESS;
		destructors.insert_or_assign(destructors.begin(), *outLayout, (Destructor*)DestroyDescriptorLayout);
		devices[*outLayout] = device;

		return res;
	}

	bool VulkanAPI::CreateDescriptorPool(VkDevice device, std::vector<VkDescriptorPoolSize> pools, VkDescriptorPool* outDescriptorPool)
	{
		VkDescriptorPoolCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.maxSets = 1;
		createInfo.poolSizeCount = pools.size();
		createInfo.pPoolSizes = pools.data();
		createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		bool res = vkCreateDescriptorPool(device, &createInfo, NULL, outDescriptorPool) == VK_SUCCESS;
		destructors.insert_or_assign(destructors.begin(), *outDescriptorPool, (Destructor*)DestroyDescriptorPool);
		devices[*outDescriptorPool] = device;

		return res;
	}

	bool VulkanAPI::CreateSampler(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkSampler* outSampler, float mipLevels)
	{
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = mipLevels;
		
		VkResult res = vkCreateSampler(logicalDevice, &samplerInfo, nullptr, outSampler);

		if (res != VK_SUCCESS)
		{
			Logger::Out("[VK] Failed to create sampler with code %d", OutputType::Error, res);
			return false;
		}

		destructors[*outSampler] = (Destructor*)DestroySampler;
		devices[*outSampler] = logicalDevice;

		return res == VK_SUCCESS;
	}

	bool VulkanAPI::CreateSampler(VkDevice logicalDevice, VkSamplerCreateInfo* info, VkSampler* outSampler)
	{
		VkResult res = vkCreateSampler(logicalDevice, info, nullptr, outSampler);

		if (res != VK_SUCCESS)
		{
			Logger::Out("[VK] Failed to create sampler with code %d", OutputType::Error, res);
			return false;
		}

		destructors[*outSampler] = (Destructor*)DestroySampler;
		devices[*outSampler] = logicalDevice;

		return res == VK_SUCCESS;
	}

	void VulkanAPI::DestroyLogicalDevice(VkDevice device)
	{
		vkDestroyDevice(device, nullptr);
	}

	void VulkanAPI::DestroyMemoryAllocator(VmaAllocator allocator)
	{
		vmaDestroyAllocator(allocator);
	}

	void VulkanAPI::DestroyImageView(VkImageView view)
	{
		if (view != nullptr && destructors.erase(view) > 0)
		{
			vkDestroyImageView(devices[view], view, nullptr);
			devices.erase(view);
		}
	}

	void VulkanAPI::DestroyFence(VkFence fence)
	{
		if (fence != nullptr && destructors.erase(fence) > 0)
		{
			vkWaitForFences(devices[fence], 1, &fence, 1, UINT64_MAX);
			vkDestroyFence(devices[fence], fence, nullptr);
			devices.erase(fence);
		}
	}

	void VulkanAPI::DestroySemaphore(VkSemaphore semaphore)
	{
		if (semaphore != nullptr && destructors.erase(semaphore) > 0)
		{
			vkDestroySemaphore(devices[semaphore], semaphore, nullptr);
			devices.erase(semaphore);
		}
	}

	void VulkanAPI::DestroyCommandPool(VkCommandPool pool)
	{
		if (pool != nullptr && destructors.erase(pool) > 0)
		{
			vkDestroyCommandPool(devices[pool], pool, nullptr);
			devices.erase(pool);
		}
	}

	void VulkanAPI::DestroyFramebuffer(VkFramebuffer frameBuffer)
	{
		if (frameBuffer != nullptr && destructors.erase(frameBuffer) > 0)
		{
			vkDestroyFramebuffer(devices[frameBuffer], frameBuffer, nullptr);
			devices.erase(frameBuffer);
		}
	}

	void VulkanAPI::DestroyRenderPass(VkRenderPass renderPass)
	{
		if (renderPass != nullptr && destructors.erase(renderPass) > 0)
		{
			vkDestroyRenderPass(devices[renderPass], renderPass, nullptr);
			devices.erase(renderPass);
		}
	}

	void VulkanAPI::DestroySwapchainKHR(VkSwapchainKHR swapChain)
	{
		if (swapChain != nullptr && destructors.erase(swapChain) > 0)
		{
			vkDestroySwapchainKHR(devices[swapChain], swapChain, nullptr);
			devices.erase(swapChain);
		}
	}

	void VulkanAPI::DestroyImage(VkImage image)
	{
		if (image != nullptr && destructors.erase(image) > 0)
		{
			VmaAllocation alloc = allocations[image];
			VmaAllocator allocator = allocators[alloc];
			vmaDestroyImage(allocator, image, alloc);
			vmaFlushAllocation(allocator, alloc, 0, VK_WHOLE_SIZE);
			allocators.erase(alloc);
			allocations.erase(image);
		}
	}

	void  VulkanAPI::DestroyPipelineLayout(VkPipelineLayout layout)
	{
		if (layout != nullptr && destructors.erase(layout) > 0)
		{
			vkDestroyPipelineLayout(devices[layout], layout, nullptr);
			devices.erase(layout);
		}
	}

	void VulkanAPI::DestroyPipeline(VkPipeline pipeline)
	{
		if (pipeline != nullptr && destructors.erase(pipeline) > 0)
		{
			vkDestroyPipeline(devices[pipeline], pipeline, nullptr);
			devices.erase(pipeline);
		}
	}

	void VulkanAPI::DestroyDescriptorLayout(VkDescriptorSetLayout layout)
	{
		if (layout != nullptr && destructors.erase(layout) > 0)
		{
			vkDestroyDescriptorSetLayout(devices[layout], layout, nullptr);
			devices.erase(layout);
		}
	}

	void VulkanAPI::DestroyDescriptorPool(VkDescriptorPool pool)
	{
		if (pool != nullptr && destructors.erase(pool) > 0)
		{
			vkDestroyDescriptorPool(devices[pool], pool, nullptr);
			devices.erase(pool);
		}
	}

	void VulkanAPI::DestroySampler(VkSampler sampler)
	{
		if (sampler != nullptr && destructors.erase(sampler) > 0)
		{
			vkDestroySampler(devices[sampler], sampler, nullptr);
			devices.erase(sampler);
		}
	}

	void VulkanAPI::FreeCommandBuffer(VkCommandBuffer buffer)
	{
		if (buffer != nullptr && destructors.erase(buffer) > 0)
		{
			vkFreeCommandBuffers(devices[buffer], cmdPools[buffer], 1, &buffer);
			devices.erase(buffer);
			cmdPools.erase(buffer);
		}
	}

	void VulkanAPI::FreeCommandBuffers(VkCommandBuffer* buffers, size_t buffersCount)
	{
		for (int i = 0; i < buffersCount; i++)
		{
			if (buffers[i] != nullptr && destructors.erase(buffers[i]) > 0)
			{
				vkFreeCommandBuffers(devices[buffers[i]], cmdPools[buffers[i]], 1, &buffers[i]);
				devices.erase(buffers[i]);
				cmdPools.erase(buffers[i]);
			}
		}
	}

	void VulkanAPI::FreeDescriptorSet(VkDescriptorSet set)
	{
		if (set != nullptr && destructors.erase(set) > 0)
		{
			vkFreeDescriptorSets(devices[set], descriptors[set], 1, &set);
			devices.erase(set);
			descriptors.erase(set);
		}
	}

	void VulkanAPI::FreeDescriptorSets(VkDescriptorSet* sets, size_t setCount)
	{
		for (int i = 0; i < setCount; i++)
		{
			if (sets[i] != nullptr && destructors.erase(sets[i]) > 0)
			{
				vkFreeDescriptorSets(devices[sets[i]], descriptors[sets[i]], 1, &sets[i]);
				devices.erase(sets[i]);
				descriptors.erase(sets[i]);
			}
		}
	}

	bool VulkanAPI::GetDeviceQueue(VkDevice device, uint32_t family, VkQueue* outQueue)
	{
		vkGetDeviceQueue(device, family, 0, outQueue);
		return outQueue != nullptr;
	}

	bool VulkanAPI::AllocateCommandBuffers(VkDevice device, VkCommandPool pool, VkCommandBuffer* outBuffers, uint32_t outBuffersCount)
	{
		VkCommandBufferAllocateInfo cmdInfo = {};
		cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdInfo.pNext = NULL;
		cmdInfo.commandPool = pool;
		cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdInfo.commandBufferCount = outBuffersCount;

		bool res = vkAllocateCommandBuffers(device, &cmdInfo, outBuffers) == VK_SUCCESS;

		for (int i = 0; i < outBuffersCount; i++)
		{
			destructors.insert_or_assign(destructors.begin(), outBuffers[i], (Destructor*)FreeCommandBuffer);
			devices[outBuffers[i]] = device;
			cmdPools[outBuffers[i]] = pool;
		}

		return res;
	}

	bool VulkanAPI::AllocateDescriptorSet(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout, VkDescriptorSet* outSet)
	{
		VkDescriptorSetAllocateInfo descriptorAllocInfo{};
		descriptorAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorAllocInfo.pNext = NULL;
		descriptorAllocInfo.descriptorPool = pool;
		descriptorAllocInfo.descriptorSetCount = 1;
		descriptorAllocInfo.pSetLayouts = &layout;

		bool res = vkAllocateDescriptorSets(device, &descriptorAllocInfo, outSet) == VK_SUCCESS;
		destructors.insert_or_assign(destructors.begin(), *outSet, (Destructor*)FreeDescriptorSet);
		devices[*outSet] = device;
		descriptors[*outSet] = pool;

		return res;
	}

	std::vector<int32_t> VulkanAPI::FindFamilyIndicies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, std::vector<VkQueueFlagBits> families)
	{
		std::vector<int32_t> res;
		uint32_t queueFamilyCount = 0;
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		int f = 0;
		for (const auto& queueFamily : queueFamilies) 
		{
			if (queueFamily.queueFlags & families[f] && vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport) == VK_SUCCESS && presentSupport) {
				res.push_back(i);
				f++;
				if (f == families.size())
					return res;
			}

			i++;
		}

		return res;
	}

	bool VulkanAPI::BeginCommandBuffer(VkCommandBuffer cmd, VkCommandBufferUsageFlags usage)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = usage;

		return vkBeginCommandBuffer(cmd, &beginInfo) == VK_SUCCESS;
	}

	bool VulkanAPI::EndAndSubmitCommandBuffer(VkDevice device, VkCommandPool pool, VkCommandBuffer commandBuffer, VkQueue queue, VkFence fence)
	{
		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) return false;

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkResetFences(device, 1, &fence);
		vkQueueSubmit(queue, 1, &submitInfo, fence);
		vkQueueWaitIdle(queue);

		VulkanAPI::FreeCommandBuffer(commandBuffer);

		return true;
	}

	VkSampleCountFlagBits VulkanAPI::GetMaxSampleCount(VkPhysicalDevice physicalDevice)
	{
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

		VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
		if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
		if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
		if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
		if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
		if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
		if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

		return VK_SAMPLE_COUNT_1_BIT;
	}


	VkPresentModeKHR VulkanAPI::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, VkPresentModeKHR desiredMode)
	{
		for (const auto& availablePresentMode : availablePresentModes) 
		{
			if (availablePresentMode == desiredMode) {
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_MAILBOX_KHR;
	}

	VkSurfaceFormatKHR VulkanAPI::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& availableFormat : availableFormats) 
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkExtent2D VulkanAPI::ChooseSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != UINT32_MAX)
			return capabilities.currentExtent;
		else
		{
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	SwapChainSupportDetails VulkanAPI::QuerySwapChainSupport(VkSurfaceKHR surface, const VkPhysicalDevice physicalDevice)
	{
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	bool VulkanAPI::CheckDeviceExtensionSupport(const VkPhysicalDevice physicalDevice, std::vector<const char*> desired_extensions)
	{
		uint32_t extensionCount;

		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(desired_extensions.begin(), desired_extensions.end());

		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	bool VulkanAPI::CopyBufferToImage(VkDevice device, VkCommandPool pool, VkBuffer buffer, VkImage image, VkQueue graphicsQueue, VkFence graphicsFence, GrEngine::ImageInfo imgInfo, uint32_t length)
	{
		VkCommandBuffer commandBuffer;
		VulkanAPI::AllocateCommandBuffers(device, pool, &commandBuffer, 1);
		BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		std::vector<VkBufferImageCopy> regions;
		uint32_t offset = 0;

		for (int i = 0; i < length; i++)
		{
			VkBufferImageCopy region{};
			region.bufferOffset = offset;

			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = i;
			region.imageSubresource.layerCount = 1;

			region.imageOffset = { 0, 0, 0 };
			region.imageExtent = {
				imgInfo.width,
				imgInfo.height,
				1
			};

			regions.push_back(region);
			offset += imgInfo.width * imgInfo.height * imgInfo.channels;
		}


		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regions.size(), regions.data());

		return VulkanAPI::EndAndSubmitCommandBuffer(device, pool, commandBuffer, graphicsQueue, graphicsFence);
	}

	bool VulkanAPI::CopyBufferToImage(VkDevice device, VkCommandBuffer cmd, VkBuffer buffer, VkImage image, GrEngine::ImageInfo imgInfo, uint32_t length)
	{
		std::vector<VkBufferImageCopy> regions;
		uint32_t offset = 0;

		for (int i = 0; i < length; i++)
		{
			VkBufferImageCopy region{};
			region.bufferOffset = offset;

			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = i;
			region.imageSubresource.layerCount = 1;

			region.imageOffset = { 0, 0, 0 };
			region.imageExtent = {
				imgInfo.width,
				imgInfo.height,
				1
			};

			regions.push_back(region);
			offset += imgInfo.width * imgInfo.height * imgInfo.channels;
		}


		vkCmdCopyBufferToImage(cmd, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regions.size(), regions.data());

		return true;
	}

	bool VulkanAPI::TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subRange, VkCommandBuffer cmd)
	{
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange = subRange;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_ACCESS_MEMORY_READ_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else {
			throw std::invalid_argument("unsupported layout transition!");
		}

		vkCmdPipelineBarrier(cmd, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

#pragma region Structs

	VkDeviceCreateInfo VulkanAPI::StructDeviceCreateInfo(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures* deviceFeatures, VkDeviceQueueCreateInfo* deviceQueues, uint32_t queuesCount, const char* const* deviceExtensions, uint32_t extensionsCount)
	{
		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.enabledExtensionCount = extensionsCount;
		createInfo.ppEnabledExtensionNames = deviceExtensions;
		createInfo.queueCreateInfoCount = queuesCount;
		createInfo.pQueueCreateInfos = deviceQueues;
		createInfo.pEnabledFeatures = deviceFeatures;
		createInfo.enabledLayerCount = 0;

		return createInfo;
	}

	std::vector<VkDeviceQueueCreateInfo> VulkanAPI::StructQueueCreateInfo(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, std::vector<VkQueueFlagBits> families, float* priority)
	{
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::vector<int32_t> familyIndices = FindFamilyIndicies(physicalDevice, surface, families);

		if (familyIndices.size() == 0)
		{
			Logger::Out("Couldn't find suitable family indices!", OutputType::Error);
			return queueCreateInfos;
		}

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : familyIndices) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = priority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		return queueCreateInfos;
	}

	VkPhysicalDeviceFeatures VulkanAPI::StructPhysicalDeviceFeatures()
	{
		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.sampleRateShading = VK_TRUE;
		deviceFeatures.geometryShader = VK_TRUE;
		deviceFeatures.imageCubeArray = VK_TRUE;
		deviceFeatures.fragmentStoresAndAtomics = VK_TRUE;
		deviceFeatures.tessellationShader = VK_TRUE;
		deviceFeatures.samplerAnisotropy = VK_TRUE;
		deviceFeatures.fragmentStoresAndAtomics = VK_TRUE;
		deviceFeatures.independentBlend = VK_TRUE;
		deviceFeatures.depthBiasClamp = VK_TRUE;
		deviceFeatures.depthClamp = VK_TRUE;
		deviceFeatures.fullDrawIndexUint32 = VK_TRUE;
		deviceFeatures.shaderFloat64 = VK_TRUE;

		return deviceFeatures;
	}

	VkImageCreateInfo VulkanAPI::StructImageCreateInfo(VkExtent3D extent, VkFormat format, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageType type, VkSharingMode sharing)
	{

		VkImageCreateInfo ImageInfo{};
		ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		ImageInfo.imageType = type;
		ImageInfo.mipLevels = 1;
		ImageInfo.extent = extent;
		ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		ImageInfo.usage = usage;
		ImageInfo.format = format;
		ImageInfo.samples = samples;
		ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ImageInfo.sharingMode = sharing;
		ImageInfo.arrayLayers = 1;
		ImageInfo.mipLevels = 1;

		return ImageInfo;
	}

	VkImageSubresourceRange VulkanAPI::StructSubresourceRange(VkImageAspectFlags aspectMask, uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLevel, uint32_t arrayLevelCount)
	{
		VkImageSubresourceRange subRange{};
		subRange.aspectMask = aspectMask;
		subRange.baseMipLevel = baseMipLevel;
		subRange.levelCount = levelCount;
		subRange.baseArrayLayer = baseArrayLevel;
		subRange.layerCount = arrayLevelCount;

		return subRange;
	}

#pragma endregion
};