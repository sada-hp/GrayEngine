#pragma once
#define GLFW_INCLUDE_VULKAN

#include <pch.h>
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
	private:
		VulkanAPI vkAPI;
		void StartUp(const AppParameters& Properties);
		void ShutDown();
		void SetUpEvents(GLFWwindow* target);
		AppParameters props;
	};
}
