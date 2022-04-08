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
		static bool PokeIt();
		inline AppWindow* getAppWindow() { return pWindow.get(); };

	protected:
		std::unique_ptr<AppWindow> pWindow;
		bool loadMeshFromPath(const char* path, std::string* out_materials = nullptr);
		bool loadImageFromPath(const char* path, int material_index = 0);
		void clearScene();
		void Run();
		void Stop();
		void TerminateLiraries();
		void* getWndNative() { return pWindow.get()->getNativePlatformWND(); };
	};
}
