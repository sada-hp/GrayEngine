#define MAIN_WIN_CLASSNAME "CppMAppHostWinClass"
#define MODEL_BROWSER_CLASSNAME "CppModelBrowserHostWin"

#pragma once
#include "EditorUI.h"
#include "Application.h"
#include "ModelBrowser.h"

GrEngine::Application* GrEngine::Application::_instance = nullptr;
GrEngine::ModelBrowser* GrEngine::ModelBrowser::_instance = nullptr;

LRESULT CALLBACK HostWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) //Background Win32 is used to receive messages from WPF front-end window
{
    switch (msg)
    {
    case WM_CLOSE:
        GrEngine::Application::KillEngine();
        GrEngine::Application::getEditorUI()->destroyUI(VIEWPORT_EDITOR);
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        break;
    case WM_SIZE:
        GrEngine::Application::updateWpfWnd();
        break;
    /*Messages received from the C# WPF front-end part of the editor*/
	case 0x1200: //Load obj file callback
        GrEngine::Application::loadModel((const char*)lParam);
		break;
	case 0x1201: //Clear viewport callback
        GrEngine::Application::clearViewport();
		break;
    case 0x1202: //Upload texture file callback
        GrEngine::Application::uploadTexture((const char*)lParam);
        break;
    case 0x1203: //Open the model browser
        GrEngine::Application::initModelBrowser();
        break;
    case 0x2200: //Model browser is closing
        GrEngine::ModelBrowser::loadModel((const char*)lParam);
        break;
    default:
        return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int main(int argc, char** argv)
{
    Logger::ShowConsole(false);
    Logger::Out("--------------- Starting the engine ---------------", OutputColor::Gray, OutputType::Log);

    GrEngine::Application* app = new GrEngine::Application();

    app->getEditorUI()->InitUI(HostWindowProc, MAIN_WIN_CLASSNAME, VIEWPORT_EDITOR);
    app->getEditorUI()->SetViewportHWND(app->getGLFW_HWND(), VIEWPORT_EDITOR);
	
	app->InitializeInAppLogger();
    app->StartEngine();
    delete app;

    return 0;
}