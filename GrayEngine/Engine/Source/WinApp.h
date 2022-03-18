#pragma once
#define VMA_IMPLEMENTATION
#define GLFW_INCLUDE_GLU

#define GLFW_EXPOSE_NATIVE_WGL
#define GLFW_EXPOSE_NATIVE_WIN32
#include <pch.h>
#include <glfw/glfw3native.h>
#include <glfw/glfw3.h>
#include "Headers/AppWindow.h"
#include "Headers/Logger.h"
#include "Headers/Events/EventListener.h"
#include "Headers/Vulkan/VulkanAPI.h"

namespace GrEngine
{
	enum class MouseButtons
	{
		Left = 0,
		Right = 1,
		Middle = 2
	};

	class WinApp : public Window
	{
	public:

		WinApp(const AppParameters& Properties);
		~WinApp();

		void OnStep() override;
		void SetVSync(bool state) override;

		inline AppParameters* WindowProperties() override { return &props; };
		inline VulkanAPI getVk() { return vkAPI; };
	private:
		VulkanAPI vkAPI;
		void StartUp(const AppParameters& Properties);
		void ShutDown();
		void SetUpEvents(GLFWwindow* target);
		AppParameters props;

		unsigned long frames = 0;
		double time;
	};
}
