#define MAIN_WIN_CLASSNAME "CppMAppHostWinClass"
#define MODEL_BROWSER_CLASSNAME "CppModelBrowserHostWin"

#pragma once
#include "EditorUI.h"
#include "Application.h"
#include "ModelBrowser.h"

GrEngine::Application* GrEngine::Application::_instance = nullptr;
GrEngine::ModelBrowser* GrEngine::ModelBrowser::_instance = nullptr;

int main(int argc, char** argv)
{
    Logger::ShowConsole(false);
    Logger::Out("--------------- Starting the engine ---------------", OutputColor::Gray, OutputType::Log);

    GrEngine::Application* app = new GrEngine::Application();

    app->getEditorUI()->InitUI(GrEngine::Application::HostWindowProc, MAIN_WIN_CLASSNAME, VIEWPORT_EDITOR);
    app->getEditorUI()->SetViewportHWND(app->getViewportHWND(), VIEWPORT_EDITOR);
	
	app->initAppLogger();
    app->StartEngine();
    delete app;

    return 0;
}