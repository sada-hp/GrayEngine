#pragma once
#include <pch.h>
#include "Engine.h"

namespace GrEngine
{
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
			pWindow.get()->OnStep();
		}

		pWindow.get()->~AppWindow();
	}

	void Engine::Stop()
	{
		glfwSetWindowShouldClose(pWindow.get()->getWindow(), true);
	}

	void Engine::loadMeshFromPath(const char* path)
	{
		pWindow.get()->getRenderer()->loadModel(path);
	}

	void Engine::loadImageFromPath(const char* path)
	{
		pWindow.get()->getRenderer()->loadImage(path);
	}

	void Engine::clearScene()
	{
		pWindow.get()->getRenderer()->clearDrawables();
	}

	void Engine::TerminateLiraries()
	{
		glfwTerminate();
	}

	void Engine::PokeIt()
	{
		Logger::Out("You've just poked an engine", OutputColor::Gray, OutputType::Log);
	}
}
