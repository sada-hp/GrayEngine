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
		EntityInfo AddEntity();
		void LoadSkybox(const char* East, const char* West, const char* Top, const char* Bottom, const char* North, const char* South);
		bool LoadObject(const char* mesh_path, std::vector<std::string> textures_vector, std::unordered_map<std::string, std::string>* out_materials = nullptr);
		bool LoadFromGMF(const char* filepath, std::unordered_map<std::string, std::string>* out_materials = nullptr);
		bool AssignTextures(std::vector<std::string> textures, Entity* target);
		void Run();
		void Stop();
		void Pause();
		void Unpause();

		static void BindContext(Engine* new_context) { context = new_context; };
		static Engine* GetContext() { return context; };
		static bool WriteGMF(const char* filepath, const char* mesh_path, std::vector<std::string> textures_vector);

	protected:
		void clearScene();
		void TerminateLiraries();
		inline void* getNativeWindow() { return pWindow->getNativeWindow(); };

	private:
		std::unique_ptr<AppWindow> pWindow;
		bool isPaused = false;
		static Engine* context;
	};
}
