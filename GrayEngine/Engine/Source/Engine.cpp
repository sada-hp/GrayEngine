#pragma once
#include <pch.h>
#include "Engine.h"

namespace GrEngine
{
	Engine* Engine::context = nullptr;

	Engine::Engine(const AppParameters& Properties)
	{
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

	bool Engine::LoadObject(const char* mesh_path, std::vector<std::string> textures_vector, std::unordered_map<std::string, std::string>* out_materials)
	{
		Pause();

		bool res = pWindow->getRenderer()->loadModel(mesh_path, textures_vector, out_materials);

		Unpause();
		return res;
	}

	bool Engine::LoadFromGMF(const char* filepath, std::unordered_map<std::string, std::string>* out_materials)
	{
		std::string mesh_path = "";
		std::vector<std::string> mat_vector;

		Globals::readGMF(filepath, &mesh_path, &mat_vector);
		Pause();

		bool res = LoadObject(mesh_path.c_str(), mat_vector, out_materials);

		Unpause();
		return res;
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

	EntityInfo Engine::AddEntity()
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
}
