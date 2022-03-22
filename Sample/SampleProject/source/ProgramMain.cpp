#pragma once
#include "EditorUI.h"
#include "Application.h"

GrEngine::Application* GrEngine::Application::_instance = nullptr;

LRESULT CALLBACK HostWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) //Background Win32 HWND is used to receive messages from WPF front-end window
{
    switch (msg)
    {
    case WM_CLOSE:
        GrEngine::Application::KillEngine();
        GrEngine::Application::getEditorUI()->DestroyUserInterface();
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        break;
    case WM_SIZE:
        if (wpf_hwnd != nullptr) 
		{
			RECT hwin_rect;
        	GetClientRect(cpphwin_hwnd, &hwin_rect);
        	MoveWindow(wpf_hwnd, 0, 0, hwin_rect.right - hwin_rect.left, hwin_rect.bottom - hwin_rect.top, TRUE);
            GrEngine::Application::getEditorUI()->SetViewportPosition();
        }
        break;
	case 0x1200:
        GrEngine::Application::loadModel((const char*)lParam);
		break;
	case 0x1201:
        GrEngine::Application::clearViewport();
		break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int main(int argc, char** argv)
{
    GrEngine::Application* app = new GrEngine::Application();

	Logger::ShowConsole(false);
    Logger::Out("Starting the engine", OutputColor::Gray, OutputType::Log);

    GrEngine::Application::getEditorUI()->InitUI(HostWindowProc);
    GrEngine::Application::getEditorUI()->SetViewportHWND(reinterpret_cast<HWND>(app->getWndNative()));
	
	app->InitializeInAppLogger();
    app->Run();

    delete app;

    return 0;
}