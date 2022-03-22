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
            _instance = this;

            MouseClickEvent([](std::vector<double> para)
                {
                    Logger::Out("MouseClickEvent", OutputColor::Blue, OutputType::Log);
                });
            WindowResizeEvent([](std::vector<double> para)
                {
                    Logger::Out("ResizeEvent", OutputColor::Blue, OutputType::Log);
                });
            KeyPressEvent([](std::vector<double> para)
                {
                    Logger::Out("KeyEvent", OutputColor::Blue, OutputType::Log);
                });
            MouseScrollEvent([](std::vector<double> para)
                {
                    Logger::Out("ScrollEvent", OutputColor::Blue, OutputType::Log);
                });
            MouseMoveEvent([](std::vector<double> para)
                {
                    //WLogger::Out("CursorMoveEvent %f", OutputColor::Blue, para.back());
                });
            WindowClosedEvent([](std::vector<double> para)
                {
                    Logger::Out("Window is now being closed", OutputColor::Gray, OutputType::Log);
                });

            CustomEvent("MyEvent", [](std::vector<double> para)
                {
                    Logger::Out("Custom Event with a parameter %d", OutputColor::Blue, OutputType::Log, (int)para.front());
                });

            std::vector<double> para = { 1 };

            CallEvent("MyEvent", para);
        }

        ~Application()
        {

        }

        void InitializeInAppLogger()
        {
            LogEvent
            ([](std::vector<double> para)
                {
                    Application::UpdateAppLogger(para);
                }
            );
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