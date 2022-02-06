#pragma once
#include <pch.h>
#include "Headers/Logger.h"
#include "Headers/AppWindow.h"

namespace GrEngine
{
	class _declspec(dllexport) Engine
	{
	public:
		const char* Name = "GrayEngineApp";
		Engine();
		virtual ~Engine();

		void Run();
		static std::vector<char> readFile(const std::string& filename);

		void PokeIt();
	protected:
		std::unique_ptr<Window> pWindow;
	};

	Engine* Init();
}
