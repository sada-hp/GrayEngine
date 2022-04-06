#pragma once
#include <GrayEngine.h>
#include "EditorUI.h"
#include "ModelBrowser.h"

std::string log_path = SOLUTION_DIR + std::string("GE_Log.txt");

namespace GrEngine
{
    class Application : public Engine
    {
        static Application* _instance;
        EditorUI wpfUI;
    public:
        Application(const AppParameters& Properties = AppParameters())
        {
            if (_instance != nullptr)
                delete _instance;

            _instance = this;

            EventListener::pushEvent(EventType::MouseClick,[](std::vector<double> para)
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

            EventListener::pushEvent(EventType::Step, [](std::vector<double> para)
                {
                    _instance->wpfUI.UpdateFramecounter(para.front());
                });

            std::vector<double> para = { 1 };

            EventListener::registerEvent("MyEvent", para);
        }

        ~Application()
        {
            TerminateLiraries();
        }

        static void InitializeInAppLogger()
        {
            EventListener::pushEvent(EventType::Log, [](std::vector<double> para)
                {
                    Application::UpdateAppLogger(para);
                }
            );
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
                GrEngine::Application::getEditorUI()->SetViewportPosition(VIEWPORT_EDITOR);
            }
        }

        static void initModelBrowser()
        {
            std::unique_ptr<ModelBrowser> mdlBrowser = std::make_unique<ModelBrowser>();
            mdlBrowser->init(mdlBrowser.get(), &_instance->wpfUI);
            mdlBrowser->StartEngine();
            mdlBrowser->KillEngine();
        }

        static void UpdateAppLogger(std::vector<double> para)
        {
            std::fstream log_file;
            std::string message = "";
            char* msg = new char[para.size() + 2];
            int i = 0;
            for (char letter : para)
            {
                msg[i++] = (int)letter;
            }
            msg[i++] = '\n';
            msg[i] = '\0';


            log_file.open(log_path, std::fstream::out | std::ios::app);

            log_file << msg;

            log_file.close();

            _instance->wpfUI.UpdateLogger(msg);

            delete[] msg;
        }
    };
}