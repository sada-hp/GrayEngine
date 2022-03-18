#pragma once
#include <GrayEngine.h>
#include <Windows.h>
namespace GrEngine {
    class Application : public Engine
    {
    public:
        Application()
        {
            MouseClickEvent([](std::vector<double> para)
                {
                    Logger::Out("MouseClickEvent", OutputColor::Blue);
                });
            WindowResizeEvent([](std::vector<double> para)
                {
                    Logger::Out("ResizeEvent", OutputColor::Blue);
                });
            KeyPressEvent([](std::vector<double> para)
                {
                    Logger::Out("KeyEvent", OutputColor::Blue);
                });
            MouseScrollEvent([](std::vector<double> para)
                {
                    Logger::Out("ScrollEvent", OutputColor::Blue);
                });
            MouseMoveEvent([](std::vector<double> para)
                {
                    //WLogger::Out("CursorMoveEvent %f", OutputColor::Blue, para.back());
                });
            WindowClosedEvent([](std::vector<double> para)
                {
                    Logger::Out("Window is now being closed", OutputColor::Gray);
                });

            CustomEvent("MyEvent", [](std::vector<double> para)
                {
                    Logger::Out("Custom Event with a parameter %d", OutputColor::Blue, (int)para.front());
                });

            std::vector<double> para = { 1 };

            CallEvent("MyEvent", para);
        }

        ~Application()
        {
            pWindow.get()->~Window();
        }
    };
}

int main(int argc, char** argv)
{
    Logger::Out("Starting the engine", OutputColor::Gray);
    auto app = (GrEngine::Engine*) new GrEngine::Application();


    app->Run();
    app->~Engine();
    return 0;
}
