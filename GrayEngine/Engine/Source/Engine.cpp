#include <pch.h>
#include "Engine.h"

namespace GrEngine
{
	Engine::Engine()
	{
		pWindow = std::unique_ptr<Window>(Window::Init());
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

		pWindow.get()->~Window();
	}

	void Engine::PokeIt()
	{
		Logger::Out("You've just poked an engine", OutputColor::Gray);
	}
}
