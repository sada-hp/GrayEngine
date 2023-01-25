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
		Entity* AddEntity();
		void LoadSkybox(const char* East, const char* West, const char* Top, const char* Bottom, const char* North, const char* South);
		bool LoadObject(UINT id, const char* mesh_path, std::vector<std::string> textures_vector);
		bool LoadFromGMF(UINT id, const char* filepath);
		UINT GetSelectedEntityID();
		bool AssignTextures(std::vector<std::string> textures, Entity* target);
		virtual void Run();
		virtual void Stop();
		virtual void Pause();
		virtual void Unpause();
		Renderer* GetRenderer() { return pWindow->getRenderer(); };
		Physics* GetPhysics() { return physEngine; };
		Entity* SelectEntity(UINT32 ID) { return GetRenderer()->selectEntity(ID); };
		POINTFLOAT GetCursorPosition();
		void SetCursorState(bool show) { pWindow->AppShowCursor(show); };
		void AddInputCallback(InputCallbackFun callback) { pWindow->AddInputProccess(callback); };
		bool IsKeyDown(int key) { return pWindow->IsKeyDown(key); }
		void TogglePhysicsState(bool state);
		POINT GetWindowSize();
		POINT GetWindowPosition();
		void SetCursorShape(int shape);
		void SetCursorPosition(double xpos, double ypos);

		static void BindContext(Engine* new_context) { context = new_context; };
		static Engine* GetContext() { return context; };
		static bool WriteGMF(const char* filepath, const char* mesh_path, std::vector<std::string> textures_vector);
		virtual void LoadScene(const char* path);
		virtual void SaveScene(const char* path);

	protected:
		void clearScene();
		void TerminateLiraries();
		inline void* getNativeWindow() { return pWindow->getNativeWindow(); };

	private:
		std::unique_ptr<AppWindow> pWindow;
		bool isPaused = false;
		static Engine* context;
		Physics* physEngine;
	};
}
