#include <pch.h>
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "VulkanRenderer.h"


float GrEngine::Renderer::NearPlane = 0.1f;
float GrEngine::Renderer::FarPlane = 1000.f;

namespace GrEngine_Vulkan
{
	void VulkanRenderer::destroy()
	{
		Initialized = false;

		waitForRenderer();

		while (entities.size() > 0)
		{
			std::map<UINT, GrEngine::Entity*>::iterator pos = entities.begin();
			switch ((*pos).second->GetEntityType())
			{
			case GrEngine::EntityType::ObjectEntity:
				static_cast<VulkanObject*>(drawables[(*pos).first])->destroyObject();
				delete drawables[(*pos).first];
				drawables.erase((*pos).first);
				break;
			case GrEngine::EntityType::SkyboxEntity:
				static_cast<VulkanSkybox*>((*pos).second)->destroyObject();
				break;
			case GrEngine::EntityType::TerrainEntity:
				static_cast<VulkanTerrain*>((*pos).second)->destroyObject();
				break;
			case GrEngine::EntityType::SpotlightEntity:
				static_cast<VulkanSpotlight*>(lights[(*pos).first])->destroyLight();
				delete lights[(*pos).first];
				lights.erase((*pos).first);
				break;
			}

			delete (*pos).second;
			entities.erase((*pos).first);
		}

		//vmaUnmapMemory(memAllocator, viewProjUBO.Allocation);
		//vmaUnmapMemory(memAllocator, shadowBuffer.Allocation);
		//vmaUnmapMemory(memAllocator, cascadeBuffer.Allocation);

		VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &viewProjUBO);
		VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &shadowBuffer);
		VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &cascadeBuffer);

		VulkanAPI::DestroyFramebuffer(shadowFramebuffer);
		VulkanAPI::DestroySampler(shadowMap.textureSampler);
		VulkanAPI::DestroyImageView(shadowMap.textureImageView);
		VulkanAPI::DestroyImage(shadowMap.newImage.allocatedImage);

		VulkanAPI::DestroyFence(transitionFence);
		VulkanAPI::DestroyFence(loadFence);

		cleanupSwapChain();
		resources.Clean(logicalDevice, memAllocator);

		VulkanAPI::FreeCommandBuffers(commandBuffers.data(), commandBuffers.size());

		waitForRenderer();
		vkDeviceWaitIdle(logicalDevice);
		VulkanAPI::Destroy(logicalDevice, memAllocator);

#ifdef VALIDATION
		DestroyDebugUtilsMessengerEXT(_vulkan, debugMessenger, nullptr);
#endif // VALIDATION

		vkDestroySurfaceKHR(_vulkan, surface, nullptr);
		vkDestroyInstance(_vulkan, nullptr);
	}

	bool VulkanRenderer::init(void* window) //Vulkan integration done with a help of vulkan-tutorial.com
	{
		bool res = true;
		pParentWindow = static_cast<GLFWwindow*>(window);

		if (!createVKInstance())
#ifdef _DEBUG
			throw std::runtime_error("Failed to create vulkan instance!");
#else
			return false;
#endif

		uint32_t deviceCount = 0;

		vkEnumeratePhysicalDevices(_vulkan, &deviceCount, nullptr);
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(_vulkan, &deviceCount, devices.data());

		if (deviceCount == 0)
#ifdef _DEBUG
			throw std::runtime_error("Failed to find GPUs with Vulkan support!");
#else
			return false;
#endif

		if (!glfwCreateWindowSurface(_vulkan, pParentWindow, nullptr, &surface) == VK_SUCCESS)
			Logger::Out("[Vk] Failed to create presentation surface", OutputColor::Red, OutputType::Error);

		for (const auto& device : devices)
		{
			if (isDeviceSuitable(device))
			{
				physicalDevice = device;
				vkGetPhysicalDeviceProperties(physicalDevice, &deviceProps);
				//msaaSamples = VulkanAPI::GetMaxSampleCount(physicalDevice);
				msaaSamples = VK_SAMPLE_COUNT_1_BIT;
				break;
			}
		}

		if (physicalDevice == VK_NULL_HANDLE)
#ifdef _DEBUG
			throw std::exception("Failed to find suitable Vulkan device");
#else
			return false;
#endif
		else
			Logger::Out("Presentation device: %s", OutputColor::Green, OutputType::Log, deviceProps.deviceName);


		float queuePriority = 1.f;
		VkPhysicalDeviceFeatures deviceFeatures = VulkanAPI::StructPhysicalDeviceFeatures();
		std::vector<VkDeviceQueueCreateInfo> deviceQueues = VulkanAPI::StructQueueCreateInfo(physicalDevice, surface, { VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_COMPUTE_BIT }, &queuePriority);
		graphics_bit = deviceQueues[0].queueFamilyIndex;
		compute_bit = deviceQueues[1].queueFamilyIndex;
		VkDeviceCreateInfo deviceInfo = VulkanAPI::StructDeviceCreateInfo(physicalDevice, &deviceFeatures, deviceQueues.data(), deviceQueues.size(), deviceExtensions.data(), deviceExtensions.size());

		if ((res = VulkanAPI::CreateLogicalDevice(physicalDevice, &deviceInfo, &logicalDevice) & res) == false)
			Logger::Out("[Vk] Failed to create logical device", OutputColor::Red, OutputType::Error);

		if ((res = VulkanAPI::GetDeviceQueue(logicalDevice, deviceQueues[0].queueFamilyIndex, &graphicsQueue) & res) == false)
			Logger::Out("[Vk] Failed to get graphics queue!", OutputColor::Red, OutputType::Error);

		if ((res = VulkanAPI::GetDeviceQueue(logicalDevice, deviceQueues[0].queueFamilyIndex, &presentQueue) & res) == false)
			Logger::Out("[Vk] Failed to get present queue!", OutputColor::Red, OutputType::Error);

		if ((res = VulkanAPI::CreateVulkanMemoryAllocator(_vulkan, physicalDevice, logicalDevice, &memAllocator) & res) == false)
			Logger::Out("[Vk] Failed to create memory allocator", OutputColor::Red, OutputType::Error);


		if ((res = VulkanAPI::CreateVkSwapchain(physicalDevice, logicalDevice, pParentWindow, surface, &swapChain) & createSwapChainImages() & res) == false)
			Logger::Out("[Vk] Failed to create swap chain", OutputColor::Red, OutputType::Error);

		if ((res = VulkanAPI::CreateRenderPass(logicalDevice, swapChainImageFormat, depthFormat, msaaSamples, &renderPass) & res) == false)
			Logger::Out("[Vk] Failed to create render pass", OutputColor::Red, OutputType::Error);

		if ((res = VulkanAPI::CreateCommandPool(logicalDevice, deviceQueues[0].queueFamilyIndex, &commandPool) & res) == false)
			Logger::Out("[Vk] Failed to create command pool", OutputColor::Red, OutputType::Error);

		commandBuffers.resize(swapChainImageViews.size());
		max_async_frames = swapChainImageViews.size();
		imageAvailableSemaphore.resize(max_async_frames);
		renderFinishedSemaphore.resize(max_async_frames);
		renderFence.resize(max_async_frames);
		if ((res = VulkanAPI::AllocateCommandBuffers(logicalDevice, commandPool, commandBuffers.data(), commandBuffers.size()) & res) == false)
			Logger::Out("[Vk] Failed to create command buffer", OutputColor::Red, OutputType::Error);

		for (std::vector<VkSemaphore>::iterator itt = renderFinishedSemaphore.begin(); itt != renderFinishedSemaphore.end(); ++itt)
		{
			if ((res = VulkanAPI::CreateVkSemaphore(logicalDevice, &(*itt)) & res) == false)
				Logger::Out("[Vk] Failed to create semaphores", OutputColor::Red, OutputType::Error);
		}

		for (std::vector<VkSemaphore>::iterator itt = imageAvailableSemaphore.begin(); itt != imageAvailableSemaphore.end(); ++itt)
		{
			if ((res = VulkanAPI::CreateVkSemaphore(logicalDevice, &(*itt)) & res) == false)
				Logger::Out("[Vk] Failed to create semaphores", OutputColor::Red, OutputType::Error);
		}

		for (std::vector<VkFence>::iterator itt = renderFence.begin(); itt != renderFence.end(); ++itt)
		{
			if ((res = VulkanAPI::CreateVkFence(logicalDevice, &(*itt)) & res) == false)
				Logger::Out("[Vk] Failed to create fences", OutputColor::Red, OutputType::Error);
		}

		VulkanAPI::CreateVkFence(logicalDevice, &transitionFence);
		VulkanAPI::CreateVkFence(logicalDevice, &loadFence);
		
		createAttachment(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, &position);
		createAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, &normal);
		createAttachment(swapChainImageFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, &albedo);

		VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, nullptr, sizeof(vpUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, &viewProjUBO, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		prepareShadowPass();
		prepareCompositionPass();
		prepareTransparencyPass();
		prepareSamplingPass();
		prepareSelectionPass();

		VkImageView clrattachments[] = { colorImage.textureImageView, position.textureImageView, normal.textureImageView, albedo.textureImageView, depthImageView };

		if ((res = VulkanAPI::CreateFrameBuffer(logicalDevice, renderPass, clrattachments, 5, swapChainExtent, &defferFramebuffer) & res) == false)
			Logger::Out("[Vk] Failed to create framebuffer", OutputColor::Red, OutputType::Error);

		swapChainFramebuffers.resize(swapChainImageViews.size());
		for (std::size_t i = 0; i < swapChainImageViews.size(); i++)
		{
			VkImageView ppattachments[] = { swapChainImageViews[i], depthImageView, };

			if ((res = VulkanAPI::CreateFrameBuffer(logicalDevice, samplingPass, ppattachments, 2, swapChainExtent, &swapChainFramebuffers[i]) & res) == false)
				Logger::Out("[Vk] Failed to create framebuffer", OutputColor::Red, OutputType::Error);
		}

		initDefaultViewport();
		initSkyEntity();
			
		vkDeviceWaitIdle(logicalDevice);
		Logger::Out("Initialized device %p", OutputColor::Blue, OutputType::Log, logicalDevice);

		return Initialized = res;
	}

	void VulkanRenderer::initSkyEntity()
	{
		sky = new VulkanSkybox(1000000000);
		sky->initObject(logicalDevice, memAllocator, this);
		sky->UpdateNameTag("Sky");
		sky->MakeStatic();
		entities[sky->GetEntityID()] = sky;
	}


	void VulkanRenderer::initDefaultViewport()
	{
		viewport_camera = new GrEngine::Camera();
	}

	void VulkanRenderer::createAttachment(VkFormat format, VkImageUsageFlags usage, Texture* attachment)
	{
		VkImageAspectFlags aspectMask = 0;
		VkImageLayout imageLayout;

		VulkanAPI::DestroyImage(attachment->newImage.allocatedImage);
		VulkanAPI::DestroyImageView(attachment->textureImageView);

		if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		{
			aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}
		if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}

		VkImageCreateInfo imageInfo = VulkanAPI::StructImageCreateInfo({ swapChainExtent.width, swapChainExtent.height, 1 }, format, VK_SAMPLE_COUNT_1_BIT, usage);

		VulkanAPI::CreateImage(memAllocator, &imageInfo, &attachment->newImage.allocatedImage, &attachment->newImage.allocation);

		VkImageSubresourceRange imageView{};
		imageView.aspectMask = aspectMask;
		imageView.baseMipLevel = 0;
		imageView.levelCount = 1;
		imageView.baseArrayLayer = 0;
		imageView.layerCount = 1;
		VulkanAPI::CreateImageView(logicalDevice, format, attachment->newImage.allocatedImage, imageView, &attachment->textureImageView);

		attachment->initialized = true;
		attachment->texInfo.mipLevels = 1;
		attachment->texInfo.format = format;
		attachment->texInfo.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		attachment->texInfo.descriptor.imageView = attachment->textureImageView;
		attachment->texInfo.descriptor.sampler = VK_NULL_HANDLE;
	}

	void VulkanRenderer::createSkybox(const char* East, const char* West, const char* Top, const char* Bottom, const char* North, const char* South)
	{
		Initialized = false;

		std::array<std::string, 6> mat_vector = { East, West, Top, Bottom, North, South };
		static_cast<CubemapProperty*>(sky->GetProperty(PropertyType::Cubemap))->SetPropertyValue(mat_vector);

		Initialized = true;
	}

	void VulkanRenderer::RenderFrame()
	{
		drawFrame(swapChainExtent);
	}

	void VulkanRenderer::drawFrame(VkExtent2D extent)
	{
		if (!Initialized) return;

		currentImageIndex = 0;
		int frame = currentFrame;
		vkAcquireNextImageKHR(logicalDevice, swapChain, UINT64_MAX, imageAvailableSemaphore[frame], VK_NULL_HANDLE, &currentImageIndex);

		vkWaitForFences(logicalDevice, 1, &renderFence[frame], TRUE, UINT64_MAX);
		vkResetFences(logicalDevice, 1, &renderFence[frame]);

		if (!updateDrawables(frame, DrawMode::NORMAL, extent))
		{
			Logger::Out("Logical device was lost!", OutputColor::Red, OutputType::Error);
#ifdef _DEBUG
			throw std::runtime_error("Logical device was lost!");
#else
			glfwSetWindowShouldClose(pParentWindow, true);
			return;
#endif
		}

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { imageAvailableSemaphore[frame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[frame];
		VkSemaphore signalSemaphores[] = { renderFinishedSemaphore[frame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		VkResult res;
		res = vkQueueSubmit(graphicsQueue, 1, &submitInfo, renderFence[frame]);

		if (res == VK_ERROR_DEVICE_LOST)
		{
			Logger::Out("Logical device was lost!", OutputColor::Red, OutputType::Error);
#ifdef _DEBUG
			throw std::runtime_error("Logical device was lost!");
#else
			glfwSetWindowShouldClose(pParentWindow, true);
			return;
#endif
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &currentImageIndex;
		presentInfo.pResults = nullptr; // Optional

		vkQueuePresentKHR(presentQueue, &presentInfo);

		previousFrame = currentFrame;
		currentFrame = (currentFrame + 1) % max_async_frames;
	}
	void VulkanRenderer::SaveScreenshot(const char* filepath)
	{
		waitForRenderer();
		vkDeviceWaitIdle(logicalDevice);

		VkImage srcImage = swapChainImages[currentFrame];
		VkImage dstImage;
		VkCommandBuffer cmd;
		VkImageCreateInfo dstInf{};
		dstInf.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		dstInf.imageType = VK_IMAGE_TYPE_2D;
		dstInf.format = swapChainImageFormat;
		dstInf.extent.width = swapChainExtent.width;
		dstInf.extent.height = swapChainExtent.height;
		dstInf.extent.depth = 1;
		dstInf.arrayLayers = 1;
		dstInf.mipLevels = 1;
		dstInf.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		dstInf.samples = VK_SAMPLE_COUNT_1_BIT;
		dstInf.tiling = VK_IMAGE_TILING_LINEAR;
		dstInf.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		vkCreateImage(logicalDevice, &dstInf, nullptr, &dstImage);

		VkMemoryRequirements memRequirements;
		VkMemoryAllocateInfo memAllocInfo{};
		VkDeviceMemory dstImageMemory;
		vkGetImageMemoryRequirements(logicalDevice, dstImage, &memRequirements);
		memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memAllocInfo.allocationSize = memRequirements.size;
		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		allocInfo.flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		vmaFindMemoryTypeIndex(memAllocator, memRequirements.memoryTypeBits, &allocInfo, &memAllocInfo.memoryTypeIndex);
		vkAllocateMemory(logicalDevice, &memAllocInfo, nullptr, &dstImageMemory);
		vkBindImageMemory(logicalDevice, dstImage, dstImageMemory, 0);

		VulkanAPI::AllocateCommandBuffers(logicalDevice, commandPool, &cmd, 1);
		VulkanAPI::BeginCommandBuffer(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		VulkanAPI::TransitionImageLayout(dstImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VulkanAPI::StructSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT), cmd);
		VulkanAPI::TransitionImageLayout(srcImage, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VulkanAPI::StructSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT), cmd);

		VkImageCopy imageCopyRegion{};
		imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.srcSubresource.layerCount = 1;
		imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.dstSubresource.layerCount = 1;
		imageCopyRegion.extent.width = swapChainExtent.width;
		imageCopyRegion.extent.height = swapChainExtent.height;
		imageCopyRegion.extent.depth = 1;

		vkCmdCopyImage(cmd, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);

		vkWaitForFences(logicalDevice, 1, &transitionFence, VK_TRUE, UINT64_MAX);
		VulkanAPI::TransitionImageLayout(dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VulkanAPI::StructSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT), cmd);
		VulkanAPI::TransitionImageLayout(srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VulkanAPI::StructSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT), cmd);
		VulkanAPI::EndAndSubmitCommandBuffer(logicalDevice, commandPool, cmd, graphicsQueue, transitionFence);

		vkWaitForFences(logicalDevice, 1, &transitionFence, VK_TRUE, UINT64_MAX);
		if (filepath != "")
		{
			VkImageSubresource subResource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
			VkSubresourceLayout subResourceLayout;
			vkGetImageSubresourceLayout(logicalDevice, dstImage, &subResource, &subResourceLayout);

			unsigned char* data;
			size_t arr_s = subResourceLayout.size;

			vkMapMemory(logicalDevice, dstImageMemory, 0, arr_s, 0, (void**)&data);
			unsigned char* pixels = new unsigned char[arr_s];

			//BGRA to RGBA
			for (int offset = subResourceLayout.offset; offset < subResourceLayout.size; offset+=4)
			{
				pixels[offset] = data[offset + 2];
				pixels[offset + 1] = data[offset + 1];
				pixels[offset + 2] = data[offset];
				pixels[offset + 3] = 255;
			}
			
			int i = stbi_write_png(filepath, swapChainExtent.width, swapChainExtent.height, 4, pixels, subResourceLayout.rowPitch);
			delete[] pixels;
			vkUnmapMemory(logicalDevice, dstImageMemory);
		}

		vkFreeMemory(logicalDevice, dstImageMemory, nullptr);
		vkDestroyImage(logicalDevice, dstImage, nullptr);
	}

	void VulkanRenderer::SelectEntityAtCursor()
	{
		if (!Initialized) return;

		waitForRenderer();

		uint32_t index = 0;
		int frame = currentFrame;
		//vkAcquireNextImageKHR(logicalDevice, swapChain, UINT64_MAX, imageAvailableSemaphore[frame], VK_NULL_HANDLE, &index);
		vkWaitForFences(logicalDevice, 1, &renderFence[frame], TRUE, UINT64_MAX);

		VkClearValue clearValues[2];
		clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };
		VkRenderPassBeginInfo renderPassInfo{};
		VkViewport viewport{};
		VkRect2D scissor{};

		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.framebuffer = selectionFramebuffer;
		renderPassInfo.renderPass = selectionPass;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChainExtent;
		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = clearValues;

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		vkBeginCommandBuffer(commandBuffers[frame], &beginInfo);
		vkCmdBeginRenderPass(commandBuffers[frame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		viewport.x = 0;
		viewport.y = 0;
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffers[frame], 0, 1, &viewport);
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;
		vkCmdSetScissor(commandBuffers[frame], 0, 1, &scissor);

		for (std::map<UINT, GrEngine::Object*>::iterator itt = drawables.begin(); itt != drawables.end(); ++itt)
		{
			if (static_cast<VulkanObject*>((*itt).second)->IsVisible())
			{
				static_cast<VulkanObject*>((*itt).second)->recordSelection(commandBuffers[frame], swapChainExtent, DrawMode::NORMAL);
			}
		}

		vkCmdEndRenderPass(commandBuffers[frame]);
		vkEndCommandBuffer(commandBuffers[frame]);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[frame];
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;

		VkResult res;
		vkResetFences(logicalDevice, 1, &renderFence[frame]);
		res = vkQueueSubmit(graphicsQueue, 1, &submitInfo, renderFence[frame]);

		if (res == VK_ERROR_DEVICE_LOST)
		{
			Logger::Out("Logical device was lost!", OutputColor::Red, OutputType::Error);
#ifdef _DEBUG
			throw std::runtime_error("Logical device was lost!");
#else
			glfwSetWindowShouldClose(pParentWindow, true);
			return;
#endif

		}

		vkWaitForFences(logicalDevice, 1, &renderFence[frame], TRUE, UINT64_MAX);

		POINTFLOAT cur = GrEngine::Engine::GetContext()->GetCursorPosition();

		VkCommandBuffer commandBuffer;
		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { (int)cur.x, (int)cur.y, 0 };
		region.imageExtent = {
			(uint32_t)1,
			(uint32_t)1,
			1
		};

		ShaderBuffer stagingBuffer;
		VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, nullptr, sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_DST_BIT, &stagingBuffer);
		VkImageSubresourceRange subresourceRange = VulkanAPI::StructSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1);

		VulkanAPI::AllocateCommandBuffers(logicalDevice, commandPool, &commandBuffer, 1);
		VulkanAPI::BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		VulkanAPI::TransitionImageLayout(identity.newImage.allocatedImage, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange, commandBuffer);
		vkCmdCopyImageToBuffer(commandBuffer, identity.newImage.allocatedImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer.Buffer, 1, &region);
		VulkanAPI::TransitionImageLayout(identity.newImage.allocatedImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, subresourceRange, commandBuffer);
		VulkanAPI::EndAndSubmitCommandBuffer(logicalDevice, commandPool, commandBuffer, graphicsQueue, renderFence[frame]);

		void* data;
		vmaMapMemory(memAllocator, stagingBuffer.Allocation, (void**)&data);
		uint32_t sel;
		memcpy(&sel, data, sizeof(uint32_t));
		vmaUnmapMemory(memAllocator, stagingBuffer.Allocation);

		VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &stagingBuffer);

		selectEntity(sel);
		//currentFrame = (currentFrame + 1) % max_async_frames;
		//Logger::Out("%d", OutputColor::Gray, OutputType::Log, sel);
	}

	std::array<byte, 3> VulkanRenderer::GetPixelColorAtCursor()
	{
		//drawFrame(swapChainExtent);
		waitForRenderer();

		VkImage srcImage = albedo.newImage.allocatedImage;
		VkImage dstImage;
		VkCommandBuffer cmd;
		VkImageCreateInfo dstInf = VulkanAPI::StructImageCreateInfo({ swapChainExtent.width, swapChainExtent.height, 1 }, VK_FORMAT_B8G8R8A8_SRGB, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSFER_DST_BIT);
		dstInf.tiling = VK_IMAGE_TILING_LINEAR;
		vkCreateImage(logicalDevice, &dstInf, nullptr, &dstImage);

		VkMemoryRequirements memRequirements;
		VkMemoryAllocateInfo memAllocInfo{};
		VkDeviceMemory dstImageMemory;
		vkGetImageMemoryRequirements(logicalDevice, dstImage, &memRequirements);
		memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memAllocInfo.allocationSize = memRequirements.size;
		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		allocInfo.flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		vmaFindMemoryTypeIndex(memAllocator, memRequirements.memoryTypeBits, &allocInfo, &memAllocInfo.memoryTypeIndex);
		vkAllocateMemory(logicalDevice, &memAllocInfo, nullptr, &dstImageMemory);
		vkBindImageMemory(logicalDevice, dstImage, dstImageMemory, 0);

		VulkanAPI::AllocateCommandBuffers(logicalDevice, commandPool, &cmd, 1);
		VulkanAPI::BeginCommandBuffer(cmd, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		VulkanAPI::TransitionImageLayout(dstImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VulkanAPI::StructSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT), cmd);
		VulkanAPI::TransitionImageLayout(srcImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VulkanAPI::StructSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT), cmd);

		VkImageCopy imageCopyRegion{};
		imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.srcSubresource.layerCount = 1;
		imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.dstSubresource.layerCount = 1;
		imageCopyRegion.extent.width = swapChainExtent.width;
		imageCopyRegion.extent.height = swapChainExtent.height;
		imageCopyRegion.extent.depth = 1;

		vkCmdCopyImage(cmd, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);

		vkWaitForFences(logicalDevice, 1, &transitionFence, VK_TRUE, UINT64_MAX);
		VulkanAPI::TransitionImageLayout(dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VulkanAPI::StructSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT), cmd);
		VulkanAPI::TransitionImageLayout(srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VulkanAPI::StructSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT), cmd);
		VulkanAPI::EndAndSubmitCommandBuffer(logicalDevice, commandPool, cmd, graphicsQueue, transitionFence);

		VkImageSubresource subResource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
		VkSubresourceLayout subResourceLayout;
		vkGetImageSubresourceLayout(logicalDevice, dstImage, &subResource, &subResourceLayout);

		const char* data;
		vkMapMemory(logicalDevice, dstImageMemory, 0, VK_WHOLE_SIZE, 0, (void**)&data);
		data += subResourceLayout.offset;
		double xpos, ypos;
		glfwGetCursorPos(pParentWindow, &xpos, &ypos);
		data += subResourceLayout.rowPitch * (int)(ypos);
		unsigned int* row = (unsigned int*)data + (int)(xpos);

		double r = ((unsigned char*)row + 2)[0];
		double g = ((unsigned char*)row + 1)[0];
		double b = ((unsigned char*)row)[0];

		vkUnmapMemory(logicalDevice, dstImageMemory);
		vkFreeMemory(logicalDevice, dstImageMemory, nullptr);
		vkDestroyImage(logicalDevice, dstImage, nullptr);

		return { (byte)r, (byte)g, (byte)b };
	}

	void VulkanRenderer::prepareCompositionPass()
	{
		std::vector<VkDescriptorPoolSize> poolSizes;
		poolSizes.resize(3);
		poolSizes[0].descriptorCount = 5;
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = 4;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		poolSizes[2].descriptorCount = 3;
		poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;
		setLayoutBindings.resize(7);
		setLayoutBindings[0].binding = 0;
		setLayoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		setLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		setLayoutBindings[0].descriptorCount = 1;
		setLayoutBindings[1].binding = 1;
		setLayoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		setLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		setLayoutBindings[1].descriptorCount = 1;
		setLayoutBindings[2].binding = 2;
		setLayoutBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		setLayoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		setLayoutBindings[2].descriptorCount = 1;
		setLayoutBindings[3].binding = 3;
		setLayoutBindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		setLayoutBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		setLayoutBindings[3].descriptorCount = 1;
		setLayoutBindings[4].binding = 4;
		setLayoutBindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		setLayoutBindings[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		setLayoutBindings[4].descriptorCount = 1;
		setLayoutBindings[5].binding = 5;
		setLayoutBindings[5].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		setLayoutBindings[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		setLayoutBindings[5].descriptorCount = 1;
		setLayoutBindings[6].binding = 6;
		setLayoutBindings[6].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		setLayoutBindings[6].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		setLayoutBindings[6].descriptorCount = 1;

		VkPushConstantRange pushConstant;
		pushConstant.offset = 0;
		pushConstant.size = 16;
		pushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VulkanAPI::CreateDescriptorPool(logicalDevice, poolSizes, &compositionSetPool);
		VulkanAPI::CreateDescriptorSetLayout(logicalDevice, setLayoutBindings, &compositionSetLayout);
		VulkanAPI::CreatePipelineLayout(logicalDevice, { pushConstant }, { compositionSetLayout }, &compositionPipelineLayout);
		VulkanAPI::AllocateDescriptorSet(logicalDevice, compositionSetPool, compositionSetLayout, &compositionSet);

		std::vector<VkWriteDescriptorSet> writeDescriptorSets;
		writeDescriptorSets.resize(7);
		writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[0].dstBinding = 0;
		writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		writeDescriptorSets[0].dstSet = compositionSet;
		writeDescriptorSets[0].pImageInfo = &position.texInfo.descriptor;
		writeDescriptorSets[0].descriptorCount = 1;
		writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[1].dstBinding = 1;
		writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		writeDescriptorSets[1].dstSet = compositionSet;
		writeDescriptorSets[1].pImageInfo = &normal.texInfo.descriptor;
		writeDescriptorSets[1].descriptorCount = 1;
		writeDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[2].dstBinding = 2;
		writeDescriptorSets[2].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		writeDescriptorSets[2].dstSet = compositionSet;
		writeDescriptorSets[2].pImageInfo = &albedo.texInfo.descriptor;
		writeDescriptorSets[2].descriptorCount = 1;
		writeDescriptorSets[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[3].dstBinding = 3;
		writeDescriptorSets[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescriptorSets[3].dstSet = compositionSet;
		writeDescriptorSets[3].pImageInfo = &shadowMap.texInfo.descriptor;
		writeDescriptorSets[3].descriptorCount = 1;
		writeDescriptorSets[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[4].dstBinding = 4;
		writeDescriptorSets[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescriptorSets[4].dstSet = compositionSet;
		writeDescriptorSets[4].pBufferInfo = &shadowBuffer.BufferInfo;
		writeDescriptorSets[4].descriptorCount = 1;
		writeDescriptorSets[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[5].dstBinding = 5;
		writeDescriptorSets[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescriptorSets[5].dstSet = compositionSet;
		writeDescriptorSets[5].pBufferInfo = &cascadeBuffer.BufferInfo;
		writeDescriptorSets[5].descriptorCount = 1;
		writeDescriptorSets[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[6].dstBinding = 6;
		writeDescriptorSets[6].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescriptorSets[6].dstSet = compositionSet;
		writeDescriptorSets[6].pBufferInfo = &viewProjUBO.BufferInfo;
		writeDescriptorSets[6].descriptorCount = 1;

		vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;
		VkPipelineRasterizationStateCreateInfo rasterizationState{};
		rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationState.depthClampEnable = VK_FALSE;
		rasterizationState.rasterizerDiscardEnable = VK_FALSE;
		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationState.lineWidth = 1.0f;
		rasterizationState.cullMode = VK_CULL_MODE_NONE;
		rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationState.depthBiasEnable = VK_FALSE;
		rasterizationState.depthBiasConstantFactor = 0.0f;
		rasterizationState.depthBiasClamp = 0.0f;
		rasterizationState.depthBiasSlopeFactor = 0.0f;
		VkPipelineColorBlendAttachmentState blendAttachmentState{};
		blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendAttachmentState.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlendState{};
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &blendAttachmentState;

		VkPipelineDepthStencilStateCreateInfo depthStencilState{};
		depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilState.pNext = nullptr;
		depthStencilState.depthTestEnable = VK_TRUE;
		depthStencilState.depthWriteEnable = VK_TRUE;
		depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencilState.depthBoundsTestEnable = VK_FALSE;
		depthStencilState.minDepthBounds = 0.0f; // Optional
		depthStencilState.maxDepthBounds = 1.0f; // Optional
		depthStencilState.stencilTestEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = nullptr;
		viewportState.scissorCount = 1;
		viewportState.pScissors = nullptr;

		VkPipelineMultisampleStateCreateInfo multisampleState{};
		multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleState.sampleShadingEnable = VK_TRUE;
		multisampleState.minSampleShading = 0;

		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.pNext = nullptr;
		dynamicState.flags = 0;
		dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
		dynamicState.pDynamicStates = dynamicStates.data();

		std::string solution_path = GrEngine::Globals::getExecutablePath();
		std::vector<char> vertShaderCode = GrEngine::Globals::readFile(solution_path + "Shaders//composition_vert.spv");
		std::vector<char> fragShaderCode = GrEngine::Globals::readFile(solution_path + "Shaders//composition_frag.spv");
		VkShaderModule shaders[2] = { VulkanAPI::m_createShaderModule(logicalDevice, vertShaderCode) , VulkanAPI::m_createShaderModule(logicalDevice, fragShaderCode) };

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = shaders[0];
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = shaders[1];
		fragShaderStageInfo.pName = "main";


		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = { vertShaderStageInfo , fragShaderStageInfo };

		struct SpecializationData {
			float near_p;
			float far_p;
			uint32_t lights_count;
			int cascade_count;
		} specs;
		specs.near_p = NearPlane;
		specs.far_p = FarPlane;
		specs.lights_count = (lights.size() + cascade_count * (SHADOW_MAP_CASCADE_COUNT - 1) + omni_count * 5) * use_dynamic_lighting;
		specs.cascade_count = SHADOW_MAP_CASCADE_COUNT;
		std::array<VkSpecializationMapEntry, 4> entries;
		entries[0].constantID = 0;
		entries[0].offset = 0;
		entries[0].size = sizeof(float);
		entries[1].constantID = 1;
		entries[1].offset = sizeof(float);
		entries[1].size = sizeof(float);
		entries[2].constantID = 2;
		entries[2].offset = sizeof(float) * 2;
		entries[2].size = sizeof(uint32_t);
		entries[3].constantID = 3;
		entries[3].offset = sizeof(float) * 2 + sizeof(uint32_t);
		entries[3].size = sizeof(int);

		VkSpecializationInfo specializationInfo;
		specializationInfo.mapEntryCount = entries.size();
		specializationInfo.pMapEntries = entries.data();
		specializationInfo.dataSize = sizeof(SpecializationData);
		specializationInfo.pData = &specs;

		shaderStages[1].pSpecializationInfo = &specializationInfo;

		VkGraphicsPipelineCreateInfo pipelineCI{};

		VkPipelineVertexInputStateCreateInfo emptyInputState{};
		emptyInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCI.layout = compositionPipelineLayout;
		pipelineCI.renderPass = renderPass;
		pipelineCI.basePipelineIndex = 0;
		pipelineCI.pVertexInputState = &emptyInputState;
		pipelineCI.pInputAssemblyState = &inputAssembly;
		pipelineCI.pRasterizationState = &rasterizationState;
		pipelineCI.pColorBlendState = &colorBlendState;
		pipelineCI.pMultisampleState = &multisampleState;
		pipelineCI.pViewportState = &viewportState;
		pipelineCI.pDepthStencilState = &depthStencilState;
		pipelineCI.pDynamicState = &dynamicState;
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();
		pipelineCI.subpass = 1;
		depthStencilState.depthWriteEnable = VK_FALSE;

		VulkanAPI::CreateGraphicsPipeline(logicalDevice, &pipelineCI, &compositionPipeline);

		vkDestroyShaderModule(logicalDevice, shaders[0], nullptr);
		vkDestroyShaderModule(logicalDevice, shaders[1], nullptr);
	}

	void VulkanRenderer::prepareTransparencyPass()
	{
		VkCommandBuffer cmd;

		const int node_count = 20;
		geometrySBO.count = 0;
		geometrySBO.maxNodeCount = node_count * swapChainExtent.width * swapChainExtent.height;
		VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, &geometrySBO, sizeof(geometrySBO), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, &transBuffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, nullptr, sizeof(Node) * node_count* swapChainExtent.width* swapChainExtent.height, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &nodeBfffer);

		VkImageCreateInfo imageInfo = VulkanAPI::StructImageCreateInfo({ swapChainExtent.width,  swapChainExtent.height, 1 }, VK_FORMAT_R32_UINT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
		VulkanAPI::CreateImage(memAllocator, &imageInfo, &headIndex.newImage.allocatedImage, &headIndex.newImage.allocation);

		VkImageSubresourceRange viewRange{};
		viewRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewRange.baseMipLevel = 0;
		viewRange.levelCount = 1;
		viewRange.baseArrayLayer = 0;
		viewRange.layerCount = 1;
		VulkanAPI::CreateImageView(logicalDevice, VK_FORMAT_R32_UINT, headIndex.newImage.allocatedImage, viewRange, &headIndex.textureImageView);

		headIndex.srcInfo.channels = 1;
		headIndex.srcInfo.width = swapChainExtent.width;
		headIndex.srcInfo.height = swapChainExtent.height;
		headIndex.textureSampler = VK_NULL_HANDLE;
		headIndex.texInfo.descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		headIndex.texInfo.descriptor.imageView = headIndex.textureImageView;
		headIndex.texInfo.mipLevels = 1;
		headIndex.texInfo.format = VK_FORMAT_R32_UINT;
		headIndex.initialized = true;

		vkWaitForFences(logicalDevice, 1, &transitionFence, VK_TRUE, UINT64_MAX);
		VulkanAPI::AllocateCommandBuffers(logicalDevice, commandPool, &cmd, 1);
		VulkanAPI::BeginCommandBuffer(cmd, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		VulkanAPI::TransitionImageLayout(headIndex.newImage.allocatedImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VulkanAPI::StructSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT), cmd);
		VulkanAPI::EndAndSubmitCommandBuffer(logicalDevice, commandPool, cmd, graphicsQueue, transitionFence);

		std::vector<VkDescriptorPoolSize> poolSizes;
		poolSizes.resize(2);
		poolSizes[0].descriptorCount = 1;
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSizes[1].descriptorCount = 1;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;
		setLayoutBindings.resize(2);
		setLayoutBindings[0].binding = 0;
		setLayoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		setLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		setLayoutBindings[0].descriptorCount = 1;
		setLayoutBindings[1].binding = 1;
		setLayoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		setLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		setLayoutBindings[1].descriptorCount = 1;

		VulkanAPI::CreateDescriptorPool(logicalDevice, poolSizes, &transparencySetPool);
		VulkanAPI::CreateDescriptorSetLayout(logicalDevice, setLayoutBindings, &transparencySetLayout);
		VulkanAPI::CreatePipelineLayout(logicalDevice, {}, { transparencySetLayout }, &transparencyPipelineLayout);
		VulkanAPI::AllocateDescriptorSet(logicalDevice, transparencySetPool, transparencySetLayout, &transparencySet);

		VkDescriptorImageInfo texDescriptorHead{};
		texDescriptorHead.imageView = headIndex.textureImageView;
		texDescriptorHead.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		texDescriptorHead.sampler = VK_NULL_HANDLE;

		std::vector<VkWriteDescriptorSet> writeDescriptorSets;
		writeDescriptorSets.resize(2);
		writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[0].dstBinding = 0;
		writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		writeDescriptorSets[0].dstSet = transparencySet;
		writeDescriptorSets[0].pImageInfo = &texDescriptorHead;
		writeDescriptorSets[0].descriptorCount = 1;
		writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[1].dstBinding = 1;
		writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writeDescriptorSets[1].dstSet = transparencySet;
		writeDescriptorSets[1].pBufferInfo = &nodeBfffer.BufferInfo;
		writeDescriptorSets[1].descriptorCount = 1;

		vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;
		VkPipelineRasterizationStateCreateInfo rasterizationState{};
		rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationState.depthClampEnable = VK_FALSE;
		rasterizationState.rasterizerDiscardEnable = VK_FALSE;
		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationState.lineWidth = 1.0f;
		rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
		rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationState.depthBiasEnable = VK_FALSE;
		rasterizationState.depthBiasConstantFactor = 0.0f;
		rasterizationState.depthBiasClamp = 0.0f;
		rasterizationState.depthBiasSlopeFactor = 0.0f;
		VkPipelineColorBlendAttachmentState blendAttachmentState{};
		blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendAttachmentState.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlendState{};
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &blendAttachmentState;

		VkPipelineDepthStencilStateCreateInfo depthStencilState{};
		depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilState.pNext = nullptr;
		depthStencilState.depthTestEnable = VK_FALSE;
		depthStencilState.depthWriteEnable = VK_FALSE;
		depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencilState.depthBoundsTestEnable = VK_FALSE;
		depthStencilState.minDepthBounds = 0.0f; // Optional
		depthStencilState.maxDepthBounds = 1.0f; // Optional
		depthStencilState.stencilTestEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = nullptr;
		viewportState.scissorCount = 1;
		viewportState.pScissors = nullptr;

		VkPipelineMultisampleStateCreateInfo multisampleState{};
		multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleState.sampleShadingEnable = VK_TRUE;
		multisampleState.minSampleShading = 0;

		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.pNext = nullptr;
		dynamicState.flags = 0;
		dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
		dynamicState.pDynamicStates = dynamicStates.data();

		std::string solution_path = GrEngine::Globals::getExecutablePath();
		std::vector<char> vertShaderCode = GrEngine::Globals::readFile(solution_path + "Shaders//composition_vert_transparent.spv");
		std::vector<char> fragShaderCode = GrEngine::Globals::readFile(solution_path + "Shaders//composition_frag_transparent.spv");
		VkShaderModule shaders[2] = { VulkanAPI::m_createShaderModule(logicalDevice, vertShaderCode) , VulkanAPI::m_createShaderModule(logicalDevice, fragShaderCode) };

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = shaders[0];
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = shaders[1];
		fragShaderStageInfo.pName = "main";


		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = { vertShaderStageInfo , fragShaderStageInfo };

		std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;
		blendAttachmentStates.resize(1);
		blendAttachmentStates[0].blendEnable = VK_TRUE;
		blendAttachmentStates[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendAttachmentStates[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachmentStates[0].colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentStates[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachmentStates[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachmentStates[0].alphaBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
		colorBlendState.pAttachments = blendAttachmentStates.data();

		VkGraphicsPipelineCreateInfo pipelineCI{};

		VkPipelineVertexInputStateCreateInfo emptyInputState{};
		emptyInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCI.layout = transparencyPipelineLayout;
		pipelineCI.renderPass = renderPass;
		pipelineCI.basePipelineIndex = 0;
		pipelineCI.pVertexInputState = &emptyInputState;
		pipelineCI.pInputAssemblyState = &inputAssembly;
		pipelineCI.pRasterizationState = &rasterizationState;
		pipelineCI.pColorBlendState = &colorBlendState;
		pipelineCI.pMultisampleState = &multisampleState;
		pipelineCI.pViewportState = &viewportState;
		pipelineCI.pDepthStencilState = &depthStencilState;
		pipelineCI.pDynamicState = &dynamicState;
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();
		pipelineCI.subpass = 3;

		depthStencilState.depthWriteEnable = VK_FALSE;

		VulkanAPI::CreateGraphicsPipeline(logicalDevice, &pipelineCI, &transparencyPipeline);

		vkDestroyShaderModule(logicalDevice, shaders[0], nullptr);
		vkDestroyShaderModule(logicalDevice, shaders[1], nullptr);
	}

	void VulkanRenderer::prepareSamplingPass()
	{
		VulkanAPI::DestroyRenderPass(samplingPass);

		frameSize = { swapChainExtent.width, swapChainExtent.height };
		std::vector<VkDescriptorPoolSize> poolSizes;
		poolSizes.resize(2);
		poolSizes[0].descriptorCount = 1;
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = 1;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;
		setLayoutBindings.resize(2);
		setLayoutBindings[0].binding = 0;
		setLayoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		setLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		setLayoutBindings[0].descriptorCount = 1;
		setLayoutBindings[1].binding = 1;
		setLayoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		setLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		setLayoutBindings[1].descriptorCount = 1;

		VulkanAPI::CreateDescriptorPool(logicalDevice, poolSizes, &samplingSetPool);
		VulkanAPI::CreateDescriptorSetLayout(logicalDevice, setLayoutBindings, &samplingSetLayout);
		VulkanAPI::CreatePipelineLayout(logicalDevice, {}, { samplingSetLayout }, &samplingPipelineLayout);
		VulkanAPI::AllocateDescriptorSet(logicalDevice, samplingSetPool, samplingSetLayout, &samplingSet);
		VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, &frameSize, sizeof(glm::vec2), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &frameInfo);

		VkDescriptorImageInfo imgInfo{};
		imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imgInfo.imageView = colorImage.textureImageView;
		imgInfo.sampler = colorImage.textureSampler;
		std::vector<VkWriteDescriptorSet> writeDescriptorSets;
		writeDescriptorSets.resize(2);
		writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[0].dstBinding = 0;
		writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescriptorSets[0].dstSet = samplingSet;
		writeDescriptorSets[0].pImageInfo = &imgInfo;
		writeDescriptorSets[0].descriptorCount = 1;
		writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[1].dstBinding = 1;
		writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writeDescriptorSets[1].dstSet = samplingSet;
		writeDescriptorSets[1].pBufferInfo = &frameInfo.BufferInfo;
		writeDescriptorSets[1].descriptorCount = 1;

		vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);

		std::array<VkAttachmentDescription, 2> attchmentDescriptions = {};
		// Color attachment
		attchmentDescriptions[0].format = swapChainImageFormat;
		attchmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attchmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attchmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attchmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attchmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attchmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attchmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		// Depth attachment
		attchmentDescriptions[1].format = depthFormat;
		attchmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attchmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attchmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attchmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attchmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attchmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attchmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		VkAttachmentReference depthReference = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

		VkSubpassDescription subpassDescription = {};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &colorReference;
		subpassDescription.pDepthStencilAttachment = &depthReference;

		std::array<VkSubpassDependency, 2> dependencies;

		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attchmentDescriptions.size());
		renderPassInfo.pAttachments = attchmentDescriptions.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpassDescription;
		renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
		renderPassInfo.pDependencies = dependencies.data();

		VulkanAPI::CreateRenderPass(logicalDevice, &renderPassInfo, &samplingPass);

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;
		VkPipelineRasterizationStateCreateInfo rasterizationState{};
		rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationState.depthClampEnable = VK_FALSE;
		rasterizationState.rasterizerDiscardEnable = VK_FALSE;
		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationState.lineWidth = 1.0f;
		rasterizationState.cullMode = VK_CULL_MODE_NONE;
		rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationState.depthBiasEnable = VK_FALSE;
		rasterizationState.depthBiasConstantFactor = 0.0f;
		rasterizationState.depthBiasClamp = 0.0f;
		rasterizationState.depthBiasSlopeFactor = 0.0f;
		VkPipelineColorBlendAttachmentState blendAttachmentState{};
		blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendAttachmentState.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlendState{};
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &blendAttachmentState;

		VkPipelineDepthStencilStateCreateInfo depthStencilState{};
		depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilState.pNext = nullptr;
		depthStencilState.depthTestEnable = VK_FALSE;
		depthStencilState.depthWriteEnable = VK_FALSE;
		depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencilState.depthBoundsTestEnable = VK_FALSE;
		depthStencilState.minDepthBounds = 0.0f; // Optional
		depthStencilState.maxDepthBounds = 1.0f; // Optional
		depthStencilState.stencilTestEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = nullptr;
		viewportState.scissorCount = 1;
		viewportState.pScissors = nullptr;

		VkPipelineMultisampleStateCreateInfo multisampleState{};
		multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleState.sampleShadingEnable = VK_TRUE;
		multisampleState.minSampleShading = 0;

		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.pNext = nullptr;
		dynamicState.flags = 0;
		dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
		dynamicState.pDynamicStates = dynamicStates.data();

		std::string solution_path = GrEngine::Globals::getExecutablePath();
		std::vector<char> vertShaderCode = GrEngine::Globals::readFile(solution_path + "Shaders//fxaa_vert.spv");
		std::vector<char> fragShaderCode = GrEngine::Globals::readFile(solution_path + "Shaders//fxaa_frag.spv");
		VkShaderModule shaders[2] = { VulkanAPI::m_createShaderModule(logicalDevice, vertShaderCode) , VulkanAPI::m_createShaderModule(logicalDevice, fragShaderCode) };

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = shaders[0];
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = shaders[1];
		fragShaderStageInfo.pName = "main";


		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = { vertShaderStageInfo , fragShaderStageInfo };

		VkGraphicsPipelineCreateInfo pipelineCI{};

		VkPipelineVertexInputStateCreateInfo emptyInputState{};
		emptyInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCI.layout = samplingPipelineLayout;
		pipelineCI.renderPass = samplingPass;
		pipelineCI.basePipelineIndex = 0;
		pipelineCI.pVertexInputState = &emptyInputState;
		pipelineCI.pInputAssemblyState = &inputAssembly;
		pipelineCI.pRasterizationState = &rasterizationState;
		pipelineCI.pColorBlendState = &colorBlendState;
		pipelineCI.pMultisampleState = &multisampleState;
		pipelineCI.pViewportState = &viewportState;
		pipelineCI.pDepthStencilState = &depthStencilState;
		pipelineCI.pDynamicState = &dynamicState;
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();
		pipelineCI.subpass = 0;
		depthStencilState.depthWriteEnable = VK_FALSE;

		VulkanAPI::CreateGraphicsPipeline(logicalDevice, &pipelineCI, &samplingPipeline);

		vkDestroyShaderModule(logicalDevice, shaders[0], nullptr);
		vkDestroyShaderModule(logicalDevice, shaders[1], nullptr);
	}

	void VulkanRenderer::prepareSelectionPass()
	{
		VulkanAPI::DestroyRenderPass(selectionPass);
		VulkanAPI::DestroyFramebuffer(selectionFramebuffer);

		std::array<VkAttachmentDescription, 2> attchmentDescriptions = {};
		// Color attachment
		attchmentDescriptions[0].format = VK_FORMAT_R32_UINT;
		attchmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attchmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attchmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attchmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attchmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attchmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attchmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		// Depth attachment
		attchmentDescriptions[1].format = depthFormat;
		attchmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attchmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attchmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attchmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attchmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attchmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attchmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		VkAttachmentReference depthReference = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

		VkSubpassDescription subpassDescription = {};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &colorReference;
		subpassDescription.pDepthStencilAttachment = &depthReference;

		std::array<VkSubpassDependency, 2> dependencies;

		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attchmentDescriptions.size());
		renderPassInfo.pAttachments = attchmentDescriptions.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpassDescription;
		renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
		renderPassInfo.pDependencies = dependencies.data();

		VulkanAPI::CreateRenderPass(logicalDevice, &renderPassInfo, &selectionPass);

		createAttachment(VK_FORMAT_R32_UINT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, &identity);

		VkImageView clrattachments[] = { identity.textureImageView, depthImageView };
		VulkanAPI::CreateFrameBuffer(logicalDevice, selectionPass, clrattachments, 2, swapChainExtent, &selectionFramebuffer);
	}

	void VulkanRenderer::prepareShadowPass()
	{
		VulkanAPI::DestroyRenderPass(shadowPass);
		VulkanAPI::DestroyFramebuffer(shadowFramebuffer);
		VulkanAPI::DestroySampler(shadowMap.textureSampler);
		VulkanAPI::DestroyImageView(shadowMap.textureImageView);
		VulkanAPI::DestroyImage(shadowMap.newImage.allocatedImage);

		VkImageAspectFlags aspectMask = 0;
		VkImageLayout imageLayout;

		aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

		VkImageCreateInfo imageInfo = VulkanAPI::StructImageCreateInfo({ SHADOW_MAP_DIM, SHADOW_MAP_DIM, 1 }, depthFormat, msaaSamples, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		//imageInfo.arrayLayers = glm::max((int)lights.size(), 1);
		imageInfo.arrayLayers = lightsCount();
		VulkanAPI::CreateImage(memAllocator, &imageInfo, &shadowMap.newImage.allocatedImage, &shadowMap.newImage.allocation);

		VkImageSubresourceRange imageView{};
		imageView.aspectMask = aspectMask;
		imageView.baseMipLevel = 0;
		imageView.levelCount = 1;
		imageView.baseArrayLayer = 0;
		//imageView.layerCount = glm::max((int)lights.size(), 1);
		imageView.layerCount = lightsCount();
		VulkanAPI::CreateImageView(logicalDevice, depthFormat, shadowMap.newImage.allocatedImage, imageView, &shadowMap.textureImageView, VK_IMAGE_VIEW_TYPE_2D_ARRAY);

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = msaaSamples;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 1.f;
		VulkanAPI::CreateSampler(logicalDevice, &samplerInfo, &shadowMap.textureSampler);

		shadowMap.initialized = true;
		shadowMap.texInfo.mipLevels = 1;
		shadowMap.texInfo.format = depthFormat;
		shadowMap.texInfo.descriptor.imageLayout = imageLayout;
		shadowMap.texInfo.descriptor.imageView = shadowMap.textureImageView;
		shadowMap.texInfo.descriptor.sampler = shadowMap.textureSampler;

		////VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, nullptr, sizeof(VulkanSpotlight::ShadowProjection) * glm::max((int)lights.size(), 1), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &shadowBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, nullptr, sizeof(VulkanSpotlight::ShadowProjection) * lightsCount(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, &shadowBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		//vmaMapMemory(memAllocator, shadowBuffer.Allocation, (void**)&shadowBuffer.data);
		VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, nullptr, 16 * SHADOW_MAP_CASCADE_COUNT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, &cascadeBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		//vmaMapMemory(memAllocator, cascadeBuffer.Allocation, (void**)&cascadeBuffer.data);

		std::array<VkAttachmentDescription, 1> attchmentDescriptions = {};
		attchmentDescriptions[0].format = depthFormat;
		attchmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attchmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attchmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attchmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attchmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attchmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attchmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

		VkAttachmentReference depthReference = { 0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

		VkSubpassDescription subpassDescription = {};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 0;
		subpassDescription.pColorAttachments = nullptr;
		subpassDescription.pDepthStencilAttachment = &depthReference;

		std::array<VkSubpassDependency, 2> dependencies;

		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attchmentDescriptions.size());
		renderPassInfo.pAttachments = attchmentDescriptions.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpassDescription;
		renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
		renderPassInfo.pDependencies = dependencies.data();

		VulkanAPI::CreateRenderPass(logicalDevice, &renderPassInfo, &shadowPass);

		VkImageView attachments[] = { shadowMap.textureImageView };

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = shadowPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = SHADOW_MAP_DIM;
		framebufferInfo.height = SHADOW_MAP_DIM;
		framebufferInfo.layers = lightsCount();
		VulkanAPI::CreateFrameBuffer(logicalDevice, &framebufferInfo, &shadowFramebuffer);
	}

	bool VulkanRenderer::updateDrawables(uint32_t index, DrawMode mode, VkExtent2D extent)
	{
		VkViewport viewport{};
		VkRect2D scissor{};
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		//vkWaitForFences(logicalDevice, 1, &renderFence[index], TRUE, UINT64_MAX);

		if (vkBeginCommandBuffer(commandBuffers[index], &beginInfo) != VK_SUCCESS)
			return false;

		vpUBO.pos = getActiveViewport()->UpdateCameraPosition(0.65);
		vpUBO.view = glm::translate(glm::mat4_cast(getActiveViewport()->UpdateCameraOrientation(0.2)), -vpUBO.pos);
		vpUBO.proj = glm::perspective(glm::radians(60.0f), (float)extent.width / (float)extent.height, NearPlane, FarPlane);
		vpUBO.proj[1][1] *= -1;
		//memcpy_s(viewProjUBO.data, sizeof(ViewProjection), &vpUBO, sizeof(ViewProjection));
		vkCmdUpdateBuffer(commandBuffers[index], viewProjUBO.Buffer, 0, sizeof(ViewProjection), &vpUBO);

		std::vector<VulkanSpotlight::ShadowProjection> projections;
		std::vector<VulkanSpotlight::ShadowProjection> points;
		if (lights.size() > 0)
		{
			
			for (std::map<UINT, GrEngine::LightObject*>::iterator light = lights.begin(); light != lights.end(); ++light)
			{
				LightType clt = ((*light).second)->GetLightType();
				if (clt == LightType::Spot)
				{
					VulkanSpotlight::ShadowProjection prj = static_cast<VulkanSpotlight*>((*light).second)->getLightUBO();
					vkCmdUpdateBuffer(commandBuffers[index], shadowBuffer.Buffer, sizeof(VulkanSpotlight::ShadowProjection) * projections.size(), sizeof(VulkanSpotlight::ShadowProjection), &prj);
					projections.push_back(prj);
				}
				else if (clt == LightType::Cascade)
				{
					auto arr = static_cast<VulkanCascade*>((*light).second)->getCascadeUBO();
					for (int i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
					{
						VulkanSpotlight::ShadowProjection prj{};
						prj.model = arr[i].model;
						prj.viewproj = arr[i].proj * arr[i].view;
						prj.spec.x = LightType::Cascade;
						prj.spec.y = i;
						prj.spec.z = 0.5;
						prj.color = arr[i].color;
						//void* p = (byte*)cascadeBuffer.data + 16 * i;
						//memcpy_s(p, sizeof(float), &arr[i].splitDepth, sizeof(float));
						vkCmdUpdateBuffer(commandBuffers[index], shadowBuffer.Buffer, sizeof(VulkanSpotlight::ShadowProjection) * projections.size(), sizeof(VulkanSpotlight::ShadowProjection), &prj);
						vkCmdUpdateBuffer(commandBuffers[index], cascadeBuffer.Buffer, 16 * i, sizeof(float), &arr[i].splitDepth);
						projections.push_back(prj);
					}
				}
				else if (clt == LightType::Omni)
				{
					auto arr = static_cast<VulkanOmniLight*>((*light).second)->getLightUBO();
					for (int i = 0; i < 6; i++)
					{
						VulkanSpotlight::ShadowProjection prj{};
						prj.model = arr[i].model;
						prj.viewproj = arr[i].viewproj;
						prj.spec = arr[i].spec;
						prj.color = arr[i].color;
						//void* p = (byte*)cascadeBuffer.data + 16 * i;
						//memcpy_s(p, sizeof(float), &arr[i].splitDepth, sizeof(float));
						vkCmdUpdateBuffer(commandBuffers[index], shadowBuffer.Buffer, sizeof(VulkanSpotlight::ShadowProjection) * projections.size(), sizeof(VulkanSpotlight::ShadowProjection), &prj);
						projections.push_back(prj);
					}
				}
				else if (clt == LightType::Point)
				{
					auto arr = static_cast<VulkanPointLight*>((*light).second)->getLightUBO();
					VulkanSpotlight::ShadowProjection prj{};
					prj.color = arr.color;
					prj.model = arr.model;
					prj.viewproj = glm::mat4(1.f);
					prj.spec = arr.spec;
					points.push_back(prj);
				}
			}

			//memcpy_s(shadowBuffer.data, sizeof(VulkanSpotlight::ShadowProjection) * projections.size(), projections.data(), sizeof(VulkanSpotlight::ShadowProjection) * projections.size());
		}

		VkClearValue shadowClear[1];
		shadowClear->depthStencil = { 1.f, 0 };
		VkRenderPassBeginInfo shadowPassInfo{};
		shadowPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		shadowPassInfo.framebuffer = shadowFramebuffer;
		shadowPassInfo.renderPass = shadowPass;
		shadowPassInfo.renderArea.offset = { 0, 0 };
		shadowPassInfo.renderArea.extent = { SHADOW_MAP_DIM, SHADOW_MAP_DIM };
		shadowPassInfo.clearValueCount = 1;
		shadowPassInfo.pClearValues = shadowClear;
		vkCmdBeginRenderPass(commandBuffers[index], &shadowPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		viewport.x = 0;
		viewport.y = 0;
		viewport.width = (float)SHADOW_MAP_DIM;
		viewport.height = (float)SHADOW_MAP_DIM;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffers[index], 0, 1, &viewport);
		scissor.offset = { 0, 0 };
		scissor.extent = { SHADOW_MAP_DIM, SHADOW_MAP_DIM };
		vkCmdSetScissor(commandBuffers[index], 0, 1, &scissor);

		if (lights.size() > 0 && use_dynamic_lighting)
		{
			for (std::map<UINT, GrEngine::Object*>::iterator itt = drawables.begin(); itt != drawables.end(); ++itt)
			{
				if (static_cast<VulkanObject*>((*itt).second)->GetOwnerEntity()->GetPropertyValue(PropertyType::CastShadow, 1)
					&& static_cast<VulkanObject*>((*itt).second)->IsVisible()
					&& !static_cast<VulkanObject*>((*itt).second)->GetOwnerEntity()->GetPropertyValue(PropertyType::Transparency, 0))
				{
					float dist = (*itt).second->GetOwnerEntity()->GetPropertyValue(PropertyType::MaximumDistance, FarPlane);
					if ((dist == -1.f || glm::distance(viewport_camera->GetObjectPosition(), (*itt).second->GetObjectPosition()) < dist))
					{
						static_cast<VulkanObject*>((*itt).second)->recordShadowPass(commandBuffers[index], projections.size());
					}
				}
			}
		}

		vkCmdEndRenderPass(commandBuffers[index]);

		if (points.size() > 0)
		{
			vkCmdUpdateBuffer(commandBuffers[index], shadowBuffer.Buffer, sizeof(VulkanSpotlight::ShadowProjection) * projections.size(), sizeof(VulkanSpotlight::ShadowProjection) * points.size(), points.data());
		}

		VkClearValue clearValues[6];
		VkRenderPassBeginInfo renderPassInfo{};
		clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[3].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[4].depthStencil = { 1.0f, 0 };
		clearValues[5].color = { 0.0f };

		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.framebuffer = defferFramebuffer;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = extent;
		renderPassInfo.clearValueCount = 6;
		renderPassInfo.pClearValues = clearValues;

		VkClearColorValue clearColor;
		clearColor.uint32[0] = 0xffffffff;

		VkImageSubresourceRange subresRange = {};
		subresRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresRange.levelCount = 1;
		subresRange.layerCount = 1;
		vkCmdClearColorImage(commandBuffers[index], headIndex.newImage.allocatedImage, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &subresRange);

		vkCmdFillBuffer(commandBuffers[index], transBuffer.Buffer, 0, sizeof(uint32_t), 0);

		vkCmdBeginRenderPass(commandBuffers[index], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		viewport.x = 0;
		viewport.y = 0;
		viewport.width = (float)extent.width;
		viewport.height = (float)extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffers[index], 0, 1, &viewport);
		scissor.offset = { 0, 0 };
		scissor.extent = extent;
		vkCmdSetScissor(commandBuffers[index], 0, 1, &scissor);

		for (std::map<UINT, GrEngine::Entity*>::iterator itt = entities.begin(); itt != entities.end(); ++itt)
		{
			if ((*itt).second->GetEntityType() == GrEngine::EntityType::ObjectEntity)
			{
				GrEngine::Object* obj = drawables[(*itt).second->GetEntityID()];
				float dist = (*itt).second->GetPropertyValue(PropertyType::MaximumDistance, FarPlane);
				if (obj->IsVisible() && (dist == -1.f || glm::distance(viewport_camera->GetObjectPosition(), (*itt).second->GetObjectPosition()) < dist))
				{
					static_cast<VulkanObject*>(obj)->recordCommandBuffer(commandBuffers[index], DrawMode::NORMAL);
				}
			}
			else if ((*itt).second->GetEntityType() == GrEngine::EntityType::SkyboxEntity)
			{
				static_cast<VulkanSkybox*>((*itt).second)->recordCommandBuffer(commandBuffers[index], DrawMode::NORMAL);
			}
			else if ((*itt).second->GetEntityType() == GrEngine::EntityType::TerrainEntity)
			{
				static_cast<VulkanTerrain*>((*itt).second)->recordCommandBuffer(commandBuffers[index], DrawMode::NORMAL);
			}
		}

		vkCmdNextSubpass(commandBuffers[index], VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, compositionPipeline);
		vkCmdPushConstants(commandBuffers[index], compositionPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 16, &ambient);
		vkCmdBindDescriptorSets(commandBuffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, compositionPipelineLayout, 0, 1, &compositionSet, 0, NULL);
		vkCmdDraw(commandBuffers[index], 3, 1, 0, 0);

		vkCmdNextSubpass(commandBuffers[index], VK_SUBPASS_CONTENTS_INLINE);

		for (std::map<UINT, GrEngine::Object*>::iterator itt = drawables.begin(); itt != drawables.end(); ++itt)
		{
			if (static_cast<VulkanObject*>((*itt).second)->IsVisible())
			{
				static_cast<VulkanObject*>((*itt).second)->recordCommandBuffer(commandBuffers[index], DrawMode::TRANSPARENCY);
			}
		}

		vkCmdNextSubpass(commandBuffers[index], VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, transparencyPipeline);
		vkCmdBindDescriptorSets(commandBuffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, transparencyPipelineLayout, 0, 1, &transparencySet, 0, nullptr);
		vkCmdDraw(commandBuffers[index], 3, 1, 0, 0);

		vkCmdEndRenderPass(commandBuffers[index]);

		VkMemoryBarrier memoryBarrier{};
		memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		vkCmdPipelineBarrier(commandBuffers[index], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 1, &memoryBarrier, 0, NULL, 0, NULL);

		VkClearValue samplingClears[2];
		samplingClears[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		samplingClears[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		samplingClears[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.renderPass = samplingPass;
		renderPassInfo.framebuffer = swapChainFramebuffers[index];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = extent;
		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = samplingClears;

		vkCmdBeginRenderPass(commandBuffers[index], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, samplingPipeline);
		vkCmdBindDescriptorSets(commandBuffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, samplingPipelineLayout, 0, 1, &samplingSet, 0, nullptr);
		vkCmdDraw(commandBuffers[index], 3, 1, 0, 0);

		vkCmdEndRenderPass(commandBuffers[index]);

		return vkEndCommandBuffer(commandBuffers[index]) == VK_SUCCESS;
	}

	bool VulkanRenderer::createVKInstance()
	{
#ifndef VALIDATION
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Gray Engine App";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Gray Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_2;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledLayerCount = 0;

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;

		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		createInfo.enabledExtensionCount = glfwExtensionCount;
		createInfo.ppEnabledExtensionNames = glfwExtensions;
		createInfo.enabledLayerCount = 0;
#else
		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("validation layers requested, but not available!");
		}

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Gray Engine App";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Gray Enginee";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_2;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto extensions = getRequiredExtensions();
		
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else {
			createInfo.enabledLayerCount = 0;

			createInfo.pNext = nullptr;
		}
#endif // DEBUG

		return vkCreateInstance(&createInfo, nullptr, &_vulkan) == VK_SUCCESS;
	}

	bool VulkanRenderer::isDeviceSuitable(const VkPhysicalDevice device)
	{
		float queuePriority = 1.f;
		std::vector<VkDeviceQueueCreateInfo> deviceQueues = VulkanAPI::StructQueueCreateInfo(device, surface, { VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_COMPUTE_BIT }, &queuePriority);
		deviceQueues.insert(deviceQueues.begin(), deviceQueues[0]);

		bool extensionsSupported = VulkanAPI::CheckDeviceExtensionSupport(device, deviceExtensions);

		bool swapChainAdequate = false;
		if (extensionsSupported) {
			SwapChainSupportDetails swapChainSupport = VulkanAPI::QuerySwapChainSupport(surface, device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		bool is_complete = true;
		for (VkDeviceQueueCreateInfo info : deviceQueues)
		{
			if (info.queueFamilyIndex < 0)
			{
				is_complete = false;
			}
		}

		return is_complete && extensionsSupported && swapChainAdequate;
	}

	bool VulkanRenderer::createSwapChainImages()
	{
		bool res = true;

		SwapChainSupportDetails swapChainSupport = VulkanAPI::QuerySwapChainSupport(surface, physicalDevice);
		VkSurfaceFormatKHR surfaceFormat = VulkanAPI::ChooseSwapSurfaceFormat(swapChainSupport.formats);
		VkExtent2D extent = VulkanAPI::ChooseSwapExtent(pParentWindow, swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, swapChainImages.data());
		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;

		depthFormat = VK_FORMAT_D16_UNORM;
		//depthFormat = VK_FORMAT_D32_SFLOAT;
		VkImageCreateInfo depthImageCreateInfo = VulkanAPI::StructImageCreateInfo({ swapChainExtent.width, swapChainExtent.height, 1 }, depthFormat, msaaSamples, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
		res = VulkanAPI::CreateImage(memAllocator, &depthImageCreateInfo, &depthImage.allocatedImage, &depthImage.allocation) & res;
		res = VulkanAPI::CreateImageView(logicalDevice, depthFormat, depthImage.allocatedImage, VulkanAPI::StructSubresourceRange(VK_IMAGE_ASPECT_DEPTH_BIT), &depthImageView) & res;

		VkImageCreateInfo samplingImageInfo = VulkanAPI::StructImageCreateInfo({ swapChainExtent.width, swapChainExtent.height, 1 }, swapChainImageFormat, msaaSamples, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		res = VulkanAPI::CreateImage(memAllocator, &samplingImageInfo, &colorImage.newImage.allocatedImage, &colorImage.newImage.allocation) & res;
		res = VulkanAPI::CreateImageView(logicalDevice, swapChainImageFormat, colorImage.newImage.allocatedImage, VulkanAPI::StructSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT), &colorImage.textureImageView) & res;
		res = VulkanAPI::CreateSampler(physicalDevice, logicalDevice, &colorImage.textureSampler) & res;

		swapChainImageViews.resize(swapChainImages.size());
		for (std::size_t i = 0; i < swapChainImages.size(); i++) {
			VkImageSubresourceRange subresourceRange = VulkanAPI::StructSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
			if ((res = VulkanAPI::CreateImageView(logicalDevice, swapChainImageFormat, swapChainImages[i], subresourceRange, &swapChainImageViews[i]) & res) == false)
				Logger::Out("[Vk] Failed to create image views", OutputColor::Red, OutputType::Error);
		}

		return res;
	}

	void VulkanRenderer::SetUseDynamicLighting(bool state)
	{
		use_dynamic_lighting = state;
		updateShadowResources();
	}

	void VulkanRenderer::Update()
	{
		recreateSwapChain();
	}

	void VulkanRenderer::VSyncState(bool state)
	{
		vsync = state;
		recreateSwapChain();
	}

	void VulkanRenderer::recreateSwapChain()
	{
		waitForRenderer();

		//Initialized = false; //Block rendering for a time it takes to recreate swapchain
		cleanupSwapChain();

		VkPresentModeKHR presMode = vsync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_MAILBOX_KHR;
		VulkanAPI::CreateVkSwapchain(physicalDevice, logicalDevice, pParentWindow, surface, &swapChain, presMode);
		createSwapChainImages();

		swapChainImageViews.resize(swapChainImages.size());

		for (std::size_t i = 0; i < swapChainImages.size(); i++)
		{
			VkImageSubresourceRange subresourceRange = VulkanAPI::StructSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
			VulkanAPI::CreateImageView(logicalDevice, swapChainImageFormat, swapChainImages[i], subresourceRange, &swapChainImageViews[i]);
		}

		createAttachment(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, &position);
		createAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, &normal);
		createAttachment(swapChainImageFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, &albedo);

		VulkanAPI::CreateRenderPass(logicalDevice, swapChainImageFormat, depthFormat, msaaSamples, &renderPass);
		prepareCompositionPass();
		prepareTransparencyPass();
		prepareSamplingPass();
		prepareSelectionPass();
		//prepareShadowPass();

		swapChainFramebuffers.resize(swapChainImageViews.size());
		for (std::size_t i = 0; i < swapChainImageViews.size(); i++) {
			VkImageView attachments[] = { swapChainImageViews[i], depthImageView, };

			VulkanAPI::CreateFrameBuffer(logicalDevice, samplingPass, attachments, 2, swapChainExtent, &swapChainFramebuffers[i]);
		}

		VkImageView attachments[] = { colorImage.textureImageView, position.textureImageView, normal.textureImageView, albedo.textureImageView, depthImageView };
		VulkanAPI::CreateFrameBuffer(logicalDevice, renderPass, attachments, 5, swapChainExtent, &defferFramebuffer);

		commandBuffers.resize(swapChainFramebuffers.size());
		VulkanAPI::AllocateCommandBuffers(logicalDevice, commandPool, commandBuffers.data(), commandBuffers.size());

		for (std::map<UINT, GrEngine::Entity*>::iterator itt = entities.begin(); itt != entities.end(); ++itt)
		{
			if (((*itt).second->GetEntityType() & GrEngine::EntityType::ObjectEntity) != 0)
			{
				static_cast<VulkanObject*>(drawables[(*itt).second->GetEntityID()])->updateObject();
			}
			else if (((*itt).second->GetEntityType() & GrEngine::EntityType::SkyboxEntity) != 0)
			{
				static_cast<VulkanSkybox*>((*itt).second)->updateObject();
			}
			else if (((*itt).second->GetEntityType() & GrEngine::EntityType::TerrainEntity) != 0)
			{
				static_cast<VulkanTerrain*>((*itt).second)->updateObject();
			}
		}
		vkDeviceWaitIdle(logicalDevice);

		currentFrame = 0;
		Initialized = swapChainExtent.height != 0;
	}

	void VulkanRenderer::updateShadowResources()
	{
		//Initialized = false; //Block rendering for a time it takes to recreate swapchain

		waitForRenderer();

		//vmaUnmapMemory(memAllocator, shadowBuffer.Allocation);
		//vmaUnmapMemory(memAllocator, cascadeBuffer.Allocation);

		VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &shadowBuffer);
		VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &cascadeBuffer);

		prepareShadowPass();
		recreateSwapChain();

		//Initialized = true;
	}

	void VulkanRenderer::cleanupSwapChain()
	{
		VulkanAPI::DestroyFramebuffer(defferFramebuffer);
		for (std::size_t i = 0; i < swapChainFramebuffers.size(); i++)
		{
			VulkanAPI::DestroyFramebuffer(swapChainFramebuffers[i]);
		}
		swapChainFramebuffers.resize(0);

		VulkanAPI::FreeDescriptorSet(compositionSet);
		VulkanAPI::DestroyDescriptorLayout(compositionSetLayout);
		VulkanAPI::DestroyDescriptorPool(compositionSetPool);
		VulkanAPI::DestroyPipeline(compositionPipeline);
		VulkanAPI::DestroyPipelineLayout(compositionPipelineLayout);

		VulkanAPI::FreeDescriptorSet(transparencySet);
		VulkanAPI::DestroyDescriptorLayout(transparencySetLayout);
		VulkanAPI::DestroyDescriptorPool(transparencySetPool);
		VulkanAPI::DestroyPipeline(transparencyPipeline);
		VulkanAPI::DestroyPipelineLayout(transparencyPipelineLayout);

		VulkanAPI::FreeDescriptorSet(samplingSet);
		VulkanAPI::DestroyDescriptorLayout(samplingSetLayout);
		VulkanAPI::DestroyDescriptorPool(samplingSetPool);
		VulkanAPI::DestroyPipeline(samplingPipeline);
		VulkanAPI::DestroyPipelineLayout(samplingPipelineLayout);
		VulkanAPI::DestroyImageView(headIndex.textureImageView);
		VulkanAPI::DestroyImage(headIndex.newImage.allocatedImage);
		VulkanAPI::DestroyImageView(colorImage.textureImageView);
		VulkanAPI::DestroyImage(colorImage.newImage.allocatedImage);

		if (frameInfo.initialized == true)
		{
			VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &frameInfo);
		}

		if (transBuffer.initialized == true)
		{
			VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &transBuffer);
		}

		if (nodeBfffer.initialized == true)
		{
			VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &nodeBfffer);
		}

		//if (shadowBuffer.initialized == true)
		//{
		//	VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &shadowBuffer);
		//}

		VulkanAPI::FreeCommandBuffers(commandBuffers.data(), commandBuffers.size());
		commandBuffers.resize(0);

		VulkanAPI::DestroyRenderPass(renderPass);

		for (std::size_t i = 0; i < swapChainImageViews.size(); i++)
		{
			VulkanAPI::DestroyImageView(swapChainImageViews[i]);
		}
		swapChainImageViews.resize(0);

		if (depthImageView != nullptr)
		{
			VulkanAPI::DestroyImageView(depthImageView);
			VulkanAPI::DestroyImage(depthImage.allocatedImage);
		}

		VulkanAPI::DestroySwapchainKHR(swapChain);

		depthImageView = nullptr;
		swapChain = nullptr;
	}

	void VulkanRenderer::clearDrawables()
	{
		waitForRenderer();

		int offset = 0;
		while (entities.size() != offset)
		{
			std::map<UINT, GrEngine::Entity*>::iterator pos = entities.begin();
			std::advance(pos, offset);

			if ((*pos).second->IsStatic() == false)
			{
				if ((*pos).second->GetEntityType() == GrEngine::EntityType::ObjectEntity)
				{
					static_cast<VulkanObject*>(drawables[(*pos).first])->destroyObject();
					delete drawables[(*pos).first];
					drawables.erase((*pos).first);
				}
				else if ((*pos).second->GetEntityType() == GrEngine::EntityType::TerrainEntity)
				{
					static_cast<VulkanTerrain*>((*pos).second)->destroyObject();
					terrain = nullptr;
				}
				else if ((*pos).second->GetEntityType() == GrEngine::EntityType::SkyboxEntity)
				{
					static_cast<VulkanSkybox*>((*pos).second)->destroyObject();
					sky = nullptr;
				}
				if ((*pos).second->GetEntityType() == GrEngine::EntityType::SpotlightEntity)
				{
					static_cast<VulkanSpotlight*>(lights[(*pos).first])->destroyLight();
					delete lights[(*pos).first];
					lights.erase((*pos).first);
				}
				else if ((*pos).second->GetEntityType() == GrEngine::EntityType::OmniLightEntity)
				{
					static_cast<VulkanOmniLight*>(lights[(*pos).first])->destroyLight();
					delete lights[(*pos).first];
					lights.erase((*pos).first);
				}
				else if ((*pos).second->GetEntityType() == GrEngine::EntityType::PointLightEntity)
				{
					static_cast<VulkanPointLight*>(lights[(*pos).first])->destroyLight();
					delete lights[(*pos).first];
					lights.erase((*pos).first);
				}
				else if ((*pos).second->GetEntityType() == GrEngine::EntityType::CascadeLightEntity)
				{
					static_cast<VulkanCascade*>(lights[(*pos).first])->destroyLight();
					delete lights[(*pos).first];
					lights.erase((*pos).first);
				}
				delete (*pos).second;
				entities.erase((*pos).first);
			}
			else
			{
				offset++;
			}
		}

		//recreateSwapChain();

		Logger::Out("The scene was cleared", OutputColor::Green, OutputType::Log);
	}

	bool VulkanRenderer::loadModel(UINT id, const char* mesh_path, std::vector<std::string> textures_vector)
	{
		Initialized = false;
		if (drawables.count(id) > 0)
		{
			VulkanObject* ref_obj = static_cast<VulkanObject*>(drawables[id]);
			ref_obj->LoadModel("", mesh_path, textures_vector, {});
		}
		else
		{
			return false;
		}

		Initialized = true;
		return true;
	}

	bool VulkanRenderer::loadModel(UINT id, const char* model_path)
	{
		Initialized = false;
		VulkanObject* ref_obj = static_cast<VulkanObject*>(drawables[id]);

		if (ref_obj != nullptr)
		{
			if (ref_obj->GetOwnerEntity()->HasProperty("Drawable"))
			{
				ref_obj->GetOwnerEntity()->ParsePropertyValue("Drawable", model_path);
			}
			else
			{
				ref_obj->GetOwnerEntity()->AddNewProperty("Drawable");
				ref_obj->GetOwnerEntity()->ParsePropertyValue("Drawable", model_path);
			}
		}

		Initialized = true;
		return true;
	}

	bool VulkanRenderer::assignTextures(std::vector<std::string> textures, GrEngine::Entity* target, GrEngine::TextureType type, bool update_object)
	{
		VulkanRenderer* rend = this;
		VulkanDrawable* object = nullptr;
		VkImageViewType viewtype = VK_IMAGE_VIEW_TYPE_2D;
		VkFormat imgformat = type == GrEngine::TextureType::Color ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;

		if (target->GetEntityType() == GrEngine::EntityType::SkyboxEntity)
		{
			object = static_cast<VulkanSkybox*>(target);
			viewtype = VK_IMAGE_VIEW_TYPE_CUBE;
		}
		else if (target->GetEntityType() == GrEngine::EntityType::TerrainEntity)
		{
			object = static_cast<VulkanTerrain*>(target);
		}
		else if (drawables.count(target->GetEntityID()) > 0)
		{
			object = static_cast<VulkanObject*>(drawables[target->GetEntityID()]);
		}

		if (object != nullptr)
		{
			std::vector<Texture*>* target_collection = type == GrEngine::TextureType::Color ? &object->object_texture : &object->object_normal;

			int procNum = 0;
			std::map<int, std::future<void>> processes_map;
			std::unordered_map<std::string, int> active_tex;

			for (int i = 0; i < target_collection->size(); i++)
			{
				resources.RemoveTexture(target_collection->at(i), logicalDevice, memAllocator);
				(*target_collection)[i] = nullptr;
			}
			target_collection->clear();

			if (viewtype == VK_IMAGE_VIEW_TYPE_CUBE)
			{
				target_collection->push_back(loadTexture({ textures }, type, viewtype, VK_IMAGE_TYPE_2D, imgformat)->AddLink());
			}
			else
			{
				//Precache
				//target_collection->resize(textures.size());
				for (auto texture : textures)
				{
					if (active_tex.count(texture) == 0)
					{
						processes_map[procNum] = std::async(std::launch::async, [rend, texture, object, viewtype, imgformat, type]()
							{
								rend->loadTexture({ texture }, type, viewtype, VK_IMAGE_TYPE_2D, imgformat);
							});

						active_tex[texture] = procNum;
						procNum++;
					}
				}

				for (int ind = 0; ind < processes_map.size(); ind++)
				{
					if (processes_map[ind].valid())
					{
						processes_map[ind].wait();
					}
				}

				for (int i = 0; i < textures.size(); i++)
				{
					target_collection->push_back(loadTexture({ textures[i] }, type, viewtype, VK_IMAGE_TYPE_2D, imgformat)->AddLink());
				}
			}

			if (update_object) object->updateObject();
		}

		return true;
	}

	//bool VulkanRenderer::assignNormals(std::vector<std::string> normals, GrEngine::Entity* target, bool update_object)
	//{
	//	VulkanRenderer* rend = this;
	//	VulkanDrawable* object = nullptr;

	//	if (target->GetEntityType() == GrEngine::EntityType::TerrainEntity)
	//	{
	//		object = static_cast<VulkanTerrain*>(target);
	//	}
	//	else if (drawables.count(target->GetEntityID()) > 0)
	//	{
	//		object = static_cast<VulkanObject*>(drawables[target->GetEntityID()]);
	//	}

	//	if (object != nullptr)
	//	{
	//		int texInd = 0;
	//		int procNum = 0;
	//		std::map<int, std::future<void>> processes_map;
	//		std::unordered_map<std::string, int> active_tex;

	//		for (int i = 0; i < object->object_normal.size(); i++)
	//		{
	//			resources.RemoveTexture(object->object_normal[i], logicalDevice, memAllocator);
	//		}
	//		object->object_normal.clear();

	//		//Precache
	//		if (normals.size() > 0)
	//		{
	//			object->object_normal.resize(normals.size());
	//		}
	//		else
	//		{
	//			object->object_normal.resize(object->object_texture.size());
	//			normals.resize(object->object_texture.size());
	//		}
	//		for (auto texture : normals)
	//		{
	//			if (active_tex.count(texture) == 0)
	//			{
	//				processes_map[procNum] = std::async(std::launch::async, [rend, texture, object]()
	//					{
	//						rend->loadTexture({ texture }, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM);
	//					});

	//				active_tex[texture] = procNum;
	//				procNum++;
	//			}
	//		}

	//		for (int ind = 0; ind < processes_map.size(); ind++)
	//		{
	//			if (processes_map[ind].valid())
	//			{
	//				processes_map[ind].wait();
	//			}
	//		}

	//		for (int i = 0; i < normals.size(); i++)
	//		{
	//			object->object_normal[i] = loadTexture({ normals[i] }, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM)->AddLink();
	//		}

	//		if (update_object) object->updateObject();
	//	}

	//	return true;
	//}

	Resource<Texture*>* VulkanRenderer::loadTexture(std::vector<std::string> texture_path, GrEngine::TextureType type, VkImageViewType type_view, VkImageType type_img, VkFormat format_img, bool default_to_black)
	{
		bool new_empty = false;

		if (texture_path.size() == 0)
		{
			int size = type_view == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1;
			for (int i = 0; i < size; i++)
			{
				texture_path.push_back("empty_texture");
			}
			new_empty = true;
		}
		else
		{
			int empties = 0;
			for (int i = 0; i < texture_path.size(); i++)
			{
				if (texture_path[i] == "")
				{
					texture_path[i] = "empty_texture";
					empties++;
				}
			}

			new_empty = empties == texture_path.size();
		}

		Resource<Texture*>* resource = resources.GetTextureResource(texture_path, type);

		if (resource == nullptr)
		{
			int maxW = 1;
			int maxH = 1;
			int channels = 4;
			unsigned char* data = nullptr;
			ShaderBuffer stagingBuffer;
			Texture* new_texture = new Texture();
			std::string solution = GrEngine::Globals::getExecutablePath();
			std::vector<std::string> fullpath;

			for (std::vector<std::string>::iterator itt = texture_path.begin(); itt != texture_path.end(); ++itt)
			{
				int width;
				int height;

				if ((*itt) != "empty_texture")
				{
					if ((*itt).size() >= solution.size() && (*itt).substr(0, solution.size()) == solution)
					{
						fullpath.push_back((*itt));
					}
					else
					{
						fullpath.push_back(solution + (*itt));
					}
					if (!stbi_info(fullpath.back().c_str(), &width, &height, &channels))
					{
						fullpath.back() = "empty_texture";
					}
					else
					{
						maxW = width > maxW ? width : maxW;
						maxH = height > maxH ? height : maxH;
					}
				}
				else
				{
					fullpath.push_back((*itt));
				}
			}

			data = (unsigned char*)malloc(maxW * maxH * channels * fullpath.size());
			//TBD: fix loading same image file on different surfaces
			int procNum = 0;
			int image_size = maxW * maxH * channels;
			for (std::vector<std::string>::iterator itt = fullpath.begin(); itt != fullpath.end(); ++itt)
			{
				const char* texture = (*itt).c_str();
				if ((*itt) != solution && (*itt) != "empty_texture")
				{
					int width;
					int height;
					int channels;
					stbi_uc* pixels = stbi_load(texture, &width, &height, &channels, STBI_rgb_alpha);

					if (width < maxW || height < maxH)
					{
						unsigned char* output = (unsigned char*)malloc(image_size);
						stbir_resize_uint8(pixels, width, height, 0, output, maxW, maxH, 0, channels);
						memcpy_s(data + image_size * procNum, image_size, output, image_size);
						stbi_image_free(pixels);
						free(output);
					}
					else
					{
						memcpy_s(data + image_size * procNum, image_size, pixels, image_size);
						stbi_image_free(pixels);
					}
				}
				else
				{
					unsigned char* pixels = (unsigned char*)malloc(image_size);
					memset(pixels, (byte)(255 * (1 - default_to_black)), image_size);
					memcpy_s(data + image_size * procNum, image_size, pixels, image_size);
					free(pixels);
					texture_path[procNum] = "empty_texture";
				}

				procNum++;
			}

			vkWaitForFences(logicalDevice, 1, &loadFence, 1, UINT64_MAX);
			vkResetFences(logicalDevice, 1, &loadFence);

			VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, data, maxW * maxH * channels * texture_path.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &stagingBuffer);
			free(data);

			uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(maxW, maxH)))) + 1;

			VkExtent3D imageExtent;
			imageExtent.width = static_cast<uint32_t>(maxW);
			imageExtent.height = static_cast<uint32_t>(maxH);
			imageExtent.depth = 1;

			VkImageCreateInfo dimg_info = VulkanAPI::StructImageCreateInfo(imageExtent, format_img, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, type_img);
			dimg_info.arrayLayers = texture_path.size();
			dimg_info.mipLevels = mipLevels;
			dimg_info.flags = type_view == VK_IMAGE_VIEW_TYPE_CUBE ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;

			VulkanAPI::CreateImage(memAllocator, &dimg_info, &new_texture->newImage.allocatedImage, &new_texture->newImage.allocation);

			VkCommandBuffer commandBuffer;
			VkImageSubresourceRange subresourceRange = VulkanAPI::StructSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevels, 0, texture_path.size());

			vkWaitForFences(logicalDevice, 1, &transitionFence, VK_TRUE, UINT64_MAX);
			VulkanAPI::AllocateCommandBuffers(logicalDevice, commandPool, &commandBuffer, 1);
			VulkanAPI::BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			VulkanAPI::TransitionImageLayout(new_texture->newImage.allocatedImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange, commandBuffer);
			VulkanAPI::EndAndSubmitCommandBuffer(logicalDevice, commandPool, commandBuffer, graphicsQueue, transitionFence);

			vkWaitForFences(logicalDevice, 1, &transitionFence, VK_TRUE, UINT64_MAX);
			VulkanAPI::CopyBufferToImage(logicalDevice, commandPool, stagingBuffer.Buffer, new_texture->newImage.allocatedImage, graphicsQueue, transitionFence, { (uint32_t)maxW, (uint32_t)maxH, (uint32_t)channels }, texture_path.size());

			generateMipmaps(new_texture->newImage.allocatedImage, format_img, maxW, maxH, mipLevels, texture_path.size());

			vkWaitForFences(logicalDevice, 1, &transitionFence, VK_TRUE, UINT64_MAX);
			VulkanAPI::AllocateCommandBuffers(logicalDevice, commandPool, &commandBuffer, 1);
			VulkanAPI::BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			VulkanAPI::TransitionImageLayout(new_texture->newImage.allocatedImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange, commandBuffer);
			VulkanAPI::EndAndSubmitCommandBuffer(logicalDevice, commandPool, commandBuffer, graphicsQueue, loadFence);

			VulkanAPI::CreateImageView(logicalDevice, format_img, new_texture->newImage.allocatedImage, subresourceRange, &new_texture->textureImageView, type_view);
			VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &stagingBuffer);
			VulkanAPI::CreateSampler(physicalDevice, logicalDevice, &new_texture->textureSampler, (float)mipLevels);

			new_texture->srcInfo.width = maxW;
			new_texture->srcInfo.height = maxH;
			new_texture->srcInfo.channels = channels;
			new_texture->texInfo.mipLevels = mipLevels;
			new_texture->texInfo.format = format_img;
			new_texture->texInfo.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			new_texture->texInfo.descriptor.sampler = new_texture->textureSampler;
			new_texture->texInfo.descriptor.imageView = new_texture->textureImageView;
			new_texture->initialized = true;
			new_texture->texture_collection = texture_path;

			//if (new_empty)
			//{
			//	resource = resources.AddTextureResource((std::string("empty_texture") + std::to_string(resources.CountTextures())).c_str(), new_texture);
			//}
			//else
			//{
			//	resource = resources.AddTextureResource(texture_path, new_texture);
			//}

			resource = resources.AddTextureResource(texture_path, new_texture, type);
		}

		return resource;
	}

	bool VulkanRenderer::updateTexture(GrEngine::Entity* target, int textureIndex)
	{
		VulkanDrawable* object = nullptr;

		if (target->GetEntityType() == GrEngine::EntityType::SkyboxEntity)
		{
			object = static_cast<VulkanSkybox*>(target);
		}
		else if (target->GetEntityType() == GrEngine::EntityType::TerrainEntity)
		{
			object = static_cast<VulkanTerrain*>(target);
		}
		else if (drawables.count(target->GetEntityID()) > 0)
		{
			object = static_cast<VulkanObject*>(drawables[target->GetEntityID()]);
		}
		else
		{
			return false;
		}

		bool res = updateResource(object->object_texture[textureIndex], 0);
		if (res)
		{
			object->updateObject();
		}

		return true;
	}

	bool VulkanRenderer::updateResource(Texture* target, int textureIndex)
	{
		Resource<Texture*>* resource = resources.GetTextureResource(target->texture_collection);
		std::string solution = GrEngine::Globals::getExecutablePath();

		if (resource != nullptr)
		{
			int maxW = target->srcInfo.width;
			int maxH = target->srcInfo.height;
			int channels = target->srcInfo.channels;
			uint32_t mipLevels = target->texInfo.mipLevels;

			VkCommandBuffer commandBuffer;
			std::vector<VkBufferImageCopy> regions;
			uint32_t offset = 0;

			for (int i = 0; i < target->texture_collection.size(); i++)
			{
				VkBufferImageCopy region{};
				region.bufferOffset = offset;

				region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				region.imageSubresource.mipLevel = 0;
				region.imageSubresource.baseArrayLayer = i;
				region.imageSubresource.layerCount = 1;

				region.imageOffset = { 0, 0, 0 };
				region.imageExtent = {
					(uint32_t)maxW,
					(uint32_t)maxH,
					1
				};

				regions.push_back(region);
				offset += maxW * maxH * channels;
			}

			ShaderBuffer stagingBuffer;
			VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, nullptr, maxW * maxH * channels * target->texture_collection.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &stagingBuffer);
			VkImageSubresourceRange subresourceRange = VulkanAPI::StructSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, target->texture_collection.size());

			vkWaitForFences(logicalDevice, 1, &transitionFence, VK_TRUE, UINT64_MAX);
			VulkanAPI::AllocateCommandBuffers(logicalDevice, commandPool, &commandBuffer, 1);
			VulkanAPI::BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			VulkanAPI::TransitionImageLayout(target->newImage.allocatedImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange, commandBuffer);
			vkCmdCopyImageToBuffer(commandBuffer, target->newImage.allocatedImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer.Buffer, regions.size(), regions.data());
			VulkanAPI::EndAndSubmitCommandBuffer(logicalDevice, commandPool, commandBuffer, graphicsQueue, transitionFence);

			size_t alloc_size = maxW * maxH * channels * target->texture_collection.size();
			unsigned char* data;
			vmaMapMemory(memAllocator, stagingBuffer.Allocation, (void**)&data);

			uint32_t buf_offset = maxW * maxH * channels * textureIndex;
			int width;
			int height;
			stbi_uc* pixels = stbi_load((solution + target->texture_collection[textureIndex]).c_str(), &width, &height, &channels, STBI_rgb_alpha);

			if (!pixels)
			{
				Logger::Out("An error occurred while loading the texture: %s", OutputColor::Green, OutputType::Error, target->texture_collection[textureIndex].c_str());
				vmaUnmapMemory(memAllocator, stagingBuffer.Allocation);
				return false;
			}

			if (width < maxW || height < maxH)
			{
				auto output = (unsigned char*)malloc(maxW * maxH * channels);
				stbir_resize_uint8(pixels, width, height, 0, output, maxW, maxH, 0, channels);
				memcpy_s(data + buf_offset, maxW * maxH * channels, output, maxW * maxH * channels); //sizeof(byte)
				free(pixels);
				free(output);
			}
			else
			{
				memcpy_s(data + buf_offset, width * height * channels, pixels, width * height * channels); //sizeof(byte)
				stbi_image_free(pixels);
			}

			vmaUnmapMemory(memAllocator, stagingBuffer.Allocation);

			VulkanAPI::DestroyImageView(target->textureImageView);
			VulkanAPI::DestroySampler(target->textureSampler);
			VulkanAPI::DestroyImage(target->newImage.allocatedImage);
			target->initialized = false;

			VkExtent3D imageExtent;
			imageExtent.width = static_cast<uint32_t>(maxW);
			imageExtent.height = static_cast<uint32_t>(maxH);
			imageExtent.depth = 1;

			VkImageCreateInfo dimg_info = VulkanAPI::StructImageCreateInfo(imageExtent, VK_FORMAT_R8G8B8A8_SRGB, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
			dimg_info.arrayLayers = target->texture_collection.size();
			dimg_info.mipLevels = mipLevels;
			dimg_info.flags = 0;

			VulkanAPI::CreateImage(memAllocator, &dimg_info, &target->newImage.allocatedImage, &target->newImage.allocation);

			vkWaitForFences(logicalDevice, 1, &transitionFence, VK_TRUE, UINT64_MAX);
			VulkanAPI::AllocateCommandBuffers(logicalDevice, commandPool, &commandBuffer, 1);
			VulkanAPI::BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			VulkanAPI::TransitionImageLayout(target->newImage.allocatedImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange, commandBuffer);
			VulkanAPI::EndAndSubmitCommandBuffer(logicalDevice, commandPool, commandBuffer, graphicsQueue, transitionFence);

			vkWaitForFences(logicalDevice, 1, &transitionFence, VK_TRUE, UINT64_MAX);
			VulkanAPI::CopyBufferToImage(logicalDevice, commandPool, stagingBuffer.Buffer, target->newImage.allocatedImage, graphicsQueue, transitionFence, { (uint32_t)maxW, (uint32_t)maxH, (uint32_t)channels }, target->texture_collection.size());


			generateMipmaps(target->newImage.allocatedImage, target->texInfo.format, maxW, maxH, mipLevels, target->texture_collection.size());


			subresourceRange.levelCount = mipLevels;
			vkWaitForFences(logicalDevice, 1, &transitionFence, VK_TRUE, UINT64_MAX);
			VulkanAPI::AllocateCommandBuffers(logicalDevice, commandPool, &commandBuffer, 1);
			VulkanAPI::BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			VulkanAPI::TransitionImageLayout(target->newImage.allocatedImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange, commandBuffer);
			VulkanAPI::EndAndSubmitCommandBuffer(logicalDevice, commandPool, commandBuffer, graphicsQueue, transitionFence);

			VulkanAPI::CreateImageView(logicalDevice, VK_FORMAT_R8G8B8A8_SRGB, target->newImage.allocatedImage, subresourceRange, &target->textureImageView, VK_IMAGE_VIEW_TYPE_2D_ARRAY);
			VulkanAPI::CreateSampler(physicalDevice, logicalDevice, &target->textureSampler, (float)mipLevels);

			target->texture_collection = target->texture_collection;
			target->srcInfo.width = maxW;
			target->srcInfo.height = maxH;
			target->srcInfo.channels = channels;
			target->texInfo.mipLevels = mipLevels;
			target->texInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
			target->texInfo.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			target->texInfo.descriptor.sampler = target->textureSampler;
			target->texInfo.descriptor.imageView = target->textureImageView;
			target->initialized = true;

			VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &stagingBuffer);
			return true;
		}
		else
		{
			return false;
		}
	}

	bool VulkanRenderer::updateResource(Texture* target, byte* pixels)
	{
		ShaderBuffer stagingBuffer;
		VkCommandBuffer commandBuffer;
		VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, pixels, sizeof(byte) * target->srcInfo.width * target->srcInfo.height * target->srcInfo.channels * target->texture_collection.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &stagingBuffer);
		VkImageSubresourceRange subresourceRange = VulkanAPI::StructSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, target->texture_collection.size());

		VulkanAPI::AllocateCommandBuffers(logicalDevice, commandPool, &commandBuffer, 1);
		vkWaitForFences(logicalDevice, 1, &transitionFence, VK_TRUE, UINT64_MAX);
		VulkanAPI::BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		VulkanAPI::TransitionImageLayout(target->newImage.allocatedImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange, commandBuffer);
		VulkanAPI::EndAndSubmitCommandBuffer(logicalDevice, commandPool, commandBuffer, graphicsQueue, transitionFence);

		vkWaitForFences(logicalDevice, 1, &transitionFence, VK_TRUE, UINT64_MAX);
		VulkanAPI::CopyBufferToImage(logicalDevice, commandPool, stagingBuffer.Buffer, target->newImage.allocatedImage, graphicsQueue, transitionFence, target->srcInfo, target->texture_collection.size());


		generateMipmaps(target->newImage.allocatedImage, target->texInfo.format, target->srcInfo.width, target->srcInfo.height, target->texInfo.mipLevels, target->texture_collection.size());


		subresourceRange.levelCount = target->texInfo.mipLevels;
		vkWaitForFences(logicalDevice, 1, &transitionFence, VK_TRUE, UINT64_MAX);
		VulkanAPI::AllocateCommandBuffers(logicalDevice, commandPool, &commandBuffer, 1);
		VulkanAPI::BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		VulkanAPI::TransitionImageLayout(target->newImage.allocatedImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange, commandBuffer);
		VulkanAPI::EndAndSubmitCommandBuffer(logicalDevice, commandPool, commandBuffer, graphicsQueue, transitionFence);

		VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &stagingBuffer);
		return true;
	}


	bool VulkanRenderer::updateResource(Texture* target, byte* pixels, uint32_t width, uint32_t height, uint32_t offset_x, uint32_t offset_y)
	{
		byte* data;
		ShaderBuffer stagingBuffer;
		VkCommandBuffer commandBuffer;
		VkImageSubresourceRange subresourceRange = VulkanAPI::StructSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, target->texture_collection.size());

		VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, nullptr, sizeof(byte) * width * target->srcInfo.channels * height, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &stagingBuffer);

		vkWaitForFences(logicalDevice, 1, &transitionFence, VK_TRUE, UINT64_MAX);
		VulkanAPI::AllocateCommandBuffers(logicalDevice, commandPool, &commandBuffer, 1);
		VulkanAPI::BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		VulkanAPI::TransitionImageLayout(target->newImage.allocatedImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange, commandBuffer);
		VulkanAPI::EndAndSubmitCommandBuffer(logicalDevice, commandPool, commandBuffer, graphicsQueue, transitionFence);

		std::vector<VkBufferImageCopy> regions;
		VkBufferImageCopy region{};
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		int whole_size = target->srcInfo.width * target->srcInfo.height * target->srcInfo.channels;
		int index = 0;

		vmaMapMemory(memAllocator, stagingBuffer.Allocation, (void**)&data);
		memset(data, (byte)0, width * target->srcInfo.channels * height);
		for (int i = 0; i < height; i++)
		{
			uint32_t offset = (offset_x + (offset_y + i) * target->srcInfo.width) * target->srcInfo.channels;
			if (offset > whole_size)
				continue;

			memcpy_s(data + width * target->srcInfo.channels * index, width * target->srcInfo.channels, pixels + offset,  width * target->srcInfo.channels); //sizeof(byte)

			region.bufferOffset = width * target->srcInfo.channels * index;
			region.imageOffset = { (int)offset_x, (int)offset_y + i, 0 };
			region.imageExtent = {
				width,
				1,
				1
			};

			regions.push_back(region);
			index++;
		}
		vmaUnmapMemory(memAllocator, stagingBuffer.Allocation);

		vkWaitForFences(logicalDevice, 1, &transitionFence, VK_TRUE, UINT64_MAX);
		VulkanAPI::AllocateCommandBuffers(logicalDevice, commandPool, &commandBuffer, 1);
		VulkanAPI::BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.Buffer, target->newImage.allocatedImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regions.size(), regions.data());
		VulkanAPI::EndAndSubmitCommandBuffer(logicalDevice, commandPool, commandBuffer, graphicsQueue, transitionFence);


		generateMipmaps(target->newImage.allocatedImage, target->texInfo.format, target->srcInfo.width, target->srcInfo.height, target->texInfo.mipLevels, target->texture_collection.size());

		vkWaitForFences(logicalDevice, 1, &transitionFence, VK_TRUE, UINT64_MAX);
		subresourceRange.levelCount = target->texInfo.mipLevels;
		VulkanAPI::AllocateCommandBuffers(logicalDevice, commandPool, &commandBuffer, 1);
		VulkanAPI::BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		VulkanAPI::TransitionImageLayout(target->newImage.allocatedImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange, commandBuffer);
		VulkanAPI::EndAndSubmitCommandBuffer(logicalDevice, commandPool, commandBuffer, graphicsQueue, transitionFence);
		VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &stagingBuffer);

		return true;
	}

	void VulkanRenderer::generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels, uint32_t arrayLevels)
	{
#ifdef _DEBUG
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) 
		{
			throw std::runtime_error("texture image format does not support linear blitting!");
		}
#endif

		VkCommandBuffer commandBuffer;

		if (mipLevels == 1)
			return;

		for (int k = 0; k < arrayLevels; k++)
		{
			VulkanAPI::AllocateCommandBuffers(logicalDevice, commandPool, &commandBuffer, 1);
			VulkanAPI::BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = image;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseArrayLayer = k;
			barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
			barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;

			int32_t mipWidth = texWidth;
			int32_t mipHeight = texHeight;

			for (uint32_t i = 1; i < mipLevels; i++) 
			{
				barrier.subresourceRange.baseMipLevel = i - 1;
				barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

				vkCmdPipelineBarrier(commandBuffer,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
					0, nullptr,
					0, nullptr,
					1, &barrier);

				VkImageBlit blit{};
				blit.srcOffsets[0] = { 0, 0, 0 };
				blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
				blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.srcSubresource.mipLevel = i - 1;
				blit.srcSubresource.baseArrayLayer = k;
				blit.srcSubresource.layerCount = 1;
				blit.dstOffsets[0] = { 0, 0, 0 };
				blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
				blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.dstSubresource.mipLevel = i;
				blit.dstSubresource.baseArrayLayer = k;
				blit.dstSubresource.layerCount = 1;

				vkCmdBlitImage(commandBuffer,
					image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1, &blit,
					VK_FILTER_LINEAR);

				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				vkCmdPipelineBarrier(commandBuffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
					0, nullptr,
					0, nullptr,
					1, &barrier);

				if (mipWidth > 1) mipWidth /= 2;
				if (mipHeight > 1) mipHeight /= 2;
			}

			barrier.subresourceRange.baseMipLevel = mipLevels - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			vkWaitForFences(logicalDevice, 1, &transitionFence, VK_TRUE, UINT64_MAX);
			VulkanAPI::EndAndSubmitCommandBuffer(logicalDevice, commandPool, commandBuffer, graphicsQueue, transitionFence);
		}
	}

	void VulkanRenderer::LoadTerrain(int resolution, int width, int height, int depth, std::array<std::string, 6> maps, std::array<std::string, 4> normals, std::array<std::string, 4> displacement)
	{
		if (terrain != nullptr)
		{
			terrain->destroyObject();
			entities.erase(terrain->GetEntityID());
			delete terrain;
			terrain = nullptr;
		}

		terrain = new VulkanTerrain(100);
		terrain->initObject(logicalDevice, memAllocator, this);
		entities[terrain->GetEntityID()] = terrain;

		terrain->GenerateTerrain(resolution, width, height, depth, maps, normals, displacement);
	}

	void VulkanRenderer::LoadTerrain(const char* filepath)
	{
		if (terrain != nullptr)
		{
			terrain->destroyObject();
			entities.erase(terrain->GetEntityID());
			delete terrain;
			terrain = nullptr;
		}

		terrain = new VulkanTerrain(100);
		terrain->initObject(logicalDevice, memAllocator, this);
		entities[terrain->GetEntityID()] = terrain;

		terrain->LoadTerrain(filepath);
	}

	GrEngine::Object* VulkanRenderer::InitDrawableObject(GrEngine::Entity* ownerEntity)
	{
		VulkanObject* drawable = new VulkanObject(ownerEntity);
		drawables[ownerEntity->GetEntityID()] = drawable;
		drawable->initObject(logicalDevice, memAllocator, this);

		return drawable;
	}

	GrEngine::LightObject* VulkanRenderer::InitSpotlightObject(GrEngine::Entity* ownerEntity)
	{
		VulkanSpotlight* light = new VulkanSpotlight(ownerEntity);
		light->initLight(logicalDevice, memAllocator);
		lights[ownerEntity->GetEntityID()] = static_cast<GrEngine::LightObject*>(light);
		updateShadowResources();

		return light;
	}

	GrEngine::LightObject* VulkanRenderer::InitCascadeLightObject(GrEngine::Entity* ownerEntity)
	{
		cascade_count++;
		VulkanCascade* light = new VulkanCascade(ownerEntity);
		light->initLight(logicalDevice, memAllocator);
		lights[ownerEntity->GetEntityID()] = static_cast<GrEngine::LightObject*>(light);
		updateShadowResources();

		return light;
	}

	GrEngine::LightObject* VulkanRenderer::InitPointLightObject(GrEngine::Entity* ownerEntity)
	{
		VulkanPointLight* light = new VulkanPointLight(ownerEntity);
		light->initLight(logicalDevice, memAllocator);
		lights[ownerEntity->GetEntityID()] = static_cast<GrEngine::LightObject*>(light);
		updateShadowResources();

		return light;
	}

	GrEngine::LightObject* VulkanRenderer::InitOmniLightObject(GrEngine::Entity* ownerEntity)
	{
		omni_count++;
		VulkanOmniLight* light = new VulkanOmniLight(ownerEntity);
		light->initLight(logicalDevice, memAllocator);
		lights[ownerEntity->GetEntityID()] = static_cast<GrEngine::LightObject*>(light);
		updateShadowResources();

		return light;
	}

	GrEngine::Entity* VulkanRenderer::addEntity()
	{
		GrEngine::Entity* ent = nullptr;

		UINT id = 1000000000;
		while (entities.contains(id) > 0)
			id++;

		ent = new GrEngine::Entity(id);
		std::string new_name = std::string("Entity") + std::to_string(entities.size());
		ent->UpdateNameTag(new_name.c_str());
		entities[id] = ent;

		return ent;
	}

	GrEngine::Entity* VulkanRenderer::addEntity(UINT id)
	{
		GrEngine::Entity* ent = nullptr;

		ent = new GrEngine::Entity(id);
		std::string new_name = std::string("Entity") + std::to_string(entities.size());
		ent->UpdateNameTag(new_name.c_str());
		entities[id] = ent;

		return ent;
	}

	void VulkanRenderer::addEntity(GrEngine::Entity* entity)
	{
		std::string new_name = std::string("Entity") + std::to_string(entities.size() + 1);
		entity->UpdateNameTag(new_name.c_str());
		entities[entity->GetEntityID()] = entity;
	}

	GrEngine::Entity* VulkanRenderer::CloneEntity(UINT id)
	{
		GrEngine::Entity* ent = nullptr;
		if (entities.count(id) > 0)
		{
			ent = addEntity();
			auto props = entities.at(id)->GetProperties();
			for (std::vector<EntityProperty*>::iterator itt = props.begin(); itt != props.end(); ++itt)
			{
				ent->AddNewProperty((*itt)->GetPropertyType());
				ent->ParsePropertyValue((*itt)->GetPropertyType(), (*itt)->ValueString());
			}
		}

		return ent;
	}

	GrEngine::Entity* VulkanRenderer::selectEntity(UINT ID)
	{
		selected_entity = ID;
		std::vector<double> para = { (double)selected_entity };
		listener->registerEvent(EventType::SelectionChanged, para);

		if (entities.contains(ID))
		{
			if (highlight_selection)
				VulkanObject::selected_id = ID;
			else
				VulkanObject::selected_id = 0;

			return entities.at(ID);
		}

		selected_entity = 0;
		VulkanObject::selected_id = 0;
		return nullptr;
	}

	void VulkanRenderer::DeleteEntity(UINT id)
	{
		waitForRenderer();

		if (entities.at(id) != nullptr)
		{
			if ((entities.at(id)->GetEntityType() & GrEngine::EntityType::ObjectEntity) != 0)
			{
				static_cast<VulkanObject*>(drawables.at(id))->destroyObject();
				drawables.erase(id);
			}
			if ((entities.at(id)->GetEntityType() & GrEngine::EntityType::SpotlightEntity) != 0)
			{
				static_cast<VulkanSpotlight*>(lights.at(id))->destroyLight();
				lights.erase(id);
				updateShadowResources();
			}
			else if ((entities.at(id)->GetEntityType() & GrEngine::EntityType::CascadeLightEntity) != 0)
			{
				static_cast<VulkanCascade*>(lights.at(id))->destroyLight();
				lights.erase(id);
				cascade_count--;
				updateShadowResources();
			}
			else if ((entities.at(id)->GetEntityType() & GrEngine::EntityType::PointLightEntity) != 0)
			{
				static_cast<VulkanCascade*>(lights.at(id))->destroyLight();
				lights.erase(id);
				updateShadowResources();
			}
			else if ((entities.at(id)->GetEntityType() & GrEngine::EntityType::OmniLightEntity) != 0)
			{
				static_cast<VulkanOmniLight*>(lights.at(id))->destroyLight();
				omni_count--;
				lights.erase(id);
				updateShadowResources();
			}

			entities.erase(id);

			if (id == selected_entity)
			{
				selectEntity(0);
			}
		}
	}

	void VulkanRenderer::SetHighlightingMode(bool enabled)
	{
		highlight_selection = enabled;
	}

	void VulkanRenderer::waitForRenderer()
	{
		vkWaitForFences(logicalDevice, renderFence.size(), renderFence.data(), TRUE, UINT64_MAX);
		//vkDeviceWaitIdle(logicalDevice);
		//vkQueueWaitIdle(graphicsQueue);
	}

	std::vector<std::string> VulkanRenderer::GetMaterialNames(const char* mesh_path)
	{
		std::string solution = GrEngine::Globals::getExecutablePath();
		std::string model_path = mesh_path;
		Assimp::Importer importer;
		const aiScene* model;
		std::vector<std::string> output;

		if (model_path.size() >= solution.size() && model_path.substr(0, solution.size()) == solution)
		{
			model = importer.ReadFile(mesh_path, 0);
		}
		else
		{
			model = importer.ReadFile(solution + mesh_path, 0);
		}

		if (model == NULL)
		{
			Logger::Out("Could not load the mesh %c%s%c!", OutputColor::Red, OutputType::Error, '"', mesh_path, '"');
			return output;
		}

		for (int mesh_ind = 0; mesh_ind < model->mNumMeshes; mesh_ind++)
		{
			aiString material;
			aiGetMaterialString(model->mMaterials[model->mMeshes[mesh_ind]->mMaterialIndex], AI_MATKEY_NAME, &material);
			output.push_back(material.C_Str());
		}

		return output;
	}

	void VulkanRenderer::SaveScene(const char* path)
	{
		std::string directory = path;
		directory = directory.erase(directory.find_last_of('.', directory.size()), directory.size());
		std::fstream new_file;
		new_file.open(path, std::fstream::out | std::ios::trunc);

		if (!new_file)
		{
			Logger::Out("Couldn't create file for saving!", OutputColor::Red, OutputType::Error);
			return;
		}

		getActiveViewport()->ClampAxes();
		new_file << "viewport\n{\n";
		std::vector<EntityProperty*> cur_props = getActiveViewport()->GetProperties();
		for (std::vector<EntityProperty*>::iterator itt = cur_props.begin(); itt != cur_props.end(); ++itt)
		{
			new_file << "    " << (*itt)->PropertyNameString() << " " << (*itt)->ValueString() << "\n";
		}
		new_file << "}\n";

		for (std::map<UINT, GrEngine::Entity*>::iterator itt = entities.begin(); itt != entities.end(); ++itt)
		{
			if ((*itt).second->IsStatic() == false || (*itt).second == sky)
			{
				new_file << (*itt).second->GetTypeString() << " " << (*itt).second->GetEntityID() << "\n{\n";
				cur_props = (*itt).second->GetProperties();
				for (std::vector<EntityProperty*>::iterator itt = cur_props.begin(); itt != cur_props.end(); ++itt)
				{
					new_file << "   " << (*itt)->PropertyNameString() << " " << (*itt)->ValueString() << "\n";
				}
				new_file << "}\n";
			}
		}

		if (terrain != nullptr)
		{
			terrain->SaveTerrain((directory + ".terg").c_str());
		}

		new_file << '\0';
		new_file.close();
	}

	void VulkanRenderer::LoadScene(const char* path)
	{
		clearDrawables();
		int resizable = glfwGetWindowAttrib(pParentWindow, GLFW_RESIZABLE);
		glfwSetWindowAttrib(pParentWindow, GLFW_RESIZABLE, 0);
		VulkanDrawable::skip_update = true;
		//Initialized = false;

		std::ifstream file(path, std::ios::ate | std::ios::binary);

		if (!file.is_open())
		{
			Logger::Out("Couldn't open level %s", OutputColor::Red, OutputType::Error, path);
			return;
		}

		file.seekg(0);
		std::string stream;
		std::string cur_property;
		bool block_open = false;
		GrEngine::Entity* cur_ent = nullptr;

		while (file >> stream && !file.eof())
		{
			if (stream == "{")
			{
				block_open = true;
			}
			else if (stream == "}")
			{
				block_open = false;
			}
			else if (block_open && cur_ent != nullptr)
			{
				cur_property = stream;
				cur_ent->AddNewProperty(cur_property.c_str());
				file >> stream;
				cur_ent->ParsePropertyValue(cur_property.c_str(), stream.c_str());
			}
			else if (!block_open && stream == "Entity")
			{
				file >> stream;
				cur_ent = addEntity(std::atoi(stream.c_str()));
			}
			else if (!block_open && stream == "viewport")
			{
				cur_ent = viewport_camera;
			}
			else if (!block_open && stream == "Skybox")
			{
				cur_ent = sky;
				entities[sky->GetEntityID()] = sky;
			}
			else if (!block_open && stream == "Terrain")
			{
				std::string directory = path;
				std::string terrain_path = (directory.erase(directory.find_last_of('.', directory.size()), directory.size()) + ".terg");
				LoadTerrain(terrain_path.c_str());
				cur_ent = terrain;
			}
		}
		file.close();
		VulkanDrawable::skip_update = false;

		//viewport_camera->SetRotation(viewport_camera->GetObjectOrientation());
		viewport_camera->PositionObjectAt(viewport_camera->GetObjectPosition());
		glfwSetWindowAttrib(pParentWindow, GLFW_RESIZABLE, resizable);
		recreateSwapChain();
		//Initialized = true;
	}
};