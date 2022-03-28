#pragma once
#include "Engine/Source/Headers/Core.h"
#include "Vulkan/VulkanAPI.h"
#include "Headers/AppWindow.h"

namespace GrEngine
{
	class DllExport Engine
	{
	public:
		Engine(const AppParameters& Properties = AppParameters());
		virtual ~Engine();
		static void PokeIt();
		inline AppWindow* getAppWindow() { return pWindow.get(); };

	protected:
		std::unique_ptr<AppWindow> pWindow;
		void loadModelFromPath(const char* path);
		void clearScene();
		void Run();
		void Stop();
		void* getWndNative() { return pWindow.get()->getNativePlatformWND(); };
	};
}
