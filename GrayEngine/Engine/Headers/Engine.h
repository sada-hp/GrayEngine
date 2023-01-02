#pragma once
#include "Core/Core.h"
#include "Virtual/AppWindow.h"
#include "Bullet/BulletAPI.h"

namespace GrEngine
{
	class DllExport Engine
	{
	public:
		Engine(const AppParameters& Properties = AppParameters());
		virtual ~Engine();
		static bool PokeIt();
		std::string getExecutablePath();
		EntityInfo AddEntity();
		void LoadSkybox(const char* East, const char* West, const char* Top, const char* Bottom, const char* North, const char* South);
		bool LoadObject(const char* mesh_path, std::vector<std::string> textures_vector, std::unordered_map<std::string, std::string>* out_materials = nullptr);
		bool LoadFromGMF(const char* filepath, std::unordered_map<std::string, std::string>* out_materials = nullptr);
		bool AssignTextures(std::vector<std::string> textures, Entity* target);
		void Run();
		void Stop();
		void Pause();
		void Unpause();
		Renderer* GetRenderer() { return pWindow->getRenderer(); };
		Entity* SelectEntity(UINT32 ID) { return GetRenderer()->selectEntity(ID); };
		void SetCursorState(bool show) { pWindow->AppShowCursor(show); };
		void AddInputCallback(InputCallbackFun callback) { pWindow->AddInputProccess(callback); };
		bool IsKeyDown(int key) { return pWindow->IsKeyDown(key); }
		void TogglePhysicsState(bool state);

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
