#pragma once
#include <GrayEngine.h>

std::string log_path = SOLUTION_DIR + std::string("GE_Log.txt");

namespace GrEngine
{
    class Application : public Engine
    {
        static Application* _instance;
        EditorUI wpfUI;
    public:
        Application()
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

            std::vector<double> para = { 1 };

            EventListener::registerEvent("MyEvent", para);
        }

        ~Application()
        {

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
            _instance->loadModelFromPath(mesh_path);
        }

        static void clearViewport()
        {
            _instance->clearScene();
        }

        static HWND getGLFW_HWND()
        {
            return reinterpret_cast<HWND>(_instance->getWndNative());
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