#pragma once
#include <pch.h>
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
		void Stop();
		static std::vector<char> readFile(const std::string& filename);

		void PokeIt();
		void loadModelFromPath(const char* path);
		void clearScene();

		void* getWndNative() { return pWindow.get()->getNativePlatformWND(); };

		inline AppWindow* getAppWindow() { return pWindow.get(); };
		
	protected:
		std::unique_ptr<AppWindow> pWindow;
	};

}
