#pragma once
#include <pch.h>
#include "Engine.h"

namespace GrEngine
{
	std::array<short, 3> Entity::next_id = {5, 10, 25};
	Engine* Engine::context = nullptr;

	Engine::Engine(const AppParameters& Properties)
	{
		Physics::SetContext(new GrEngineBullet::BulletAPI());
		pWindow = std::unique_ptr<AppWindow>(AppWindow::Init(Properties));
	}

	Engine::~Engine()
	{

	}

	void Engine::Run()
	{
		while (!glfwWindowShouldClose(pWindow.get()->getWindow()))
		{
			if (!isPaused)
				pWindow->OnStep();
		}

		pWindow->~AppWindow();
	}

	void Engine::Stop()
	{
		glfwSetWindowShouldClose(pWindow.get()->getWindow(), true);
	}

	bool Engine::LoadObject(UINT id, const char* mesh_path, std::vector<std::string> textures_vector)
	{
		Pause();

		bool res = GetRenderer()->loadModel(id, mesh_path, textures_vector);

		Unpause();
		return res;
	}

	bool Engine::LoadFromGMF(UINT id, const char* filepath)
	{
		return GetRenderer()->loadModel(id, filepath);
	}

	bool Engine::AssignTextures(std::vector<std::string> textures, Entity* target)
	{
		Pause();

		bool res = pWindow->getRenderer()->assignTextures(textures, target);

		Unpause();
		return res;
	}

	void Engine::clearScene()
	{
		pWindow->getRenderer()->clearDrawables();
	}

	void Engine::TerminateLiraries()
	{
		glfwTerminate();
	}

	bool Engine::PokeIt()
	{
		Logger::Out("You've just poked an engine", OutputColor::Gray, OutputType::Log);
		return true;
	}

	bool Engine::WriteGMF(const char* filepath, const char* mesh_path, std::vector<std::string> textures_vector)
	{
		return Globals::writeGMF(filepath, mesh_path, textures_vector);
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
		Physics::GetContext()->TogglePhysicsState(state);
	}

	UINT Engine::GetSelectedEntityID()
	{
		return GetRenderer()->GetSelectedEntity()->GetEntityID();
	}
}
