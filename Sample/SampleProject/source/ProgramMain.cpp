#pragma once
#include "EditorUI.h"
#include "Application.h"

GrEngine::Application* GrEngine::Application::_instance = nullptr;

LRESULT CALLBACK HostWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) //Background Win32 is used to receive messages from WPF front-end window
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

    app->getEditorUI()->InitUI(HostWindowProc);
    app->getEditorUI()->SetViewportHWND(app->getGLFW_HWND());
	
	app->InitializeInAppLogger();
    app->StartEngine();
    delete app;

    return 0;
}