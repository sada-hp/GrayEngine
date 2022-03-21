#pragma once
#include <pch.h>
#include "Engine.h"
#include "Headers/Vulkan/VulkanAPI.h"
#include "Headers/Logger.h"

namespace GrEngine
{
	Engine::Engine()
	{
		pWindow = std::unique_ptr<AppWindow>(AppWindow::Init());
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

			EventListener::GetListener()->registerEvent(EventType::Step, para);
		}

		pWindow.get()->~AppWindow();
	}

	void Engine::Stop()
	{
		glfwSetWindowShouldClose(pWindow.get()->getWindow(), true);
	}

	void Engine::loadModelFromPath(const char* path)
	{
		VulkanAPI::m_getRenderer()->loadModel(path);
	}

	void Engine::clearScene()
	{
		VulkanAPI::m_getRenderer()->clearDrawables();
	}

	void Engine::PokeIt()
	{
		Logger::Out("You've just poked an engine", OutputColor::Gray, OutputType::Log);
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
