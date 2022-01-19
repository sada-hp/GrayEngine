#define GLFW_INCLUDE_VULKAN
#include <GrayEngine.h>

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
                    //Logger::Out("CursorMoveEvent", OutputColor::Blue);
                });
        }

        ~Application()
        {

        }
    };

    Engine* Init()
    {
        Logger::Out("Engine started successfully", OutputColor::Green);
        return new Application();
    }
}