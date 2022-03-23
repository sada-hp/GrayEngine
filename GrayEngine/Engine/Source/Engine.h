#pragma once
#include <pch.h>
#include "Headers/AppWindow.h"

namespace GrEngine
{
	class _declspec(dllexport) Engine
	{
	public:
		Engine();
		virtual ~Engine();
		static std::vector<char> readFile(const std::string& filename);
		static void PokeIt();
		inline AppWindow* getAppWindow() { return pWindow.get(); };

	protected:
		std::unique_ptr<AppWindow> pWindow;
		void loadModelFromPath(const char* path);
		void clearScene();
		void Run();
		void Stop();
		void* getWndNative() { return pWindow.get()->getNativePlatformWND(); };
	};
}
