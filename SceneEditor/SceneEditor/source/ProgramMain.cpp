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
    Logger::Out("--------------- Starting the engine ---------------", OutputColor::Gray, OutputType::Log);
    Logger::ShowConsole(false);
    GrEngine::Application* app = new GrEngine::Application();

    try
    {
        app->getEditorUI()->InitUI(GrEngine::Application::HostWindowProc, MAIN_WIN_CLASSNAME, VIEWPORT_EDITOR);
        app->getEditorUI()->SetViewportHWND(app->getViewportHWND(), VIEWPORT_EDITOR);

        app->initAppLogger();
        app->StartEngine();
        delete app;

        return 0;
    }
    catch (const char* msg)
    {
        Logger::Out(msg, OutputColor::Red, OutputType::Error);
        delete app;

        return 1;
    }
}