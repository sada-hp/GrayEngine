#pragma once
#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>
#include <vector>

class VulkanAPI
{
public:
	void initVulkan();
	void destroy();
private:
	VkInstance _vulkan;

	bool getVKInstance();
	bool isDeviceSuitable(VkPhysicalDevice device);
};

