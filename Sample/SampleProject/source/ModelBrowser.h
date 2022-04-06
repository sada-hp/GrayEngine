#pragma once
#include <GrayEngine.h>
#include "EditorUI.h"

namespace GrEngine
{
    class ModelBrowser : public Engine
    {
        static ModelBrowser* _instance;
        EditorUI* wpfUI;

        static LRESULT CALLBACK HostWindowProcBrowser(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) //Background Win32 is used to receive messages from WPF front-end window
        {
            switch (msg)
            {
            case WM_CLOSE:
                GrEngine::ModelBrowser::closeBrowser();
                DestroyWindow(hwnd);
                break;
            case WM_DESTROY:
                break;
            case WM_SIZE:
                GrEngine::ModelBrowser::updateWpfWnd();
                break;
                /*Messages received from the C# WPF front-end part of the editor*/
            case 0x1200: //Load obj file callback
                GrEngine::ModelBrowser::loadModel((const char*)lParam);
                break;
            case 0x1201: //Clear viewport callback
                GrEngine::ModelBrowser::clearViewport();
                break;
            case 0x1202: //Upload texture file callback
                GrEngine::ModelBrowser::uploadTexture((const char*)lParam);
                break;
            case 0x1203: //Close model browser
                GrEngine::ModelBrowser::closeBrowser();
                break;
            default:
                return DefWindowProcA(hwnd, msg, wParam, lParam);
            }
            return 0;
        }

    public:
        ModelBrowser(const AppParameters& Properties = AppParameters())
        {
            EventListener::pushEvent(EventType::MouseClick, [](std::vector<double> para)
                {
                    Logger::Out("MouseClickEvent", OutputColor::Blue, OutputType::Log);
                });
            EventListener::pushEvent(EventType::WindowResize, [](std::vector<double> para)
                {
                    Logger::Out("ResizeEvent", OutputColor::Blue, OutputType::Log);
                });
            EventListener::pushEvent(EventType::KeyPress, [](std::vector<double> para)
                {
                    Logger::Out("KeyEvent", OutputColor::Blue, OutputType::Log);
                });
            EventListener::pushEvent(EventType::Scroll, [](std::vector<double> para)
                {
                    Logger::Out("ScrollEvent", OutputColor::Blue, OutputType::Log);
                });
            EventListener::pushEvent(EventType::MouseMove, [](std::vector<double> para)
                {
                    //WLogger::Out("CursorMoveEvent %f", OutputColor::Blue, para.back());
                });
            EventListener::pushEvent(EventType::WindowClosed, [](std::vector<double> para)
                {
                    Logger::Out("Window is now being closed", OutputColor::Gray, OutputType::Log);
                });

            EventListener::pushEvent("MyEvent", [](std::vector<double> para)
                {
                    Logger::Out("Custom Event with a parameter %d", OutputColor::Blue, OutputType::Log, (int)para.front());
                });
        }

        ~ModelBrowser()
        {
            _instance = nullptr;
        }

        void init(ModelBrowser* instance, EditorUI* pUserInterface)
        {
            if (_instance != nullptr)
            {
                delete _instance;
                _instance = nullptr;
            }

            _instance = instance;
            wpfUI = pUserInterface;
            initModelBrowser();
        }

        static void StartEngine()
        {
            _instance->Run();
        }

        static void KillEngine()
        {
            _instance->Stop();
        }

        static EditorUI* getEditorUI()
        {
            return _instance->wpfUI;
        };

        static void loadModel(const char* mesh_path)
        {
            _instance->loadMeshFromPath(mesh_path);
        }

        static void uploadTexture(const char* image_path)
        {
            _instance->loadImageFromPath(image_path);
        }

        static void clearViewport()
        {
            _instance->clearScene();
        }

        static HWND getGLFW_HWND()
        {
            return reinterpret_cast<HWND>(_instance->getWndNative());
        }

        static void updateWpfWnd()
        {
            if (getEditorUI()->wpf_hwnd != nullptr)
            {
                RECT hwin_rect;
                GetClientRect(getEditorUI()->cpphwin_hwnd, &hwin_rect);
                MoveWindow(getEditorUI()->wpf_hwnd, 0, 0, hwin_rect.right - hwin_rect.left, hwin_rect.bottom - hwin_rect.top, TRUE);
                GrEngine::ModelBrowser::getEditorUI()->SetViewportPosition(VIEWPORT_EDITOR);
            }
        }

        static void initModelBrowser()
        {
            getEditorUI()->InitUI(HostWindowProcBrowser, MODEL_BROWSER_CLASSNAME, VIEWPORT_MODEL_BROWSER);
            getEditorUI()->SetViewportHWND(getGLFW_HWND(), 1);
            ShowWindow(getGLFW_HWND(), SW_SHOW);
        }

        static void closeBrowser()
        {
            KillEngine();
            getEditorUI()->destroyUI(VIEWPORT_MODEL_BROWSER);
        }
    };
}