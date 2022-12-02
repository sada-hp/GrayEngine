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
		bool createModel(const char* filepath, const char* mesh_path, std::vector<std::string> textures_vector);
		bool loadModel(const char* mesh_path, std::vector<std::string> textures_vector, std::unordered_map<std::string, std::string>* out_materials = nullptr);
		void clearScene();
		void Run();
		void Stop();
		void TerminateLiraries();
		inline void* getNativeWindow() { return pWindow->getNativeWindow(); };
		void addDummy(EntityInfo* out_entity = nullptr);
		void Pause();
		void Unpause();
		void loadSkybox(const char* East, const char* West, const char* Top, const char* Bottom, const char* North, const char* South);
	private:
		std::unique_ptr<AppWindow> pWindow;
		bool isPaused = false;
	};
}
