#pragma once
#include <pch.h>
#include "Engine.h"
#include "GL_APP.h"

namespace GrEngine
{
	Engine* Engine::context = nullptr;

	Engine::Engine(const AppParameters& Properties)
	{
		if (context == nullptr)
		{
			context = this;
			Logger::JoinEventListener(&eventListener);
		}

		Logger::Out("--------------- Starting the engine ---------------", OutputType::Log);
		physEngine = new GrEngineBullet::BulletAPI();
		AppParameters param = Properties;
		param.eventListener = &eventListener;
		pWindow = new GL_APP(param);
	}

	Engine::~Engine()
	{
		delete pWindow;
		delete physEngine;

		if (context == this)
		{
			context = nullptr;
		}
	}

	void Engine::Run()
	{
		while (!glfwWindowShouldClose(pWindow->getWindow()))
		{
			if (!isPaused)
				pWindow->OnStep();
		}

		physEngine->CleanUp();
	}

	void Engine::Stop()
	{
		eventListener.clearEventQueue();
		eventListener.setEventsPermissions(false, false);

		glfwSetWindowShouldClose(pWindow->getWindow(), true);
	}

	bool Engine::LoadObject(UINT id, const char* mesh_path, std::vector<std::string> textures_vector)
	{
		bool res = false;
		Entity* target = pWindow->getRenderer()->selectEntity(id);
		Object* drawComponent = (Object*)target->GetPropertyValue(PropertyType::Drawable, (void*)nullptr);

		if (drawComponent != nullptr)
		{
			res = drawComponent->LoadModel(mesh_path, textures_vector, {});
		}

		return res;
	}

	bool Engine::LoadFromGMF(UINT id, const char* gmfpath)
	{
		auto start = std::chrono::steady_clock::now();

		std::string mesh_path = "";
		std::string coll_path = "";
		std::vector<std::string> textures_vector;
		std::vector<std::string> normals_vector;

		if (!GrEngine::Globals::readGMF(gmfpath, &mesh_path, &coll_path, &textures_vector, &normals_vector))
			return false;

		Entity* target = pWindow->getRenderer()->GetEntitiesList()[id];
		Object* drawComponent = (Object*)target->GetPropertyValue(PropertyType::Drawable, (void*)nullptr);
		PhysicsObject* physComponent = (PhysicsObject*)target->GetPropertyValue(PropertyType::PhysComponent, (void*)nullptr);

		if (drawComponent != nullptr)
		{
			drawComponent->LoadModel(mesh_path.c_str(), textures_vector, normals_vector);
		}

		if (physComponent != nullptr)
		{
			physComponent->LoadCollisionMesh(coll_path.c_str());
		}

		auto end = std::chrono::steady_clock::now();
		auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
		Logger::Out("Model %s loaded in %d ms", OutputType::Log, gmfpath, (int)time);

		return true;
	}

	bool Engine::LoadFromGMF(Object* drawable, const char* gmfpath)
	{
		auto start = std::chrono::steady_clock::now();

		std::string mesh_path = "";
		std::string coll_path = "";
		std::vector<std::string> textures_vector;
		std::vector<std::string> normals_vector;

		if (!GrEngine::Globals::readGMF(gmfpath, &mesh_path, &coll_path, &textures_vector, &normals_vector))
			return false;

		PhysicsObject* physComponent = nullptr;

		if (drawable != nullptr)
		{
			drawable->LoadModel(mesh_path.c_str(), textures_vector, normals_vector);
			physComponent = (PhysicsObject*)drawable->GetOwnerEntity()->GetPropertyValue(PropertyType::PhysComponent, (void*)nullptr);
		}

		if (physComponent != nullptr)
		{
			physComponent->LoadCollisionMesh(coll_path.c_str());
		}

		auto end = std::chrono::steady_clock::now();
		auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
		Logger::Out("Model %s loaded in %d ms", OutputType::Log, gmfpath, (int)time);

		return true;
	}

	std::vector<std::string> Engine::GetMaterialNames(const char* mesh_path)
	{
		return GetRenderer()->GetMaterialNames(mesh_path);
	}

	void Engine::DeleteEntity(UINT id)
	{
		physEngine->RemoveSimulationObject(id);
		GetRenderer()->DeleteEntity(id);
	}

	void Engine::TerminateLiraries()
	{
		glfwTerminate();
	}

	bool Engine::PokeIt()
	{
		Logger::Out("You've just poked an engine", OutputType::Log);
		return true;
	}

	bool Engine::WriteGMF(const char* filepath, const char* mesh_path, const char* collision_path, std::vector<std::string> textures_vector)
	{
		return Globals::writeGMF(filepath, mesh_path, collision_path, textures_vector);
	}

	Entity* Engine::AddEntity()
	{
		return pWindow->getRenderer()->addEntity();
	}

	std::string Engine::getExecutablePath()
	{
		return Globals::getExecutablePath();
	}

	void Engine::Pause()
	{
		GetRenderer()->waitForRenderer();
		isPaused = true;
	}

	void Engine::Unpause()
	{
		isPaused = false;
	}

	void Engine::LoadSkybox(const char* East, const char* West, const char* Top, const char* Bottom, const char* North, const char* South)
	{
		Pause();

		pWindow->getRenderer()->createSkybox(East, West, Top, Bottom, North, South);

		Unpause();
	}

	void Engine::TogglePhysicsState(bool state)
	{
		physEngine->TogglePhysicsState(state);
	}

	void Engine::SetVSync(bool state)
	{
		pWindow->SetVSync(state);
	}

	UINT Engine::GetSelectedEntityID()
	{
		return GetRenderer()->GetSelectedEntity()->GetEntityID();
	}

	void Engine::LoadScene(const char* path)
	{
		auto start = std::chrono::steady_clock::now();

		Pause();
		physEngine->CleanUp();
		GetContext()->GetRenderer()->LoadScene(path);
		Unpause();
		eventListener.clearEventQueue();

		auto time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
		Logger::Out("Level %s loaded in %d ms", OutputType::Log, path, (int)time);
	}

	void Engine::ClearScene()
	{
		SelectEntity(0);
		physEngine->CleanUp();
		pWindow->getRenderer()->clearDrawables();
	}

	void Engine::SaveScene(const char* path)
	{
		auto start = std::chrono::steady_clock::now();

		GetRenderer()->SaveScene(path);

		auto time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
		Logger::Out("Level %s saved in %d ms", OutputType::Log, path, (int)time);
	}

	POINTFLOAT Engine::GetCursorPosition()
	{
		POINTFLOAT res;
		double xpos, ypos;
		glfwGetCursorPos(pWindow->getWindow(), &xpos, &ypos);
		res = { (float)xpos, (float)ypos };

		return res;
	}

	POINT Engine::GetWindowSize()
	{
		int w, h;
		glfwGetWindowSize(pWindow->getWindow(), &w, &h);
		return { w, h };
	}

	POINT Engine::GetWindowPosition()
	{
		int x, y;
		glfwGetWindowPos(pWindow->getWindow(), &x, &y);
		return { x, y };
	}

	void Engine::SetCursorShape(int shape)
	{
		GLFWcursor* cur = glfwCreateStandardCursor(shape);
		glfwSetCursor(pWindow->getWindow(), cur);
	}

	void Engine::SetCursorPosition(double xpos, double ypos)
	{
		glfwFocusWindow(pWindow->getWindow());
		glfwSetCursorPos(pWindow->getWindow(), xpos, ypos);
	}

	void Engine::GenerateTerrain(int resolution, int width, int height, int depth, std::array<std::string, 6> maps, std::array<std::string, 4> normals, std::array<std::string, 4> displacement)
	{
		GetRenderer()->LoadTerrain(resolution, width, height, depth, maps, normals, displacement);
	}
}
