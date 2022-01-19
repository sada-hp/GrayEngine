#pragma once
#define GLFW_INCLUDE_VULKAN

#include <vector>
#include <functional>
#include <glfw/glfw3.h>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp> 
#include "AppWindow.h"
#include "Logger.h"
#include "Events/EventListener.h"
#include "VulkanAPI.h"


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
		std::vector<std::function<bool(MouseButtons button, int action, int mods)>> MouseEvents;

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
