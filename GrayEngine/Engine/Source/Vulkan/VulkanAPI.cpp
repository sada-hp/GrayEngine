#include <pch.h>
#define GLFW_INCLUDE_VULKAN
#define VMA_IMPLEMENTATION
#define VK_KHR_swapchain
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#define STB_IMAGE_IMPLEMENTATION
#include "VulkanAPI.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace GrEngine_Vulkan
{
	void VulkanAPI::destroy()
	{
		if (Initialized == false) return;
		Initialized = false;

		vkDeviceWaitIdle(logicalDevice);
		vkQueueWaitIdle(graphicsQueue);

		clearDrawables();
		cleanupSwapChain();
		vkDestroySemaphore(logicalDevice, renderFinishedSemaphore, nullptr);
		vkDestroySemaphore(logicalDevice, imageAvailableSemaphore, nullptr);
		vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
		vmaDestroyAllocator(memAllocator);
		vkDestroyDevice(logicalDevice, nullptr);
		vkDestroySurfaceKHR(_vulkan, surface, nullptr);
		vkDestroyInstance(_vulkan, nullptr);
	}

	bool VulkanAPI::init(GLFWwindow* window) //Vulkan integration done with a help of vulkan-tutorial.com
	{
		bool res = true;
		pParentWindow = window;

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

		if ((res = createImageViews() & res) == false)
			Logger::Out("[Vk] Failed to create image views", OutputColor::Red, OutputType::Error);

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

		vkDeviceWaitIdle(logicalDevice);

		return Initialized = res;
	}

	void VulkanAPI::drawFrame()
	{
		if (!Initialized) return;

		vkQueueWaitIdle(graphicsQueue); //TBD

		uint32_t imageIndex = 0;
		vkAcquireNextImageKHR(logicalDevice, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
		updateDrawables(imageIndex);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
		VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		VkResult res;
		res = vkQueueSubmit(graphicsQueue, 1, &submitInfo, graphicsFence);

		if (res == VK_ERROR_DEVICE_LOST)
		{
			//recreateSwapChain();
			Logger::Out("Logical device was lost!", OutputColor::Red, OutputType::Error);
			throw std::runtime_error("Logical device was lost!");
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr; // Optional

		vkQueuePresentKHR(presentQueue, &presentInfo);
	}

	bool VulkanAPI::updateDrawables(uint32_t index)
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
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffers[index], 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;
		vkCmdSetScissor(commandBuffers[index], 0, 1, &scissor);

		for (auto object : drawables)
		{
			object.recordCommandBuffer(logicalDevice, commandBuffers[index], swapChainExtent);
		}

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

		if (vkCreateInstance(&createInfo, nullptr, &_vulkan) != VK_SUCCESS)
			return false;
		return true;
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

	VkPresentModeKHR VulkanAPI::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
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

		VkExtent3D depthImageExtent = { extent.width, extent.height, 1 };

		depthFormat = VK_FORMAT_D32_SFLOAT;

		VkImageCreateInfo depthImageCreateInfo{};
		depthImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		depthImageCreateInfo.pNext = nullptr;
		depthImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		depthImageCreateInfo.format = depthFormat;
		depthImageCreateInfo.extent = depthImageExtent;
		depthImageCreateInfo.mipLevels = 1;
		depthImageCreateInfo.arrayLayers = 1;
		depthImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
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
		depthImageViewCreateInfo.format = depthFormat;
		depthImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		depthImageViewCreateInfo.subresourceRange.levelCount = 1;
		depthImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		depthImageViewCreateInfo.subresourceRange.layerCount = 1;
		depthImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		vkCreateImageView(logicalDevice, &depthImageViewCreateInfo, nullptr, &depthImageView);

		return true;
	}

	bool VulkanAPI::createImageViews()
	{
		swapChainImageViews.resize(swapChainImages.size());

		for (std::size_t i = 0; i < swapChainImages.size(); i++) {
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapChainImageFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;
			createInfo.image = swapChainImages[i];

			if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
				return false;
		}
		return true;
	}

	VkShaderModule VulkanAPI::createShaderModule(VkDevice device, const std::vector<char>& code)
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

	bool VulkanAPI::createRenderPass()
	{
		VkAttachmentDescription depth_attachment{};
		depth_attachment.flags = 0;
		depth_attachment.format = depthFormat;
		depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depth_attachment_ref{};
		depth_attachment_ref.attachment = 1;
		depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depth_attachment_ref;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkSubpassDependency depth_dependency = {};
		depth_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		depth_dependency.dstSubpass = 0;
		depth_dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		depth_dependency.srcAccessMask = 0;
		depth_dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		depth_dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		VkAttachmentDescription attachments[] = { colorAttachment, depth_attachment };
		VkSubpassDependency dependecies[] = { dependency, depth_dependency };

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 2;
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
				swapChainImageViews[i], depthImageView
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 2;
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
		poolInfo.flags = 0;

		if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
			return false;

		return true;
	}

	bool VulkanAPI::createCommandBuffers()
	{
		commandBuffers.resize(swapChainFramebuffers.size());

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

		if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
			return false;

		return true;
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

		cleanupSwapChain();
		createSwapChain();
		createImageViews();
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

		/*Fixes a crash when glfw window is minimized*/
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

		for (int ind = 0; ind < drawables.size(); ind++)
		{
			drawables[ind].destroyObject(logicalDevice, memAllocator);
		}

		drawables.clear();
		Logger::Out("The scene was cleared", OutputColor::Green, OutputType::Log);
	}

	bool VulkanAPI::loadModel(const char* mesh_path, std::vector<std::string> textures_vector, std::string* out_materials_names)
	{
		auto start = std::chrono::steady_clock::now();

		DrawableObj object;
		DrawableObj* ref_obj = &object;
		VulkanAPI* inst = this;
		std::map<int, std::future<void>> mat_map;
		std::map<std::string, int> proc_map;
		int mat_index = 0;

		for (auto texture : textures_vector)
		{
			if (mat_map[proc_map[texture]].valid()) //the file might be currently in use, so check for it
			{
				mat_map[proc_map[texture]].wait();
			}

			mat_map[mat_index] = std::async(std::launch::async, [texture, ref_obj, mat_index, inst]()
				{
					inst->loadTexture(texture.c_str(), ref_obj, mat_index);
				});

			mat_index++;
		}

		mat_map[mat_index] = std::async(std::launch::async, [mesh_path, ref_obj, out_materials_names, inst]()
			{
				inst->loadMesh(mesh_path, ref_obj, out_materials_names);
			});

		for (int ind = 0; ind < mat_map.size(); ind++)
		{
			if (mat_map[ind].valid())
			{
				mat_map[ind].wait();
			}
		}

		ref_obj->updateObject(logicalDevice);
		drawables.push_back(object);

		auto end = std::chrono::steady_clock::now();
		auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

		Logger::Out("Model %s loaded in %d ms", OutputColor::Gray, OutputType::Log, mesh_path, (int)time);

		return true;
	}

	bool VulkanAPI::loadMesh(const char* mesh_path, DrawableObj* target, std::string* out_materials)
	{
		std::unordered_map<Vertex, uint32_t> uniqueVertices{};
		Assimp::Importer importer;

		auto model = importer.ReadFile(mesh_path, 0);

		if (model == NULL)
		{
			Logger::Out("Could not load the mesh %c%s%c!", OutputColor::Red, OutputType::Error, '"', mesh_path, '"');
			return false;
		}
		else if (model->mNumMaterials > TEXTURE_ARRAY_SIZE)
		{
			Logger::Out("Max number of supported materials(%d) exceded in the mesh %c%s%c!", OutputColor::Red, OutputType::Error, TEXTURE_ARRAY_SIZE, '"', mesh_path, '"');
			return false;
		}
		float highest_pointx = 0.f;
		float highest_pointy = 0.f;
		float highest_pointz = 0.f;

		for (int mesh_ind = 0; mesh_ind < model->mNumMeshes; mesh_ind++)
		{
			auto num_vert = model->mMeshes[mesh_ind]->mNumVertices;
			auto cur_mesh = model->mMeshes[mesh_ind];
			auto name3 = model->mMeshes[mesh_ind]->mName;
			auto uv_ind = mesh_ind;
			for (int vert_ind = 0; vert_ind < num_vert; vert_ind++)
			{
				auto coord = model->mMeshes[mesh_ind]->mTextureCoords[0];

				Vertex vertex = { {cur_mesh->mVertices[vert_ind].x, cur_mesh->mVertices[vert_ind].y, cur_mesh->mVertices[vert_ind].z, 1.0f},
				{1.0f, 1.0f, 1.0f, 1.0f},
				{coord[vert_ind].x, 1.0f - coord[vert_ind].y},
				uv_ind };

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(target->object_mesh.vertices.size());
					target->object_mesh.vertices.push_back(vertex);
				}

				if (highest_pointx < cur_mesh->mVertices[vert_ind].x)
					highest_pointx = cur_mesh->mVertices[vert_ind].x;
				if (highest_pointy < cur_mesh->mVertices[vert_ind].y)
					highest_pointy = cur_mesh->mVertices[vert_ind].y;
				if (highest_pointz < cur_mesh->mVertices[vert_ind].z)
					highest_pointz = cur_mesh->mVertices[vert_ind].z;

				target->object_mesh.indices.push_back(uniqueVertices[vertex]);
			}

			aiString name;
			aiGetMaterialString(model->mMaterials[model->mMeshes[mesh_ind]->mMaterialIndex], AI_MATKEY_NAME, &name);

			if (out_materials)
			{
				out_materials->append(name.C_Str());
				out_materials->append("\\");
			}
		}
		target->bound = {highest_pointx, highest_pointy, highest_pointz};
		target->object_mesh.mesh_path = mesh_path;
		target->initObject(logicalDevice, memAllocator, this);

		Logger::Out("Mesh %c%s%c was loaded succesfully", OutputColor::Green, OutputType::Log, '"', mesh_path, '"');
		return true;
	}

	bool VulkanAPI::loadImage(const char* image_path, int material_index)
	{
		auto object = &drawables.back();

		loadTexture(image_path, object, material_index);
		object->updateObject(logicalDevice);

		return true;
	}

	bool VulkanAPI::loadTexture(const char* texture_path, DrawableObj* target, int material_index)
	{
		ShaderBuffer stagingBuffer;
		Texture new_texture;

		int texWidth, texHeight, texChannels;

		stbi_uc* pixels = stbi_load(texture_path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		if (!pixels) 
		{
			Logger::Out("An error occurred while loading the texture: %s", OutputColor::Green, OutputType::Error, texture_path);
			return false;
		}

		createVkBuffer(logicalDevice, memAllocator, pixels, texWidth * texHeight * 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &stagingBuffer);

		stbi_image_free(pixels);

		VkExtent3D imageExtent;
		imageExtent.width = static_cast<uint32_t>(texWidth);
		imageExtent.height = static_cast<uint32_t>(texHeight);
		imageExtent.depth = 1;

		VkImageCreateInfo dimg_info{};
		dimg_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		dimg_info.pNext = nullptr;
		dimg_info.imageType = VK_IMAGE_TYPE_2D;
		dimg_info.format = VK_FORMAT_R8G8B8A8_SRGB;
		dimg_info.extent = imageExtent;
		dimg_info.mipLevels = 1;
		dimg_info.arrayLayers = 1;
		dimg_info.samples = VK_SAMPLE_COUNT_1_BIT;
		dimg_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		dimg_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		VmaAllocationCreateInfo dimg_allocinfo = {};
		dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		//allocate and create the image
		vmaCreateImage(memAllocator, &dimg_info, &dimg_allocinfo, &new_texture.newImage.allocatedImage, &new_texture.newImage.allocation, nullptr);

		transitionImageLayout(new_texture.newImage.allocatedImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copyBufferToImage(stagingBuffer.Buffer, new_texture.newImage.allocatedImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
		transitionImageLayout(new_texture.newImage.allocatedImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		destroyShaderBuffer(logicalDevice, memAllocator, &stagingBuffer);

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = new_texture.newImage.allocatedImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;
		vkCreateImageView(logicalDevice, &viewInfo, nullptr, &new_texture.textureImageView);

		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
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

		if (target->object_texture.size() < TEXTURE_ARRAY_SIZE)
			target->object_texture.resize(TEXTURE_ARRAY_SIZE);

		new_texture.texture_path = texture_path;
		destroyTexture(logicalDevice, memAllocator, &target->object_texture[material_index]);
		target->object_texture[material_index] = new_texture;

		return true;
	}

	VkCommandBuffer VulkanAPI::beginSingleTimeCommands() 
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	void VulkanAPI::endSingleTimeCommands(VkCommandBuffer commandBuffer)
	{
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);

		vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
	}

	void VulkanAPI::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

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
		else {
			throw std::invalid_argument("unsupported layout transition!");
		}

		vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		endSingleTimeCommands(commandBuffer);
	}

	void VulkanAPI::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	{
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = {
			width,
			height,
			1
		};

		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		endSingleTimeCommands(commandBuffer);
	}

	bool VulkanAPI::createVkBuffer(VkDevice device, VmaAllocator allocator, const void* bufData, uint32_t dataSize, VkBufferUsageFlags usage, ShaderBuffer* shaderBuffer)
	{
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = dataSize;
		bufferCreateInfo.usage = usage;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bufferCreateInfo.queueFamilyIndexCount = 0;
		bufferCreateInfo.pQueueFamilyIndices = NULL;

		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

		vmaCreateBuffer(allocator, &bufferCreateInfo, &vmaallocInfo, &shaderBuffer->Buffer, &shaderBuffer->Allocation, nullptr);
		vkGetBufferMemoryRequirements(device, shaderBuffer->Buffer, &shaderBuffer->MemoryRequirements);

		vmaMapMemory(allocator, shaderBuffer->Allocation, (void**)&shaderBuffer->pData);
		memcpy(shaderBuffer->pData, bufData, dataSize);
		vmaUnmapMemory(allocator, shaderBuffer->Allocation);

		shaderBuffer->BufferInfo.buffer = shaderBuffer->Buffer;
		shaderBuffer->BufferInfo.offset = 0;
		shaderBuffer->BufferInfo.range = dataSize;

		shaderBuffer->MappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		shaderBuffer->MappedMemoryRange.memory = reinterpret_cast<VkDeviceMemory>(shaderBuffer->Allocation);
		shaderBuffer->MappedMemoryRange.offset = 0;
		shaderBuffer->MappedMemoryRange.size = dataSize;

		return true;
	}

	void VulkanAPI::destroyShaderBuffer(VkDevice device, VmaAllocator allocator, ShaderBuffer* shader)
	{
		vkFlushMappedMemoryRanges(device, 1, &(shader->MappedMemoryRange));
		vkDestroyBuffer(device, shader->Buffer, NULL);
		vmaFreeMemory(allocator, shader->Allocation);
	}

	void VulkanAPI::destroyTexture(VkDevice device, VmaAllocator allocator, Texture* texture)
	{
		vkDestroySampler(device, texture->textureSampler, NULL);
		vkDestroyImageView(device, texture->textureImageView, NULL);
		vmaDestroyImage(allocator, texture->newImage.allocatedImage, texture->newImage.allocation);
	}
}