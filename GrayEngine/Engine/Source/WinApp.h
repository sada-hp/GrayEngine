#pragma once
#include "Headers/AppWindow.h"
#include "Headers/EventListener.h"
#include "Vulkan/VulkanAPI.h"

namespace GrEngine
{
	enum class MouseButtons
	{
		Left = 0,
		Right = 1,
		Middle = 2
	};

	class WinApp : public AppWindow
	{
	public:

		WinApp(const AppParameters& Properties);
		~WinApp();

		void OnStep() override;
		void SetVSync(bool state) override;
		void MaximizeGLFW(bool state) override;
		void MinimizeGLFW(bool state) override;

		inline AppParameters* WindowProperties() override { return &props; };
	private:
		Renderer* p_AppRenderer;
		void StartUp(const AppParameters& Properties);
		void ShutDown();
		void SetUpEvents(GLFWwindow* target);
		AppParameters props;

		unsigned long frames = 0;
		double time;
	};
}
