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
		std::string getExecutablePath();
		inline AppWindow* getAppWindow() { return pWindow.get(); };

	protected:
		bool loadImageFromPath(const char* path, int material_index = 0);
		bool loadModel(const char* mesh_path, std::vector<std::string> textures_vector, std::string* out_materials = nullptr);
		void clearScene();
		void Run();
		void Stop();
		void TerminateLiraries();
		inline void* getNativeWindow() { return pWindow->getNativeWindow(); };
	private:
		std::unique_ptr<AppWindow> pWindow;
	};
}
