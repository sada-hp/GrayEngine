#pragma once
#include <glfw/glfw3.h>
#include "Core/Core.h"
#include "Core/EventListener.h"
#include "Virtual/Renderer.h"

namespace GrEngine
{
	typedef void (*InputCallbackFun)();

	struct DllExport AppParameters
	{
		const char* Title;
		uint32_t Width;
		uint32_t Height;
		EventListener* eventListener;
		Renderer* p_Renderer;

		AppParameters(const char* _Title = "Application", uint32_t _Width = 1280, uint32_t _Height = 720, bool free_mode = false)
		{
			Title = _Title;
			Width = _Width;
			Height = _Height;
		}
	};

	class DllExport AppWindow
	{
	public:
		virtual ~AppWindow() {};

		virtual void OnStep() = 0;

		virtual void SetVSync(bool VsyncState) = 0;

		virtual void ProccessInputs() = 0;

		virtual AppParameters* WindowProperties() = 0;

		static AppWindow* Init(const AppParameters& Properties = AppParameters());

		inline GLFWwindow* getWindow() { return window; };

		inline Renderer* getRenderer() { return props.p_Renderer; };

		inline void* getNativeWindow() { return nativeWindow; };

		virtual void MaximizeWindow(bool state) = 0;

		virtual void MinimizeWindow(bool state) = 0;

		virtual bool IsKeyDown(int KEY) = 0;

		virtual void AppShowCursor(bool show) = 0;

		virtual void AllowResize(bool allow) = 0;

		virtual void ShowBorder(bool show) = 0;

		virtual void Focus() = 0;

		void AddInputProccess(UINT id, InputCallbackFun lambda)
		{
			inputs_vector[id] = lambda;
		}

		void RemoveInput(UINT id)
		{
			inputs_vector.erase(id);
		}

		void ClearInputs()
		{
			inputs_vector.clear();
		}

	protected:
		std::unordered_map<UINT, InputCallbackFun> inputs_vector;
		AppParameters props;
		GLFWwindow* window;
		void* nativeWindow;
	};
}