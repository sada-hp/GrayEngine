#pragma once
#include <glfw/glfw3.h>
#include "Events/EventListener.h"


namespace GrEngine
{
	struct _declspec(dllexport) AppParameters
	{
		const char* Title;

		uint32_t Width;
		uint32_t Height;
		EventListener* pEventListener = nullptr;

		AppParameters(const char* _Title = "Application", uint32_t _Width = 1280, uint32_t _Height = 720)
		{
			Title = _Title;
			Width = _Width;
			Height = _Height;
		}
	};

	class _declspec(dllexport) Window
	{
	public:
		virtual ~Window() {};

		virtual void OnStep() = 0;

		virtual void SetVSync(bool VsyncState) = 0;

		virtual AppParameters* WindowProperties() = 0;

		static Window* Init(const AppParameters& Properties = AppParameters());

		inline GLFWwindow* getWindow() { return window; };

	protected:
		GLFWwindow* window;

	};
}