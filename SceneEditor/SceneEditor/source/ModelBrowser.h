#pragma once
#include <GrayEngine.h>
#include "EditorUI.h"
#include <chrono>

namespace GrEngine
{
    class ModelBrowser : public Engine
    {
        static ModelBrowser* _instance;
        EditorUI wpfUI;

        static LRESULT CALLBACK HostWindowProcBrowser(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) //Background Win32 is used to receive messages from WPF front-end window
        {
            const char* mod = (const char*)wParam;
            const char* mat = (const char*)lParam;
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
                GrEngine::ModelBrowser::createModel((const char*)lParam);
                break;
            case 0x1201: //Clear viewport callback
                GrEngine::ModelBrowser::clearViewport();
                break;
            case 0x1202: //Upload texture file callback
                GrEngine::ModelBrowser::uploadTexture((const char*)lParam, (int)wParam);
                break;
            case 0x1203: //Close model browser
                GrEngine::ModelBrowser::createModel((const char*)wParam, (const char*)lParam);
                break;
            default:
                return DefWindowProcA(hwnd, msg, wParam, lParam);
            }
            return 0;
        }

    public:
        ModelBrowser(const AppParameters& Properties = AppParameters())
        {

        }

        ~ModelBrowser()
        {
            _instance = nullptr;
        }

        void init(ModelBrowser* instance)
        {
            if (_instance != nullptr)
            {
                delete _instance;
                _instance = nullptr;
            }

            _instance = instance;
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
            return &_instance->wpfUI;
        }

        static void uploadTexture(const char* image_path, int material_index)
        {
            _instance->loadImageFromPath(image_path, material_index);
        }

        static void createModel(const char* mesh_path, const char* textures_str = nullptr)
        {
            std::string materials;

            clearViewport();
            auto res = _instance->loadModel(mesh_path, textures_str, &materials);

            if (res)
                getEditorUI()->UpdateMaterials((char*)materials.c_str());
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
        }

        static void closeBrowser()
        {
            KillEngine();
            getEditorUI()->destroyUI(VIEWPORT_MODEL_BROWSER);
        }
    };
}