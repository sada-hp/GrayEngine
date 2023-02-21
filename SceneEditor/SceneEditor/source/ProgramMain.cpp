#include "ProgramMain.h"

namespace SceneEditor
{
    GrEngine::Application* app;
    float speed_mod = 10;

    void Inputs()
    {
        POINT vSize = app->GetWindowSize();
        POINT vPos = app->GetWindowPosition();
        GrEngine::Renderer* render = app->GetRenderer();
        GrEngine::Camera* camera = render->getActiveViewport();
        float cameraSpeed = 10;

        if (app->free_mode)
        {
            glm::vec2 old_cursor_pos{ vSize.x / 2, vSize.y / 2 };
            glm::vec3 direction{ 0.f };
            glm::vec3 orientation{ 0.f };
            float senstivity = 0.75f;

            POINTFLOAT cur = app->GetCursorPosition();

            orientation.x -= (old_cursor_pos.x - cur.x) * senstivity;
            orientation.y -= (old_cursor_pos.y - cur.y) * senstivity;
            camera->Rotate(orientation);
            SetCursorPos(vPos.x + (vSize.x / 2), vPos.y + (vSize.y / 2));

            if (app->IsKeyDown(GLFW_KEY_LEFT_SHIFT))
                cameraSpeed = cameraSpeed * 4;
            if (app->IsKeyDown(GLFW_KEY_W))
                direction.z -= 1;
            if (app->IsKeyDown(GLFW_KEY_S))
                direction.z += 1;
            if (app->IsKeyDown(GLFW_KEY_A))
                direction.x -= 1;
            if (app->IsKeyDown(GLFW_KEY_D))
                direction.x += 1;

            camera->MoveObject((direction * camera->GetObjectOrientation()) * (cameraSpeed + speed_mod));
        }

        if (app->transform_target != nullptr)
        {
            glm::vec3 tPos = app->transform_target->GetObjectPosition();
            glm::quat tOri = app->transform_target->GetObjectOrientation();
            glm::mat4 trans = app->transform_target->GetObjectTransformation();
            POINTFLOAT cursor = app->GetCursorPosition();
            float dist = (float)glm::length(camera->GetObjectPosition() - tPos);
            std::string gizmo_scale = std::to_string(dist * 0.185f);

            if (app->manipulation > 0 && app->manipulation < 4)
            {
                int mInd = app->manipulation - 1;
                glm::vec3 dir = glm::vec3(trans[mInd][0], trans[mInd][1], trans[mInd][2]);
                float len = dist * (float)glm::dot(glm::vec2(app->manip_start.x - cursor.x, app->manip_start.y - cursor.y), app->direct) * 0.0012f * ((vSize.x) / (float)(vSize.y));

                tPos = app->transform_target->GetObjectPosition() + dir * glm::vec3(len);
                app->transform_target->PositionObjectAt(tPos);
                app->manip_start = cursor;
                app->App_UpdateUIProperty("EntityPosition");
            }
            else if (app->manipulation >= 4 && app->manipulation < 7)
            {
                int mInd = app->manipulation - 4;
                auto vec = glm::normalize(glm::vec2(app->obj_center.x - cursor.x, app->obj_center.y - cursor.y));
                float angle = glm::acos(glm::dot(glm::normalize(vec), glm::normalize(app->direct)));

                if (!glm::isnan(angle) && !glm::isinf(angle))
                {
                    double len1 = glm::length(vec);
                    double len2 = glm::length(app->direct);
                    short delta = glm::sign(glm::asin(((vec.x * app->direct.y) / (len1 * len2)) - ((vec.y * app->direct.x) / (len2 * len1)))) * glm::sign(glm::dot(camera->GetObjectPosition() - tPos, glm::vec3(trans[mInd][0], trans[mInd][1], trans[mInd][2])));
                    std::array<glm::vec3, 3> directions = { glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1) };

                    tOri = tOri * glm::angleAxis(angle * delta, directions[mInd]);
                    app->transform_target->SetRotation(tOri);
                    app->direct = vec;
                    app->App_UpdateUIProperty("EntityOrientation");
                }
            }
            else if (app->manipulation == 7)
            {
                app->App_PaintMask();
            }

            app->gizmo->ParsePropertyValue("Scale", (gizmo_scale + ":" + gizmo_scale + ":" + gizmo_scale).c_str());
            app->gizmo->PositionObjectAt(tPos);
            app->gizmo->SetRotation(tOri);
        }

        app->UpdateUI();
    }

    int SceneEditor::EntryPoint()
    {
        Logger::AllowMessages(MessageMode::Allow);
        app = new GrEngine::Application();
        Logger::Out("--------------- Starting the engine ---------------", OutputColor::Gray, OutputType::Log);
        Logger::ShowConsole(false);

        app->GetEventListener()->pushEvent(EventType::Step, [](std::vector<double> para)
            {
                if (para.size() > 0)
                {
                    app->App_UpdateFrameCounter(para[0]);
                }
            });

        app->GetEventListener()->pushEvent(EventType::Scroll, [](std::vector<double> para)
            {
                speed_mod += para[1] * 5;
                speed_mod = std::max(speed_mod, 1.f);
            });

        app->GetEventListener()->pushEvent(EventType::MouseMove, [](std::vector<double> para)
            {
                app->App_UpdateSphere();
            });

        app->GetEventListener()->pushEvent("LoadModel", [](std::vector<std::any> para)
            {
                std::string model_path = "";
                for (auto chr : para)
                {
                    model_path += std::any_cast<char>(chr);
                }

                app->LoadFromGMF(app->transform_target->GetEntityID(), model_path.c_str());
            });

        app->GetEventListener()->pushEvent(EventType::KeyPress, [](std::vector<double> para)
            {
                if (static_cast<int>(para[0]) == GLFW_KEY_ESCAPE && static_cast<int>(para[2]) == GLFW_PRESS)
                {
                    POINT vSize = app->GetWindowSize();
                    POINT vPos = app->GetWindowPosition();
                    SetCursorPos(vPos.x + (vSize.x / 2), vPos.y + (vSize.y / 2));
                    app->toggle_free_mode();
                }
                else if (static_cast<int>(para[0]) == GLFW_KEY_DELETE && static_cast<int>(para[2]) == GLFW_PRESS)
                {
                    GrEngine::Entity* obj = app->GetRenderer()->GetSelectedEntity();
                    app->App_RemoveEntity();
                }
            });

        app->GetEventListener()->pushEvent(EventType::MouseClick, [](std::vector<double> para)
            {
                if (static_cast<int>(para[3]) == GLFW_PRESS)
                {
                    app->GetRenderer()->SelectEntityAtCursor();
                }
                else if (static_cast<int>(para[3]) == GLFW_RELEASE)
                {
                    app->SetCursorShape(GLFW_ARROW_CURSOR);
                    app->manipulation = 0;
                    app->gizmo->ParsePropertyValue("Color", "255:255:255:255");
                }
            });

        app->GetEventListener()->pushEvent(EventType::Log, [](std::vector<double> para)
            {
                char* msg = new char[para.size() + 2];
                int i = 0;
                for (double letter : para)
                {
                    msg[i++] = static_cast<char>(letter);
                }
                msg[i++] = '\n';
                msg[i] = '\0';
                app->pushToAppLogger(msg);
                delete[] msg;
            });

        app->GetEventListener()->pushEvent(EventType::SelectionChanged, [](std::vector<double> para)
            {
                int id = static_cast<int>(para[0]);
                app->App_UpdateSelection(id);
            });

        try
        {
            app->AddInputCallback(Inputs);
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

    GrEngine::Application* SceneEditor::GetApplication()
    {
        return app;
    }
}