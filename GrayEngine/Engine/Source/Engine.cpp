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

	std::vector<char> Engine::readFile(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open())
			throw std::runtime_error("Failed to open file!");

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		return buffer;
	}
}
