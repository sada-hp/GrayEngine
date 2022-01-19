#pragma once
#include <glfw/glfw3.h>
#include "Logger.h"
#include "AppWindow.h"

namespace GrEngine
{
	class _declspec(dllexport) Engine
	{
	public:
		const char* Name = "GrayEngineApp";
		Engine();
		virtual ~Engine();

		void Run();
		void PokeIt();
	private:
		std::unique_ptr<Window> pWindow;
	};

	Engine* Init();
}
