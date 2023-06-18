#pragma once
#include "Core/Core.h"
#include "Virtual/AppWindow.h"
#include "Bullet/BulletAPI.h"

namespace GrEngine
{
	class DllExport Engine
	{
	public:
		Engine(const AppParameters& Properties);
		virtual ~Engine();
		static bool PokeIt();
		std::string getExecutablePath();
		virtual Entity* AddEntity();
		virtual void LoadSkybox(const char* East, const char* West, const char* Top, const char* Bottom, const char* North, const char* South);
		virtual bool LoadObject(UINT id, const char* mesh_path, std::vector<std::string> textures_vector);
		virtual bool LoadFromGMF(UINT id, const char* gmfpath);
		virtual bool LoadFromGMF(Object* drawable, const char* gmfpath);
		virtual UINT GetSelectedEntityID();
		virtual void Run();
		virtual void Stop();
		virtual void Pause();
		virtual void Unpause();
		Renderer* GetRenderer() { return pWindow->getRenderer(); };
		Physics* GetPhysics() { return physEngine; };
		Entity* SelectEntity(UINT32 ID) { return GetRenderer()->selectEntity(ID); };
		POINTFLOAT GetCursorPosition();
		virtual void SetCursorState(bool show) { pWindow->AppShowCursor(show); };
		virtual void AddInputCallback(UINT id, InputCallbackFun callback) { pWindow->AddInputProccess(id, callback); };
		void RemoveInputCallback(UINT id) { pWindow->RemoveInput(id); };
		void ClearInputCallbacks() { pWindow->ClearInputs(); };
		bool IsKeyDown(int key) { return pWindow->IsKeyDown(key); }
		void FocusWindow() { pWindow->Focus(); };
		virtual void TogglePhysicsState(bool state);
		POINT GetWindowSize();
		POINT GetWindowPosition();
		virtual void SetCursorShape(int shape);
		virtual void SetCursorPosition(double xpos, double ypos);
		inline EventListener* GetEventListener() { return &eventListener; };
		virtual void SetVSync(bool state);
		virtual std::vector<std::string> GetMaterialNames(const char* mesh_path);

		static void BindContext(Engine* new_context) { context = new_context; };
		static Engine* GetContext() { return context; };
		static bool WriteGMF(const char* filepath, const char* mesh_path, const char* collision_path, std::vector<std::string> textures_vector);
		virtual void LoadScene(const char* path);
		virtual void SaveScene(const char* path);
		virtual void GenerateTerrain(int resolution, int width, int height, int depth, std::array<std::string, 6> maps, std::array<std::string, 4> normals, std::array<std::string, 4> displacement);
		virtual void DeleteEntity(UINT id);
		virtual void ClearScene();

	protected:
		void TerminateLiraries();
		inline void* getNativeWindow() { return pWindow->getNativeWindow(); };
		AppWindow* GetWindowContext() { return pWindow; };

	private:
		AppWindow* pWindow;
		bool isPaused = false;
		static Engine* context;
		Physics* physEngine;
		EventListener eventListener;
	};
}
