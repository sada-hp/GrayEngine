#pragma once
#include <pch.h>

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;

	bool isComplete() {
		return graphicsFamily.has_value();
	}
};

class VulkanAPI
{
public:
	void initVulkan(GLFWwindow* window);
	void destroy();
private:
	VkInstance _vulkan;
	VkDevice device;
	VkSurfaceKHR surface;
	VkPhysicalDevice physicalDevice = nullptr;

	bool getVKInstance();
	bool createLogicalDevice();
	bool isDeviceSuitable(VkPhysicalDevice device);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
};

