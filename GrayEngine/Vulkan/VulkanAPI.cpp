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
			std::advance(itt, offset);
			
			if (devices[(*itt).first] == logicalDevice || allocators[allocations[(*itt).first]] == allocator)
			{
				(*itt).second((*itt).first);
			}

			offset--;
		}

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

	bool VulkanAPI::m_createVkBuffer(VkDevice device, VmaAllocator allocator, const void* bufData, uint32_t dataSize, VkBufferUsageFlags usage, ShaderBuffer* shaderBuffer)
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
		vmaallocInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		vmaCreateBuffer(allocator, &bufferCreateInfo, &vmaallocInfo, &shaderBuffer->Buffer, &shaderBuffer->Allocation, nullptr);
		vkGetBufferMemoryRequirements(device, shaderBuffer->Buffer, &shaderBuffer->MemoryRequirements);

		if (bufData != nullptr)
		{
			vmaMapMemory(allocator, shaderBuffer->Allocation, (void**)&shaderBuffer->pData);
			memcpy(shaderBuffer->pData, bufData, dataSize);
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
			shader->pData = nullptr;
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
			//vmaDestroyImage(allocator, texture->newImage.allocatedImage, texture->newImage.allocation);
			//vmaFlushAllocation(allocator, texture->newImage.allocation, 0, sizeof(texture->newImage.allocation));
			VulkanAPI::DestroyImage(texture->newImage.allocatedImage);
			texture->initialized = false;
			texture->texture_collection.clear();
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

	bool VulkanAPI::CreateVkSwapchain(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, GLFWwindow* window, VkSurfaceKHR surface, VkSwapchainKHR* outSwapchain)
	{
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(surface, physicalDevice);

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
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
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapchainFormat;
		colorAttachment.samples = sampleCount;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depth_attachment{};
		depth_attachment.flags = 0;
		depth_attachment.format = depthFormat;
		depth_attachment.samples = sampleCount;
		depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		VkAttachmentReference depth_attachment_ref{};
		depth_attachment_ref.attachment = 1;
		depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription colorAttachmentResolve{};
		colorAttachmentResolve.format = swapchainFormat;
		colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		VkAttachmentReference colorAttachmentResolveRef{};
		colorAttachmentResolveRef.attachment = 2;
		colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depth_attachment_ref;
		subpass.pResolveAttachments = &colorAttachmentResolveRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		VkSubpassDependency depth_dependency = {};
		depth_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		depth_dependency.dstSubpass = 0;
		depth_dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		depth_dependency.srcAccessMask = 0;
		depth_dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		depth_dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		VkAttachmentDescription attachments[] = { colorAttachment, depth_attachment, colorAttachmentResolve };
		VkSubpassDependency dependecies[] = { dependency, depth_dependency };

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 3;
		renderPassInfo.pAttachments = attachments;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 2;
		renderPassInfo.pDependencies = dependecies;

		bool res = vkCreateRenderPass(device, &renderPassInfo, nullptr, outRenderPass) == VK_SUCCESS;

		destructors.insert_or_assign(destructors.begin(), *outRenderPass, (Destructor*)DestroyRenderPass);
		devices[*outRenderPass] = device;

		return res;
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

		bool res = vkCreateFramebuffer(device, &framebufferInfo, nullptr, outFrameBuffer) == VK_SUCCESS;

		destructors.insert_or_assign(destructors.begin(), *outFrameBuffer, (Destructor*)DestroyFramebuffer);
		devices[*outFrameBuffer] = device;

		return res;
	}

	bool VulkanAPI::CreateCommandPool(VkDevice device, uint32_t familyIndex, VkCommandPool* outPool)
	{
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = familyIndex;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		bool res = vkCreateCommandPool(device, &poolInfo, nullptr, outPool) == VK_SUCCESS;
		destructors.insert_or_assign(destructors.begin(), *outPool, (Destructor*)DestroyCommandPool);
		devices[*outPool] = device;

		return res;
	}

	bool VulkanAPI::CreateVkSemaphore(VkDevice device, VkSemaphore* outSemaphore)
	{
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		bool res = vkCreateSemaphore(device, &semaphoreInfo, nullptr, outSemaphore) == VK_SUCCESS;

		destructors.insert_or_assign(destructors.begin(), *outSemaphore, (Destructor*)DestroySemaphore);
		devices[*outSemaphore] = device;

		return res;
	}

	bool VulkanAPI::CreateVkFence(VkDevice device, VkFence* outFence)
	{
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		bool res = vkCreateFence(device, &fenceInfo, nullptr, outFence) == VK_SUCCESS;

		destructors.insert_or_assign(destructors.begin(), *outFence, (Destructor*)DestroyFence);
		devices[*outFence] = device;

		return res;
	}

	bool VulkanAPI::CreateImage(VmaAllocator allocator, VkImageCreateInfo* createInfo, VkImage* outImage, VmaAllocation* outAllocation, VmaMemoryUsage memUsage, VkMemoryPropertyFlags memFlags)
	{
		VmaAllocationCreateInfo imageAllocationCreateInfo{};
		imageAllocationCreateInfo.usage = memUsage;
		imageAllocationCreateInfo.preferredFlags = memFlags;
		imageAllocationCreateInfo.requiredFlags = 0; //CHECK THIS LATER

		bool res = vmaCreateImage(allocator, createInfo, &imageAllocationCreateInfo, outImage, outAllocation, nullptr) == VK_SUCCESS;

		destructors.insert_or_assign(destructors.begin(), *outImage, (Destructor*)DestroyImage);
		allocations[*outImage] = *outAllocation;
		allocators[*outAllocation] = allocator;

		return res;
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

		bool res = vkCreateImageView(device, &createInfo, nullptr, target) == VK_SUCCESS;

		destructors.insert_or_assign(destructors.begin(), *target, (Destructor*)DestroyImageView);
		devices[*target] = device;

		return  res;
	}

	bool VulkanAPI::CreatePipelineLayout(VkDevice device, std::vector<VkPushConstantRange> pushConstants, std::vector<VkDescriptorSetLayout> descriptorLayouts, VkPipelineLayout* outLayout)
	{
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = descriptorLayouts.size();
		pipelineLayoutInfo.pSetLayouts = descriptorLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = pushConstants.size();
		pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();

		bool res = vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, outLayout) == VK_SUCCESS;
		destructors.insert_or_assign(destructors.begin(), *outLayout, (Destructor*)DestroyPipelineLayout);
		devices[*outLayout] = device;

		return res;
	}

	bool VulkanAPI::CreateGraphicsPipeline(VkDevice device, VkGraphicsPipelineCreateInfo* info, VkPipeline* outPipeline)
	{
		bool res = vkCreateGraphicsPipelines(device, NULL, 1, info, nullptr, outPipeline);
		destructors.insert_or_assign(destructors.begin(), *outPipeline, (Destructor*)DestroyPipeline);
		devices[*outPipeline] = device;

		return res;
	}

	bool VulkanAPI::CreateComputePipeline(VkDevice device, VkComputePipelineCreateInfo* info, VkPipeline* outPipeline)
	{
		bool res = vkCreateComputePipelines(device, NULL, 1, info, nullptr, outPipeline);
		destructors.insert_or_assign(destructors.begin(), *outPipeline, (Destructor*)DestroyPipeline);
		devices[*outPipeline] = device;

		return res;
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
		
		bool res = vkCreateSampler(logicalDevice, &samplerInfo, nullptr, outSampler) == VK_SUCCESS;
		destructors[*outSampler] = (Destructor*)DestroySampler;
		devices[*outSampler] = logicalDevice;

		return res;
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
		if (destructors.erase(view) > 0)
		{
			vkDestroyImageView(devices[view], view, nullptr);
			devices.erase(view);
		}
	}

	void VulkanAPI::DestroyFence(VkFence fence)
	{
		if (destructors.erase(fence) > 0)
		{
			vkDestroyFence(devices[fence], fence, nullptr);
			devices.erase(fence);
		}
	}

	void VulkanAPI::DestroySemaphore(VkSemaphore semaphore)
	{
		if (destructors.erase(semaphore) > 0)
		{
			vkDestroySemaphore(devices[semaphore], semaphore, nullptr);
			devices.erase(semaphore);
		}
	}

	void VulkanAPI::DestroyCommandPool(VkCommandPool pool)
	{
		if (destructors.erase(pool) > 0)
		{
			vkDestroyCommandPool(devices[pool], pool, nullptr);
			devices.erase(pool);
		}
	}

	void VulkanAPI::DestroyFramebuffer(VkFramebuffer frameBuffer)
	{
		if (destructors.erase(frameBuffer) > 0)
		{
			vkDestroyFramebuffer(devices[frameBuffer], frameBuffer, nullptr);
			devices.erase(frameBuffer);
		}
	}

	void VulkanAPI::DestroyRenderPass(VkRenderPass renderPass)
	{
		if (destructors.erase(renderPass) > 0)
		{
			vkDestroyRenderPass(devices[renderPass], renderPass, nullptr);
			devices.erase(renderPass);
		}
	}

	void VulkanAPI::DestroySwapchainKHR(VkSwapchainKHR swapChain)
	{
		if (destructors.erase(swapChain) > 0)
		{
			vkDestroySwapchainKHR(devices[swapChain], swapChain, nullptr);
			devices.erase(swapChain);
		}
	}

	void VulkanAPI::DestroyImage(VkImage image)
	{
		if (destructors.erase(image) > 0)
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
		if (destructors.erase(layout) > 0)
		{
			vkDestroyPipelineLayout(devices[layout], layout, nullptr);
			devices.erase(layout);
		}
	}

	void VulkanAPI::DestroyPipeline(VkPipeline pipeline)
	{
		if (destructors.erase(pipeline) > 0)
		{
			vkDestroyPipeline(devices[pipeline], pipeline, nullptr);
			devices.erase(pipeline);
		}
	}

	void VulkanAPI::DestroyDescriptorLayout(VkDescriptorSetLayout layout)
	{
		if (destructors.erase(layout) > 0)
		{
			vkDestroyDescriptorSetLayout(devices[layout], layout, nullptr);
			devices.erase(layout);
		}
	}

	void VulkanAPI::DestroyDescriptorPool(VkDescriptorPool pool)
	{
		if (destructors.erase(pool) > 0)
		{
			vkDestroyDescriptorPool(devices[pool], pool, nullptr);
			devices.erase(pool);
		}
	}

	void VulkanAPI::DestroySampler(VkSampler sampler)
	{
		if (destructors.erase(sampler) > 0)
		{
			vkDestroySampler(devices[sampler], sampler, nullptr);
			devices.erase(sampler);
		}
	}

	void VulkanAPI::FreeCommandBuffer(VkCommandBuffer buffer)
	{
		if (destructors.erase(buffer) > 0)
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
			if (destructors.erase(buffers[i]) > 0)
			{
				vkFreeCommandBuffers(devices[buffers[i]], cmdPools[buffers[i]], 1, &buffers[i]);
				devices.erase(buffers[i]);
				cmdPools.erase(buffers[i]);
			}
		}
	}

	void VulkanAPI::FreeDescriptorSet(VkDescriptorSet set)
	{
		if (destructors.erase(set) > 0)
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
			if (destructors.erase(sets[i]) > 0)
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


	VkPresentModeKHR VulkanAPI::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const auto& availablePresentMode : availablePresentModes) 
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
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

	bool VulkanAPI::CopyBufferToImage(VkDevice device, VkCommandPool pool, VkBuffer buffer, VkImage image, VkQueue graphicsQueue, VkFence graphicsFence, ImageInfo imgInfo, uint32_t length)
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
			Logger::Out("Couldn't find suitable family indices!", OutputColor::Red, OutputType::Error);
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