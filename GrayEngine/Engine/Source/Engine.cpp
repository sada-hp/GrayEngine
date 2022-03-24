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

			std::vector<double> para{};

			EventListener::registerEvent(EventType::Step, para);
		}

		pWindow.get()->~AppWindow();
	}

	void Engine::Stop()
	{
		glfwSetWindowShouldClose(pWindow.get()->getWindow(), true);
	}

	void Engine::loadModelFromPath(const char* path)
	{
		GrEngine_Vulkan::VulkanAPI::m_getRenderer()->loadModel(path);
	}

	void Engine::clearScene()
	{
		GrEngine_Vulkan::VulkanAPI::m_getRenderer()->clearDrawables();
	}

	void Engine::PokeIt()
	{
		Logger::Out("You've just poked an engine", OutputColor::Gray, OutputType::Log);
	}
}
