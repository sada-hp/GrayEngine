#pragma once
#include <glfw/glfw3.h>
#include "EventListener.h"

namespace GrEngine
{
	struct _declspec(dllexport) AppParameters
	{
		const char* Title;

		uint32_t Width;
		uint32_t Height;

		AppParameters(const char* _Title = "Application", uint32_t _Width = 1280, uint32_t _Height = 720)
		{
			Title = _Title;
			Width = _Width;
			Height = _Height;
		}
	};

	class _declspec(dllexport) AppWindow
	{
	public:
		virtual ~AppWindow() {};

		virtual void OnStep() = 0;

		virtual void SetVSync(bool VsyncState) = 0;

		virtual AppParameters* WindowProperties() = 0;

		static AppWindow* Init(const AppParameters& Properties = AppParameters());

		inline GLFWwindow* getWindow() { return window; };

		inline void* getNativePlatformWND() { return nativeWindow; };

		virtual void MaximizeGLFW(bool state) = 0;

		virtual void MinimizeGLFW(bool state) = 0;

	protected:
		GLFWwindow* window;
		void* nativeWindow;
	};
}