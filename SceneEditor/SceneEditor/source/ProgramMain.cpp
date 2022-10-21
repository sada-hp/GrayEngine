#include "ProgramMain.h"

GrEngine::Application* GrEngine::Application::_instance = nullptr;
GrEngine::ModelBrowser* GrEngine::ModelBrowser::_instance = nullptr;

int SceneEditor::EntryPoint()
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

        return 10;
    }
    catch (const char* msg)
    {
        Logger::Out(msg, OutputColor::Red, OutputType::Error);
        delete app;

        return 1;
    }
}