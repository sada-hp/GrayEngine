#include <pch.h>
#define GLFW_INCLUDE_VULKAN
#define VMA_IMPLEMENTATION
#define VK_KHR_swapchain
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "VulkanAPI.h"

namespace GrEngine_Vulkan
{
	void VulkanAPI::destroy()
	{
		if (Initialized == false) return;
		Initialized = false;

		vkDeviceWaitIdle(logicalDevice);
		vkQueueWaitIdle(graphicsQueue);

		clearDrawables();
		grid.destroyObject(logicalDevice, memAllocator);
		cleanupSwapChain();
		vkDestroySemaphore(logicalDevice, renderFinishedSemaphore, nullptr);
		vkDestroySemaphore(logicalDevice, imageAvailableSemaphore, nullptr);
		vkDestroyFence(logicalDevice, graphicsFence, nullptr);
		vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
		vmaDestroyAllocator(memAllocator);
		vkDestroyDevice(logicalDevice, nullptr);
		vkDestroySurfaceKHR(_vulkan, surface, nullptr);
		vkDestroyInstance(_vulkan, nullptr);
	}

	bool VulkanAPI::init(void* window) //Vulkan integration done with a help of vulkan-tutorial.com
	{
		bool res = true;
		pParentWindow = reinterpret_cast<GLFWwindow*>(window);

		if (!createVKInstance())
			throw std::runtime_error("Failed to create vulkan instance!");

		uint32_t deviceCount = 0;

		vkEnumeratePhysicalDevices(_vulkan, &deviceCount, nullptr);
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(_vulkan, &deviceCount, devices.data());

		if (deviceCount == 0)
			throw std::runtime_error("Failed to find GPUs with Vulkan support!");

		if (!glfwCreateWindowSurface(_vulkan, pParentWindow, nullptr, &surface) == VK_SUCCESS)
			Logger::Out("[Vk] Failed to create presentation surface", OutputColor::Red, OutputType::Error);

		for (const auto& device : devices) {
			if (isDeviceSuitable(device)) {
				physicalDevice = device;
				vkGetPhysicalDeviceProperties(physicalDevice, &deviceProps);
				msaaSamples = getMaxUsableSampleCount();
				break;
			}
		}

		if (physicalDevice == VK_NULL_HANDLE)
			throw std::exception("Failed to find suitable Vulkan device");
		else
			Logger::Out("Presentation device: %s", OutputColor::Green, OutputType::Log, deviceProps.deviceName);

		if ((res = createLogicalDevice() & res) == false)
			Logger::Out("[Vk] Failed to create logical device", OutputColor::Red, OutputType::Error);

		if ((res = createMemoryAllocator() & res) == false)
			Logger::Out("[Vk] Failed to create memory allocator", OutputColor::Red, OutputType::Error);

		if ((res = createSwapChain() & res) == false)
			Logger::Out("[Vk] Failed to create swap chain", OutputColor::Red, OutputType::Error);

		swapChainImageViews.resize(swapChainImages.size());

		for (std::size_t i = 0; i < swapChainImages.size(); i++) {
			if ((res = createImageViews(swapChainImageFormat, VK_IMAGE_VIEW_TYPE_2D, swapChainImages[i], &swapChainImageViews[i]) & res) == false)
				Logger::Out("[Vk] Failed to create image views", OutputColor::Red, OutputType::Error);
		}
	
		if ((res = createRenderPass() & res) == false)
			Logger::Out("[Vk] Failed to create render pass", OutputColor::Red, OutputType::Error);

		if ((res = createFramebuffers() & res) == false)
			Logger::Out("[Vk] Failed to create framebuffer", OutputColor::Red, OutputType::Error);

		if ((res = createCommandPool() & res) == false)
			Logger::Out("[Vk] Failed to create command pool", OutputColor::Red, OutputType::Error);

		if ((res = createCommandBuffers() & res) == false)
			Logger::Out("[Vk] Failed to create command buffer", OutputColor::Red, OutputType::Error);

		if ((res = createSemaphores() & res) == false)
			Logger::Out("[Vk] Failed to create semaphores", OutputColor::Red, OutputType::Error);

		grid.shader_path = "Shaders//grid";
		grid.far_plane = 10000.f;

		grid.initObject(logicalDevice, memAllocator, this);

		vkDeviceWaitIdle(logicalDevice);

		return Initialized = res;
	}

	void VulkanAPI::createSkybox(const char* East, const char* West, const char* Top, const char* Bottom, const char* North, const char* South)
	{
		Initialized = false;

		if (sky >= 0)
		{
			dynamic_cast<VulkanDrawable*>(entities[sky])->destroyObject(logicalDevice, memAllocator);
			delete entities[sky];
			entities.erase(sky);
			sky = -1;
		}

		GrEngine::EntityInfo inf = addEntity();
		VulkanDrawable* back = dynamic_cast<VulkanDrawable*>(entities[inf.EntityID]);
		back->shader_path = "Shaders//background";
		back->invalidateTexture(logicalDevice, memAllocator);

		std::vector<std::string> mat_vector = { East, West, Top, Bottom, North, South };
		loadTexture(mat_vector, back, VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_TYPE_2D);

		back->updateObject(logicalDevice, memAllocator);
		sky = inf.EntityID;

		Initialized = true;
	}

	void VulkanAPI::RenderFrame()
	{
		drawFrame(cur_mode, true);
	}

	void VulkanAPI::drawFrame(DrawMode mode, bool Show)
	{
		if (!Initialized) return;

		currentImageIndex = 0;
		vkAcquireNextImageKHR(logicalDevice, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &currentImageIndex);

		if (!updateDrawables(currentImageIndex, mode))
			throw std::runtime_error("Logical device was lost!");

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[currentImageIndex];
		VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		VkResult res;
		vkResetFences(logicalDevice, 1, &graphicsFence);
		res = vkQueueSubmit(graphicsQueue, 1, &submitInfo, graphicsFence);

		if (res == VK_ERROR_DEVICE_LOST)
		{
			Logger::Out("Logical device was lost!", OutputColor::Red, OutputType::Error);
			throw std::runtime_error("Logical device was lost!");
		}

		if (Show)
		{
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
		}

		vkWaitForFences(logicalDevice, 1, &graphicsFence, TRUE, UINT64_MAX);
	}
	void VulkanAPI::SaveScreenshot(const char* filepath)
	{
		drawFrame(DrawMode::NORMAL, false);

		VkImage srcImage = swapChainImages[currentImageIndex];
		VkImage dstImage;
		VkCommandBuffer cmd;
		VkImageCreateInfo dstInf{};
		dstInf.imageType = VK_IMAGE_TYPE_2D;
		dstInf.format = VK_FORMAT_R8G8B8A8_UNORM;
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
		memAllocInfo.allocationSize = memRequirements.size;
		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		allocInfo.flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		vmaFindMemoryTypeIndex(memAllocator, memRequirements.memoryTypeBits, &allocInfo, &memAllocInfo.memoryTypeIndex);
		vkAllocateMemory(logicalDevice, &memAllocInfo, nullptr, &dstImageMemory);
		vkBindImageMemory(logicalDevice, dstImage, dstImageMemory, 0);

		allocateCommandBuffer(&cmd);
		beginCommandBuffer(cmd, VK_COMMAND_BUFFER_USAGE_FLAG_BITS_MAX_ENUM);
		transitionImageLayout(dstImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, cmd);
		transitionImageLayout(srcImage, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, cmd);

		VkImageCopy imageCopyRegion{};
		imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.srcSubresource.layerCount = 1;
		imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.dstSubresource.layerCount = 1;
		imageCopyRegion.extent.width = swapChainExtent.width;
		imageCopyRegion.extent.height = swapChainExtent.height;
		imageCopyRegion.extent.depth = 1;

		vkCmdCopyImage(cmd, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);

		transitionImageLayout(dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, cmd);
		transitionImageLayout(srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, cmd);
		freeCommandBuffer(cmd);

		VkImageSubresource subResource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
		VkSubresourceLayout subResourceLayout;
		vkGetImageSubresourceLayout(logicalDevice, dstImage, &subResource, &subResourceLayout);

		std::ofstream file;
		if (filepath != "")
		{
			file.open(filepath, std::ios::out | std::ios::binary);
			file << "P6\n" << swapChainExtent.width << "\n" << swapChainExtent.height << "\n" << 255 << "\n";
			const char* data;
			vkMapMemory(logicalDevice, dstImageMemory, 0, VK_WHOLE_SIZE, 0, (void**)&data);
			data += subResourceLayout.offset;
			for (uint32_t y = 0; y < swapChainExtent.height; y++)
			{
				unsigned int* row = (unsigned int*)data;
				for (uint32_t x = 0; x < swapChainExtent.width; x++)
				{
					file.write((char*)row + 2, 1);
					file.write((char*)row + 1, 1);
					file.write((char*)row, 1);
					row++;
				}
				data += subResourceLayout.rowPitch;
			}

			vkUnmapMemory(logicalDevice, dstImageMemory);
		}
		vkFreeMemory(logicalDevice, dstImageMemory, nullptr);
		vkDestroyImage(logicalDevice, dstImage, nullptr);
	}

	void VulkanAPI::SelectEntityAtCursor()
	{
		drawFrame(DrawMode::IDS, false);

		VkImage srcImage = swapChainImages[currentImageIndex];
		VkImage dstImage;
		VkCommandBuffer cmd;
		VkImageCreateInfo dstInf{};
		dstInf.imageType = VK_IMAGE_TYPE_2D;
		dstInf.format = VK_FORMAT_R8G8B8A8_UNORM;
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
		memAllocInfo.allocationSize = memRequirements.size;
		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		allocInfo.flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		vmaFindMemoryTypeIndex(memAllocator, memRequirements.memoryTypeBits, &allocInfo, &memAllocInfo.memoryTypeIndex);
		vkAllocateMemory(logicalDevice, &memAllocInfo, nullptr, &dstImageMemory);
		vkBindImageMemory(logicalDevice, dstImage, dstImageMemory, 0);

		allocateCommandBuffer(&cmd);
		beginCommandBuffer(cmd, VK_COMMAND_BUFFER_USAGE_FLAG_BITS_MAX_ENUM);
		transitionImageLayout(dstImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, cmd);
		transitionImageLayout(srcImage, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, cmd);

		VkImageCopy imageCopyRegion{};
		imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.srcSubresource.layerCount = 1;
		imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.dstSubresource.layerCount = 1;
		imageCopyRegion.extent.width = swapChainExtent.width;
		imageCopyRegion.extent.height = swapChainExtent.height;
		imageCopyRegion.extent.depth = 1;

		vkCmdCopyImage(cmd, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);

		transitionImageLayout(dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, cmd);
		transitionImageLayout(srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, cmd);
		freeCommandBuffer(cmd);

		VkImageSubresource subResource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
		VkSubresourceLayout subResourceLayout;
		vkGetImageSubresourceLayout(logicalDevice, dstImage, &subResource, &subResourceLayout);

		const char* data;
		vkMapMemory(logicalDevice, dstImageMemory, 0, VK_WHOLE_SIZE, 0, (void**)&data);
		data += subResourceLayout.offset;
		double xpos, ypos;
		glfwGetCursorPos(pParentWindow, &xpos, &ypos);
		data += subResourceLayout.rowPitch * (int)ypos;
		unsigned int* row = (unsigned int*)data + (int)xpos;

		double r = ((unsigned char*)row + 2)[0];
		double g = ((unsigned char*)row + 1)[0];
		double b = ((unsigned char*)row)[0];

		if ((int)r > 0 && (int)g > 0 && (int)b > 0)
		{
			char buf[11];
			char _buf[11];
			std::snprintf(_buf, sizeof(_buf), "1%03d%03d%03d", (int)r, (int)g, (int)b);
			int id = atoi(_buf);
			selectEntity(id);
		}
		else
		{
			VulkanDrawable::opo.selected_entity = { 0, 0, 0 };
			selectEntity(0);
		}

		vkUnmapMemory(logicalDevice, dstImageMemory);
		vkFreeMemory(logicalDevice, dstImageMemory, nullptr);
		vkDestroyImage(logicalDevice, dstImage, nullptr);
	}

	bool VulkanAPI::updateDrawables(uint32_t index, DrawMode mode)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffers[index], &beginInfo) != VK_SUCCESS)
			return false;

		VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 0.0f}} };
		VkClearValue depthClear;
		depthClear.depthStencil.depth = 1.f;

		VkClearValue clearValues[] = { clearColor , depthClear };

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = swapChainFramebuffers[index];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChainExtent;
		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = clearValues;

		vkCmdBeginRenderPass(commandBuffers[index], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		VkViewport viewport{};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffers[index], 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;
		vkCmdSetScissor(commandBuffers[index], 0, 1, &scissor);

		for (auto object : entities)
		{
			if (object.second->GetEntityType() == "VulkanDrawable" && dynamic_cast<VulkanDrawable*>(object.second)->IsVisible())
			{
				dynamic_cast<VulkanDrawable*>(object.second)->recordCommandBuffer(logicalDevice, commandBuffers[index], swapChainExtent, mode);
			}
		}

		grid.recordCommandBuffer(logicalDevice, commandBuffers[index], swapChainExtent, mode);

		vkCmdEndRenderPass(commandBuffers[index]);

		return vkEndCommandBuffer(commandBuffers[index]) == VK_SUCCESS;
	}

	bool VulkanAPI::createLogicalDevice()
	{
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.sampleRateShading = VK_TRUE;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledLayerCount = 0;

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS)
			return false;

		vkGetDeviceQueue(logicalDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(logicalDevice, indices.presentFamily.value(), 0, &presentQueue);

		return true;
	}

	bool VulkanAPI::createVKInstance()
	{
#ifndef _DEBUG
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Gray Engine App";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Gray Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

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
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

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

	bool VulkanAPI::createMemoryAllocator()
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
		VkDeviceSize heapSize;
		heapSize = memProperties.memoryHeapCount;
		VmaAllocatorCreateInfo vmaInfo{};
		vmaInfo.physicalDevice = physicalDevice;
		vmaInfo.device = logicalDevice;
		vmaInfo.instance = _vulkan;
		vmaInfo.vulkanApiVersion = VK_API_VERSION_1_0;
		vmaInfo.flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT;
		vmaInfo.pHeapSizeLimit = &heapSize;
		vmaInfo.pTypeExternalMemoryHandleTypes = &memProperties.memoryTypeCount;
		vmaInfo.preferredLargeHeapBlockSize = 0;

		return vmaCreateAllocator(&vmaInfo, &memAllocator) == VK_SUCCESS;
	}

	bool VulkanAPI::isDeviceSuitable(const VkPhysicalDevice device)
	{
		QueueFamilyIndices indices = findQueueFamilies(device);

		bool extensionsSupported = checkDeviceExtensionSupport(device);

		bool swapChainAdequate = false;
		if (extensionsSupported) {
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		return indices.isComplete() && extensionsSupported && swapChainAdequate;
	}

	bool VulkanAPI::checkDeviceExtensionSupport(const VkPhysicalDevice device)
	{
		uint32_t extensionCount;

		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	QueueFamilyIndices VulkanAPI::findQueueFamilies(const VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;
		uint32_t queueFamilyCount = 0;
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT && vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport) == VK_SUCCESS && presentSupport) {
				indices.graphicsFamily = i;
				indices.presentFamily = i;
				break;
			}

			i++;
		}

		return indices;
	}

	SwapChainSupportDetails VulkanAPI::querySwapChainSupport(const VkPhysicalDevice device)
	{
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	VkPresentModeKHR VulkanAPI::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) 
	{
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D VulkanAPI::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != UINT32_MAX)
			return capabilities.currentExtent;
		else
		{
			int width, height;
			glfwGetFramebufferSize(pParentWindow, &width, &height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	VkSurfaceFormatKHR VulkanAPI::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkSampleCountFlagBits VulkanAPI::getMaxUsableSampleCount()
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

	bool VulkanAPI::createSwapChain()
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

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

		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0; // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
			return false;

		vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, swapChainImages.data());
		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;



		VkImageCreateInfo samplingImageInfo{};
		samplingImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		samplingImageInfo.imageType = VK_IMAGE_TYPE_2D;
		samplingImageInfo.mipLevels = 1;
		samplingImageInfo.extent = { extent.width, extent.height, 1 };
		samplingImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		samplingImageInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		samplingImageInfo.format = swapChainImageFormat;
		samplingImageInfo.samples = msaaSamples;
		samplingImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		samplingImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		samplingImageInfo.arrayLayers = 1;
		vkCreateImage(logicalDevice, &samplingImageInfo, nullptr, &samplingImage.allocatedImage);

		VkMemoryRequirements memRequirements;
		VkMemoryAllocateInfo memAllocInfo{};
		vkGetImageMemoryRequirements(logicalDevice, samplingImage.allocatedImage, &memRequirements);
		memAllocInfo.allocationSize = memRequirements.size;
		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		vmaFindMemoryTypeIndex(memAllocator, memRequirements.memoryTypeBits, &allocInfo, &memAllocInfo.memoryTypeIndex);
		vkAllocateMemory(logicalDevice, &memAllocInfo, nullptr, &colorImageMemory);
		vkBindImageMemory(logicalDevice, samplingImage.allocatedImage, colorImageMemory, 0);

		VkImageViewCreateInfo samplingViewInfo{};
		samplingViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		samplingViewInfo.image = samplingImage.allocatedImage;
		samplingViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		samplingViewInfo.format = swapChainImageFormat;
		samplingViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		samplingViewInfo.subresourceRange.baseMipLevel = 0;
		samplingViewInfo.subresourceRange.levelCount = 1;
		samplingViewInfo.subresourceRange.baseArrayLayer = 0;
		samplingViewInfo.subresourceRange.layerCount = 1;
		vkCreateImageView(logicalDevice, &samplingViewInfo, nullptr, &colorImageView);



		depthFormat = VK_FORMAT_D32_SFLOAT;

		VkImageCreateInfo depthImageCreateInfo{};
		depthImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		depthImageCreateInfo.pNext = nullptr;
		depthImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		depthImageCreateInfo.format = VK_FORMAT_D32_SFLOAT;
		depthImageCreateInfo.extent = { extent.width, extent.height, 1 };
		depthImageCreateInfo.mipLevels = 1;
		depthImageCreateInfo.arrayLayers = 1;
		depthImageCreateInfo.samples = msaaSamples;
		depthImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		depthImageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

		VmaAllocationCreateInfo imageAllocationCreateInfo{};
		imageAllocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		imageAllocationCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		imageAllocationCreateInfo.requiredFlags = 0; //CHECK THIS LATER

		VkResult res;
		res = vmaCreateImage(memAllocator, &depthImageCreateInfo, &imageAllocationCreateInfo, &depthImage.allocatedImage, &depthImage.allocation, nullptr);
		if (res != VK_SUCCESS)
			return false;

		VkImageViewCreateInfo depthImageViewCreateInfo{};
		depthImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		depthImageViewCreateInfo.pNext = nullptr;
		depthImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		depthImageViewCreateInfo.image = depthImage.allocatedImage;
		depthImageViewCreateInfo.format = VK_FORMAT_D32_SFLOAT;
		depthImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		depthImageViewCreateInfo.subresourceRange.levelCount = 1;
		depthImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		depthImageViewCreateInfo.subresourceRange.layerCount = 1;
		depthImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		vkCreateImageView(logicalDevice, &depthImageViewCreateInfo, nullptr, &depthImageView);

		return true;
	}

	bool VulkanAPI::createImageViews(VkFormat format, VkImageViewType type, VkImage image, VkImageView* target, int array_layers, int base_layer)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.viewType = type;
		createInfo.format = format;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = base_layer;
		createInfo.subresourceRange.layerCount = array_layers;
		createInfo.image = image;

		return vkCreateImageView(logicalDevice, &createInfo, nullptr, target) == VK_SUCCESS;
	}

	bool VulkanAPI::createRenderPass()
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapChainImageFormat;
		colorAttachment.samples = msaaSamples;
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
		depth_attachment.samples = msaaSamples;
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
		colorAttachmentResolve.format = swapChainImageFormat;
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

		if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
			return false;

		return true;
	}

	bool VulkanAPI::createFramebuffers()
	{
		swapChainFramebuffers.resize(swapChainImageViews.size());

		for (std::size_t i = 0; i < swapChainImageViews.size(); i++) {
			VkImageView attachments[] = {
				colorImageView, depthImageView, swapChainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 3;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
				return false;
		}

		return true;
	}

	bool VulkanAPI::createCommandPool()
	{
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
			return false;

		return true;
	}

	bool VulkanAPI::createCommandBuffers()
	{
		commandBuffers.resize(swapChainFramebuffers.size());
		return allocateCommandBuffer(commandBuffers.data(), commandBuffers.size());
	}

	bool VulkanAPI::createSemaphores()
	{
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
			vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
			vkCreateFence(logicalDevice, &fenceInfo, nullptr, &graphicsFence) != VK_SUCCESS)
			return false;

		return true;
	}

	void VulkanAPI::Update()
	{
		recreateSwapChain();
	}

	void VulkanAPI::recreateSwapChain()
	{
		Initialized == false; //Block rendering for a time it takes to recreate swapchain

		vkDeviceWaitIdle(logicalDevice);
		vkQueueWaitIdle(graphicsQueue);

		cleanupSwapChain();
		createSwapChain();
		
		swapChainImageViews.resize(swapChainImages.size());

		for (std::size_t i = 0; i < swapChainImages.size(); i++) {
			createImageViews(swapChainImageFormat, VK_IMAGE_VIEW_TYPE_2D, swapChainImages[i], &swapChainImageViews[i]);
		}

		createRenderPass();
		createFramebuffers();
		createCommandBuffers();

		Initialized = swapChainExtent.height != 0;
	}

	void VulkanAPI::cleanupSwapChain()
	{
		for (std::size_t i = 0; i < swapChainFramebuffers.size(); i++)
		{
			vkDestroyFramebuffer(logicalDevice, swapChainFramebuffers[i], nullptr);
		}

		vkFreeCommandBuffers(logicalDevice, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

		vkDestroyRenderPass(logicalDevice, renderPass, nullptr);

		for (std::size_t i = 0; i < swapChainImageViews.size(); i++)
		{
			vkDestroyImageView(logicalDevice, swapChainImageViews[i], nullptr);
		}

		if (colorImageView != nullptr)
		{
			vkDestroyImageView(logicalDevice, colorImageView, nullptr);
			vmaDestroyImage(memAllocator, samplingImage.allocatedImage, samplingImage.allocation);
			vkFreeMemory(logicalDevice, colorImageMemory, nullptr);
		}

		if (depthImageView != nullptr)
		{
			vkDestroyImageView(logicalDevice, depthImageView, nullptr);
			vmaDestroyImage(memAllocator, depthImage.allocatedImage, depthImage.allocation);
		}
		vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);

		depthImageView = nullptr;
		swapChain = nullptr;
		swapChainImageViews.clear();
		commandBuffers.clear();
	}

	void VulkanAPI::clearDrawables()
	{
		vkDeviceWaitIdle(logicalDevice);
		vkQueueWaitIdle(graphicsQueue);

		for (auto object : entities)
		{
			if (object.second->GetEntityType() == "VulkanDrawable")
			{
				dynamic_cast<VulkanDrawable*>(object.second)->destroyObject(logicalDevice, memAllocator);
				delete object.second;
			}
		}
		
		entities.clear();
		Logger::Out("The scene was cleared", OutputColor::Green, OutputType::Log);
	}

	bool VulkanAPI::loadModel(const char* mesh_path, std::vector<std::string> textures_vector, std::unordered_map<std::string, std::string>* out_materials_names)
	{
		Initialized = false;
		auto start = std::chrono::steady_clock::now();
		VulkanDrawable* ref_obj;
		if (entities.size() > 0 && selected_entity != 0)
		{
			ref_obj = dynamic_cast<VulkanDrawable*>(entities[selected_entity]);
			ref_obj->object_mesh.indices = {};
			ref_obj->object_mesh.vertices = {};
		}
		else
		{
			return false;
		}

		VulkanAPI* inst = this;
		std::vector<std::string> materials_collection;
		std::vector<std::string>* out_materials_collection = &materials_collection;
		std::map<int, std::future<void>> processes_map;
		std::map<std::string, std::vector<int>> materials_map;

		processes_map[processes_map.size()] = std::async(std::launch::async, [textures_vector, ref_obj, inst]()
			{
				inst->loadTexture(textures_vector, ref_obj, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_TYPE_2D);
			});

		processes_map[processes_map.size()] = std::async(std::launch::async, [mesh_path, textures_vector, ref_obj, out_materials_collection, inst]()
			{
				inst->loadMesh(mesh_path, ref_obj, textures_vector.size() != 0, out_materials_collection);
			});

		for (int ind = 0; ind < processes_map.size(); ind++)
		{
			if (processes_map[ind].valid())
			{
				processes_map[ind].wait();
			}
		}

		ref_obj->updateObject(logicalDevice, memAllocator);

		auto end = std::chrono::steady_clock::now();
		auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

		if (out_materials_names)
		{
			for (int ind = 0; ind < materials_collection.size(); ind++)
			{
				if (ind < textures_vector.size())
					out_materials_names->insert_or_assign(materials_collection[ind], textures_vector[ind]);
				else
					out_materials_names->insert_or_assign(materials_collection[ind], "nil");
			}
		}

		Logger::Out("Model %s loaded in %d ms", OutputColor::Gray, OutputType::Log, mesh_path, (int)time);
		Initialized = true;
		return true;
	}

	bool VulkanAPI::loadMesh(const char* mesh_path, VulkanDrawable* target, bool useTexturing, std::vector<std::string>* out_materials)
	{
		return target->LoadMesh(mesh_path, useTexturing, out_materials);
	}

	bool VulkanAPI::assignTextures(std::vector<std::string> textures, GrEngine::Entity* target)
	{
		loadTexture(textures, (VulkanDrawable*)target, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_TYPE_2D);
		dynamic_cast<VulkanDrawable*>(target)->updateObject(logicalDevice, memAllocator);
		return true;
	}

	bool VulkanAPI::loadTexture(std::vector<std::string> texture_path, VulkanDrawable* target, VkImageViewType type_view, VkImageType type_img)
	{
		if (texture_path.size() == 0)
		{
			return false;
		}
		
		ShaderBuffer stagingBuffer;
		Texture new_texture;

		int maxW = 0;
		int maxH = 0;
		int channels = 0;
		for (int i = 0; i < texture_path.size(); i++)
		{
			int width;
			int height;
			
			stbi_info(texture_path[i].c_str(), &width, &height, &channels);

			maxW = width > maxW || i == 0 ? width : maxW;
			maxH = height > maxH || i == 0 ? height : maxH;
		}
		
		std::map<int, std::future<void>> processes_map;
		unsigned char* data = (unsigned char*)malloc(maxW * maxH * channels * texture_path.size());

		//TBD: fix loading same image file on different surfaces
		for (int i = 0; i < texture_path.size(); i++)
		{
			processes_map[i] = std::async(std::launch::async, [texture_path, data, maxW, maxH, i]()
				{
					int width;
					int height;
					int channels;
					stbi_uc* pixels = stbi_load(texture_path[i].c_str(), &width, &height, &channels, STBI_rgb_alpha);

					if (!pixels)
					{
						Logger::Out("An error occurred while loading the texture: %s", OutputColor::Green, OutputType::Error, texture_path);
						free(data);
						return;
					}

					uint32_t buf_offset = maxW * maxH * channels * i;
					if (width < maxW || height < maxH)
					{
						auto output = (unsigned char*)malloc(maxW * maxH * channels);
						stbir_resize_uint8(pixels, width, height, 0, output, maxW, maxH, 0, channels);
						auto s = sizeof(pixels);
						memcpy(data + buf_offset, output, maxW * maxH * channels);
						stbi_image_free(pixels);
						free(output);
					}
					else
					{
						memcpy(data + buf_offset, pixels, width * height * channels);
						stbi_image_free(pixels);
					}
				});
		}

		for (int ind = 0; ind < processes_map.size(); ind++)
		{
			if (processes_map[ind].valid())
			{
				processes_map[ind].wait();
			}
		}

		m_createVkBuffer(logicalDevice, memAllocator, data, maxW * maxH * channels * texture_path.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &stagingBuffer);
		free(data);

		VkExtent3D imageExtent;
		imageExtent.width = static_cast<uint32_t>(maxW);
		imageExtent.height = static_cast<uint32_t>(maxH);
		imageExtent.depth = 1;

		VkImageCreateInfo dimg_info{};
		dimg_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		dimg_info.pNext = nullptr;
		dimg_info.imageType = type_img;
		dimg_info.format = VK_FORMAT_R8G8B8A8_SRGB;
		dimg_info.extent = imageExtent;
		dimg_info.mipLevels = 1;
		dimg_info.arrayLayers = 6;
		dimg_info.samples = VK_SAMPLE_COUNT_1_BIT;
		dimg_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		dimg_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		dimg_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

		VmaAllocationCreateInfo dimg_allocinfo = {};
		dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		//allocate and create the image
		vmaCreateImage(memAllocator, &dimg_info, &dimg_allocinfo, &new_texture.newImage.allocatedImage, &new_texture.newImage.allocation, nullptr);
		//vkCreateImage(logicalDevice, &dimg_info, nullptr, &new_texture.newImage.allocatedImage);

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = 1;
		subresourceRange.layerCount = texture_path.size();

		transitionImageLayout(new_texture.newImage.allocatedImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);
		copyBufferToImage(stagingBuffer.Buffer, new_texture.newImage.allocatedImage, maxW, maxH, channels, texture_path.size());
		transitionImageLayout(new_texture.newImage.allocatedImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);
		createImageViews(VK_FORMAT_R8G8B8A8_SRGB, type_view, new_texture.newImage.allocatedImage, &new_texture.textureImageView, texture_path.size());
		m_destroyShaderBuffer(logicalDevice, memAllocator, &stagingBuffer);

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
		samplerInfo.maxLod = 0.0f;
		vkCreateSampler(logicalDevice, &samplerInfo, nullptr, &new_texture.textureSampler);

		if (target->object_texture.initialized == true)
		{
			target->invalidateTexture(logicalDevice, memAllocator); //Strip object of its previous texture
		}
		new_texture.texture_path = texture_path[0].c_str();
		new_texture.initialized = true;
		target->object_texture = new_texture;

		return true;
	}

	bool VulkanAPI::allocateCommandBuffer(VkCommandBuffer* cmd, uint32_t count)
	{
		VkCommandBufferAllocateInfo cmdInfo = {};
		cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdInfo.pNext = NULL;
		cmdInfo.commandPool = commandPool;
		cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdInfo.commandBufferCount = count == 0 ? (uint32_t)sizeof(&cmd) / sizeof(VkCommandBuffer) : count;

		return vkAllocateCommandBuffers(logicalDevice, &cmdInfo, cmd) == VK_SUCCESS;
	}

	bool VulkanAPI::beginCommandBuffer(VkCommandBuffer cmd, VkCommandBufferUsageFlags usage)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = usage;

		return vkBeginCommandBuffer(cmd, &beginInfo) == VK_SUCCESS;
	}

	bool VulkanAPI::freeCommandBuffer(VkCommandBuffer commandBuffer)
	{
		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) return false;

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkResetFences(logicalDevice, 1, &graphicsFence);
		vkQueueSubmit(graphicsQueue, 1, &submitInfo, graphicsFence);
		vkQueueWaitIdle(graphicsQueue);

		vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);

		return true;
	}

	void VulkanAPI::transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subres, VkCommandBuffer cmd)
	{
		VkCommandBuffer commandBuffer;
		if (cmd == nullptr)
		{
			allocateCommandBuffer(&commandBuffer);
			beginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		}
		else
		{
			commandBuffer = cmd;
		}

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = subres.aspectMask;
		barrier.subresourceRange.baseMipLevel = subres.baseMipLevel;
		barrier.subresourceRange.levelCount = subres.levelCount;
		barrier.subresourceRange.baseArrayLayer = subres.baseArrayLayer;
		barrier.subresourceRange.layerCount = subres.layerCount;

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
		else {
			throw std::invalid_argument("unsupported layout transition!");
		}

		vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		if (cmd == nullptr)
			freeCommandBuffer(commandBuffer);
	}

	void VulkanAPI::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t channels, uint32_t length)
	{
		VkCommandBuffer commandBuffer;
		allocateCommandBuffer(&commandBuffer);
		beginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
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
				width,
				height,
				1
			};

			regions.push_back(region);
			offset += width * height * channels;
		}


		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regions.size(), regions.data());

		freeCommandBuffer(commandBuffer);
	}

	void VulkanAPI::copyImageToBuffer(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t channels, uint32_t length)
	{
		VkCommandBuffer commandBuffer;
		allocateCommandBuffer(&commandBuffer);
		beginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
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
				width,
				height,
				1
			};

			regions.push_back(region);
			offset += width * height * channels;
		}


		vkCmdCopyImageToBuffer(commandBuffer,  image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, regions.size(), regions.data());

		freeCommandBuffer(commandBuffer);
	}
#pragma region Static functions

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
		if (shader->initialized != false)
		{
			vkDestroyBuffer(device, shader->Buffer, NULL);
			//vmaUnmapMemory(allocator, shader->Allocation);
			vmaFreeMemory(allocator, shader->Allocation);
			vmaFlushAllocation(allocator, shader->Allocation, 0, shader->MappedMemoryRange.size);
			//vkFlushMappedMemoryRanges(device, 1, &(shader->MappedMemoryRange));
		}

		shader->initialized = false;
	}

	void VulkanAPI::m_destroyTexture(VkDevice device, VmaAllocator allocator, Texture* texture)
	{
		static bool is_in_use;
		while (is_in_use) {};
		is_in_use = true;

		if (texture->initialized == true)
		{
			vkDestroySampler(device, texture->textureSampler, NULL);
			vkDestroyImageView(device, texture->textureImageView, NULL);
			vmaDestroyImage(allocator, texture->newImage.allocatedImage, texture->newImage.allocation);
			vmaFlushAllocation(allocator, texture->newImage.allocation, 0, sizeof(texture->newImage.allocation));
			texture->initialized = false;
			texture->texture_path = NULL;
		}

		is_in_use = false;
	}

#pragma endregion

	GrEngine::EntityInfo VulkanAPI::addEntity()
	{
		GrEngine::Entity* ent = new VulkanDrawable();
		dynamic_cast<VulkanDrawable*>(ent)->initObject(logicalDevice, memAllocator, this);
		std::string new_name = std::string("Entity") + std::to_string(entities.size()+1);
		ent->UpdateNameTag(new_name.c_str());
		entities[ent->GetEntityInfo().EntityID] = ent;

		return ent->GetEntityInfo();
	}

	GrEngine::Entity* VulkanAPI::selectEntity(UINT ID)
	{
		if (auto search = entities.find(ID); search != entities.end())
		{
			selected_entity = ID;
			std::vector<std::any> para = { selected_entity };
			EventListener::registerEvent(EventType::SelectionChanged, para);
			VulkanDrawable::opo.selected_entity = { ID / 1000000 % 1000, ID / 1000 % 1000, ID % 1000 };
			return entities.at(ID);
		}

		VulkanDrawable::opo.selected_entity = { 0, 0, 0 };
		return nullptr;
	}

	void VulkanAPI::SetHighlightingMode(bool enabled)
	{
		VulkanDrawable::opo.highlight_enabled = enabled;
	}
}