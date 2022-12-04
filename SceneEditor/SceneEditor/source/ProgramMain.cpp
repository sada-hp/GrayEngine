#include "ProgramMain.h"

namespace SceneEditor
{
    GrEngine::Application* app;

    void Inputs()
    {
        static glm::vec2 old_cursor_pos;
        GrEngine::Renderer* render = app->getAppWindow()->getRenderer();
        GLFWwindow* window = app->getAppWindow()->getWindow();
        GrEngine::Camera* camera = render->getActiveViewport();
        POINT cur{ 1,1 };
        float cameraSpeed = 10;
        glm::vec3 direction{ 0.f };
        glm::vec3 orientation{ 0.f };
        float senstivity = 0.75f;

        old_cursor_pos = glm::vec2{ 960, 540 };
        if (app->getAppWindow()->IsKeyDown(GLFW_KEY_LEFT_SHIFT))
            cameraSpeed = cameraSpeed * 4;

        if (app->getAppWindow()->IsKeyDown(GLFW_KEY_W))
            direction.z -= 1;
        if (app->getAppWindow()->IsKeyDown(GLFW_KEY_S))
            direction.z += 1;
        if (app->getAppWindow()->IsKeyDown(GLFW_KEY_A))
            direction.x -= 1;
        if (app->getAppWindow()->IsKeyDown(GLFW_KEY_D))
            direction.x += 1;

        GetCursorPos(&cur);
        if (app->free_mode && old_cursor_pos != glm::vec2{ 0.f })
        {
            if (glm::abs(old_cursor_pos.x - (float)(cur.x)) > 0.55f)
            {
                orientation.x -= (old_cursor_pos.x - (float)cur.x) * senstivity;
            }
            if (glm::abs(old_cursor_pos.y - (float)cur.y) > 0.55f)
            {
                orientation.y -= (old_cursor_pos.y - (float)cur.y) * senstivity;
            }
            SetCursorPos(960, 540);
        }
        else if (app->free_mode)
        {
            old_cursor_pos = glm::vec2{ 960, 540 };
            SetCursorPos(960, 540);
        }

        if (app->getAppWindow()->IsKeyDown(GLFW_KEY_UP))
            orientation.y -= 1;
        if (app->getAppWindow()->IsKeyDown(GLFW_KEY_DOWN))
            orientation.y += 1;
        if (app->getAppWindow()->IsKeyDown(GLFW_KEY_RIGHT))
            orientation.x += 1;
        if (app->getAppWindow()->IsKeyDown(GLFW_KEY_LEFT))
            orientation.x -= 1;

        camera->Rotate(orientation);
        camera->MoveObject((direction * camera->GetObjectOrientation()) * cameraSpeed);
    }

    int SceneEditor::EntryPoint()
    {
        Logger::AllowMessages(MessageMode::Allow);
        Logger::Out("--------------- Starting the engine ---------------", OutputColor::Gray, OutputType::Log);
        Logger::ShowConsole(false);
        app = new GrEngine::Application();

        EventListener::pushEvent(EventType::Step, [](std::vector<std::any> para)
            {
                if (para.size() > 0)
                {
                    app->getEditorUI()->UpdateFramecounter((float)std::any_cast<double>(para[0]));
                }
            });

        EventListener::pushEvent("LoadModel", [](std::vector<std::any> para)
            {
                std::string model_path = "";
                for (auto chr : para)
                {
                    model_path += std::any_cast<char>(chr);
                }

                app->LoadFromGMF(model_path.c_str());
            });

        EventListener::pushEvent(EventType::KeyPress, [](std::vector<std::any> para)
            {
                if (std::any_cast<int>(para[0]) == GLFW_KEY_ESCAPE && std::any_cast<int>(para[2]) == GLFW_PRESS)
                {
                    SetCursorPos(960, 540);
                    app->toggle_free_mode();
                }
            });

        EventListener::pushEvent(EventType::Log, [](std::vector<std::any> para)
            {
                char* msg = new char[para.size() + 2];
                int i = 0;
                for (auto letter : para)
                {
                    msg[i++] = std::any_cast<char>(letter);
                }
                msg[i++] = '\n';
                msg[i] = '\0';
                app->pushToAppLogger(msg);
                delete[] msg;
            }
        );

        try
        {
            app->getAppWindow()->AddInputProccess(Inputs);
            app->StartEngine();
            delete app;

            return 0;
        }
        catch (const char* msg)
        {
            Logger::Out(msg, OutputColor::Red, OutputType::Error);
            delete app;

            return 1;
        }
    }

    GrEngine::Application* GetApplication()
    {
        return app;
    }
}