#pragma once
#include "Virtual/AppWindow.h"
#include "Core/EventListener.h"
#include "Vulkan/VulkanRenderer.h"
#include "Bullet/BulletAPI.h"

namespace GrEngine
{
	enum class MouseButtons
	{
		Left = 0,
		Right = 1,
		Middle = 2
	};

	class GL_APP : public AppWindow
	{
	public:

		GL_APP(const AppParameters& Properties);
		~GL_APP();

		void OnStep() override;
		void SetVSync(bool state) override;
		void MaximizeWindow(bool state) override;
		void MinimizeWindow(bool state) override;
		void ProccessInputs() override;
		bool IsKeyDown(int KEY) override;
		void AppShowCursor(bool show) override;
		void Focus() override;
		void AllowResize(bool allow) override;
		void ShowBorder(bool show) override;

		inline AppParameters* WindowProperties() override { return &props; };
	private:
		Renderer* pAppRenderer;
		void StartUp(const AppParameters& Properties);
		void ShutDown();
		void SetUpEvents(GLFWwindow* target);

		unsigned long frames = 0;
		double time;
	};
}
