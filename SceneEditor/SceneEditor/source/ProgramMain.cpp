#include "ProgramMain.h"

namespace SceneEditor
{
    GrEngine::Application* app;
    float speed_mod = 10;

    void Inputs()
    {
        app->UpdateUI();
    }

    void Character()
    {
        if (!app->focused) return;

        glm::vec3 direction{};
        POINT vSize = app->GetWindowSize();
        POINT vPos = app->GetWindowPosition();
        float sprint = 1.f;

        GrEngine::Renderer* render = app->GetRenderer();
        GrEngine::Camera* camera = render->getActiveViewport();
        glm::vec2 old_cursor_pos{ vSize.x / 2, vSize.y / 2 };
        glm::vec3 orientation{ 0.f };
        float senstivity = 0.6f;
        POINTFLOAT cur = app->GetCursorPosition();

        orientation.y -= (glm::ceil(((old_cursor_pos.x - cur.x) * senstivity) * 10000) / 10000);
        orientation.x -= (glm::ceil(((old_cursor_pos.y - cur.y) * senstivity) * 10000) / 10000);
        camera->Rotate(orientation);
        SetCursorPos(vPos.x + (vSize.x / 2), vPos.y + (vSize.y / 2));

        glm::quat q = glm::quat_cast(glm::mat3(1.f));
        q = q * glm::angleAxis(glm::radians(0.f), glm::vec3(1, 0, 0));
        q = q * glm::angleAxis(glm::radians(-camera->GetActualPYR().y), glm::vec3(0, 1, 0));
        q = q * glm::angleAxis(glm::radians(0.f), glm::vec3(0, 0, 1));
        glm::mat4 trans = glm::translate(glm::mat4(1.f), camera->GetObjectPosition()) * glm::mat4_cast(q);
        if (app->IsKeyDown(GLFW_KEY_W))
        {
            direction -= glm::vec3(trans[2][0], trans[2][1], trans[2][2]);
        }
        if (app->IsKeyDown(GLFW_KEY_S))
        {
            direction += glm::vec3(trans[2][0], trans[2][1], trans[2][2]);
        }
        if (app->IsKeyDown(GLFW_KEY_D))
        {
            direction += glm::vec3(trans[0][0], trans[0][1], trans[0][2]);
        }
        if (app->IsKeyDown(GLFW_KEY_A))
        {
            direction -= glm::vec3(trans[0][0], trans[0][1], trans[0][2]);
        }
        if (app->IsKeyDown(GLFW_KEY_LEFT_SHIFT))
        {
            sprint = 1.35f;
        }
        GrEngine::PhysicsObject* phys_comp = static_cast<GrEngine::PhysicsObject*>(app->shrek_coll->GetProperty(PropertyType::PhysComponent)->GetValueAdress());
        if (direction != glm::vec3(0.f))
        {
            glm::vec3 cap_pos = app->shrek_coll->GetObjectPosition();
            GrEngine::RayCastResult ray = app->GetPhysics()->CastRayGetHit(cap_pos, glm::vec3(cap_pos.x, cap_pos.y - 2.5f, cap_pos.z));
            std::vector<GrEngine::RayCastResult> res;
            if (!ray.hasHit)
            {
                res = app->GetPhysics()->GetObjectContactPoints(phys_comp, 25.f);
            }
            else
            {
                res.push_back(ray);
            }

            if (res.size() > 0)
            {
                glm::vec3 normal = glm::vec3(0.f);
                int count = 0;

                for (int i = 0; i < res.size(); i++)
                {
                    normal += res[i].hitNorm;
                    count++;
                }

                normal = normal / (float)count;
                normal = glm::normalize(normal);

                float max_ang = glm::radians(45.f);
                float ang = glm::acos(glm::dot(glm::vec3(0, 1, 0), normal));
                //Logger::Out("Angle of the slope %f", OutputColor::Gray, OutputType::Log, ang);
                if (ang < max_ang)
                {
                    phys_comp->MoveObject(glm::vec3(0.15f * sprint, 0.f, 0.15f * sprint) * glm::normalize(direction));
                }
                else
                {
                    float sign = glm::sign(glm::dot(direction, normal));
                    //float bias = glm::abs(glm::radians(45.f) - ang) * 10;
                    float limit = glm::radians(20.f) / 100.f;
                    float bias = 0.15f * ((ang - max_ang) / limit) / 100.f;
                    //Logger::Out("Bias of the slope %f", OutputColor::Gray, OutputType::Log, ((ang - max_ang) / limit));
                    float speed = 0.15f + bias * sign;
                    if (speed > 0)
                    {
                        speed = glm::min(speed, 2.75f);
                        phys_comp->MoveObject(glm::vec3(speed, 0.f, speed) * glm::normalize(direction));
                    }
                    else
                    {
                        speed = glm::abs(speed) + 1.65f;
                        phys_comp->SlideObjectForDuration(glm::vec3(speed, 0.f, speed) * glm::normalize((normal)), 0.1f);
                    }
                }
            }
            else
            {
                phys_comp->MoveObject(glm::vec3(0.1f, 0.f, 0.1f) * glm::normalize(direction));
            }
        }
        else
        {
            //glm::vec3 cap_pos = app->shrek_coll->GetObjectPosition();
            //GrEngine::RayCastResult res = app->GetPhysics()->CastRayGetHit(cap_pos, glm::vec3(cap_pos.x, cap_pos.y - 4.5f, cap_pos.z));

            std::vector<GrEngine::RayCastResult> res = app->GetPhysics()->GetObjectContactPoints(phys_comp, 25.f);
            if (res.size() > 0)
            {
                glm::vec3 normal = glm::vec3(0.f);
                int count = 0;

                for (int i = 0; i < res.size(); i++)
                {
                    normal += res[i].hitNorm;
                    count++;
                }

                normal = normal / (float)count;
                normal = glm::normalize(normal);

                constexpr float max_ang = glm::radians(45.f);
                float ang = glm::acos(glm::dot(glm::vec3(0, 1, 0), normal));
                bool sliding = ang <= max_ang;
                float sign = glm::sign(glm::dot(direction, normal));
                if (!sliding)
                {
                    constexpr float limit = glm::radians(20.f) / 100.f;
                    float bias = 0.05f * ((ang - max_ang) / limit) / 100.f;
                    float speed = 0.1f + bias * sign;
                    speed = glm::abs(speed) + 1.65f;
                    phys_comp->SlideObjectForDuration(glm::vec3(speed, 0.f, speed) * glm::normalize((normal)), 0.1f);
                }
                else
                {
                    phys_comp->MoveObject(glm::vec3(0.f, 0.f, 0.f));
                }
            }
            else
            {
                phys_comp->MoveObject(glm::vec3(0.f, 0.f, 0.f));
            }
        }
    }

    void Transformation()
    {
        POINT vSize = app->GetWindowSize();
        GrEngine::Renderer* render = app->GetRenderer();
        GrEngine::Camera* camera = render->getActiveViewport();

        glm::vec3 tPos = app->transform_target->GetObjectPosition();
        glm::quat tOri = app->transform_target->GetObjectOrientation();
        glm::mat4 trans = app->transform_target->GetObjectTransformation();
        POINTFLOAT cursor = app->GetCursorPosition();
        float dist = (float)glm::length(camera->GetObjectPosition() - tPos);
        std::string gizmo_scale = std::to_string(dist * 0.25f);

        if (app->manipulation > 0 && app->manipulation < 4 && app->mouse_down)
        {
            int mInd = app->manipulation - 1;
            glm::vec3 dir = glm::vec3(trans[mInd][0], trans[mInd][1], trans[mInd][2]);
            float len = dist * (float)glm::dot(glm::vec2(app->manip_start.x - cursor.x, app->manip_start.y - cursor.y), app->direct) * 0.0012f * ((vSize.x) / (float)(vSize.y));

            tPos = app->transform_target->GetObjectPosition() + dir * glm::vec3(len);
            app->transform_target->PositionObjectAt(tPos);
            app->manip_start = cursor;
            app->App_UpdateUIProperty("EntityPosition");
        }
        else if (app->manipulation >= 4 && app->manipulation < 7 && app->mouse_down)
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

        app->gizmo->ParsePropertyValue("Scale", (gizmo_scale + ":" + gizmo_scale + ":" + gizmo_scale).c_str());
        app->gizmo->PositionObjectAt(tPos);
        app->gizmo->SetRotation(tOri);
    }

    void FreeCamera()
    {
        if (!app->focused) return;

        POINT vSize = app->GetWindowSize();
        POINT vPos = app->GetWindowPosition();
        GrEngine::Renderer* render = app->GetRenderer();
        GrEngine::Camera* camera = render->getActiveViewport();
        float cameraSpeed = 10;

        glm::vec2 old_cursor_pos{ vSize.x / 2, vSize.y / 2 };
        glm::vec3 direction{ 0.f };
        glm::vec3 orientation{ 0.f };
        float senstivity = 0.6f;

        POINTFLOAT cur = app->GetCursorPosition();

        orientation.y -= (glm::ceil(((old_cursor_pos.x - cur.x) * senstivity) * 10000) / 10000);
        orientation.x -= (glm::ceil(((old_cursor_pos.y - cur.y) * senstivity) * 10000) / 10000);
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

    void TerrainBrush()
    {
        app->App_PaintMask();
    }

    void TerrainSculpt()
    {
        app->App_Sculpt();
    }

    int SceneEditor::EntryPoint()
    {
        Logger::AllowMessages(MessageMode::Allow);
        app = new GrEngine::Application();
        Logger::ShowConsole(false);
        app->SetVSync(1);

        app->GetEventListener()->pushEvent(EventType::Step, [](std::vector<double> para)
            {
                if (para.size() > 0)
                {
                    app->App_UpdateFrameCounter(para[0]);
                }
            });

        app->GetEventListener()->pushEvent(EventType::Scroll, [](std::vector<double> para)
            {
                if (app->free_mode)
                {
                    speed_mod += para[1] * 5;
                    speed_mod = std::max(speed_mod, 1.f);
                }
                else if (app->manipulation == 7 || app->manipulation == 8)
                {
                    if (app->ctr_down)
                        app->App_UpdateBrush(app->paint_mode, glm::max(app->brush_opacity + para[1] / 50, 0.01), app->brush_size, app->brush_falloff);
                    else
                        app->App_UpdateBrush(app->paint_mode, app->brush_opacity, glm::max(app->brush_size + para[1] / 10, 0.), app->brush_falloff);

                    app->App_SendBrushInfo();
                }
            });

        app->GetEventListener()->pushEvent(EventType::MouseMove, [](std::vector<double> para)
            {
                app->App_UpdateSphere();
            });

        app->GetEventListener()->pushEvent("LoadModel", [](std::vector<std::any> para)
            {
                std::string model_path = "";
                std::string solution = GrEngine::Globals::getExecutablePath();

                for (auto chr : para)
                {
                    model_path += std::any_cast<char>(chr);
                }

                app->LoadFromGMF(app->transform_target->GetEntityID(), model_path.erase(0, solution.size()).c_str());
            });

        app->GetEventListener()->pushEvent("TerrainBlendMask", [](std::vector<std::any> para)
            {
                if (std::any_cast<bool>(para[0]))
                {
                    app->AddInputCallback(3, TerrainBrush);
                }
                else
                {
                    app->RemoveInputCallback(3);
                }
            });

        app->GetEventListener()->pushEvent("TerrainSculptMask", [](std::vector<std::any> para)
            {
                if (std::any_cast<bool>(para[0]))
                {
                    app->AddInputCallback(4, TerrainSculpt);
                }
                else
                {
                    app->RemoveInputCallback(4);
                }
            });

        app->GetEventListener()->pushEvent(EventType::KeyPress, [](std::vector<double> para)
            {
                if (static_cast<int>(para[0]) == GLFW_KEY_ESCAPE && static_cast<int>(para[2]) == GLFW_PRESS)
                {
                    POINT vSize = app->GetWindowSize();
                    POINT vPos = app->GetWindowPosition();
                    SetCursorPos(vPos.x + (vSize.x / 2), vPos.y + (vSize.y / 2));
                    app->toggle_free_mode();

                    if (app->free_mode)
                    {
                        app->AddInputCallback(1, FreeCamera);
                    }
                    else
                    {
                        app->RemoveInputCallback(1);
                    }
                }
                else if (static_cast<int>(para[0]) == GLFW_KEY_DELETE && static_cast<int>(para[2]) == GLFW_PRESS)
                {
                    GrEngine::Entity* obj = app->GetRenderer()->GetSelectedEntity();
                    app->App_RemoveEntity();
                }
                else if (static_cast<int>(para[0]) == GLFW_KEY_LEFT_CONTROL)
                {
                    app->ctr_down = static_cast<int>(para[2]) != GLFW_RELEASE;
                }
                else if (static_cast<int>(para[0]) == GLFW_KEY_TAB && static_cast<int>(para[2]) == GLFW_PRESS)
                {
                    if (!app->char_mode)
                    {
                        POINT vSize = app->GetWindowSize();
                        POINT vPos = app->GetWindowPosition();
                        SetCursorPos(vPos.x + (vSize.x / 2), vPos.y + (vSize.y / 2));

                        if (app->free_mode)
                        {
                            app->toggle_free_mode();
                            app->RemoveInputCallback(1);
                        }
                        app->AddInputCallback(5, Character);
                        app->App_SetUpCharacter();
                        app->TogglePhysicsState(true);
                        app->char_mode = true;
                    }
                    else
                    {
                        app->RemoveInputCallback(5);
                        app->TogglePhysicsState(false);
                        app->App_ClearCharacter();
                        app->char_mode = false;
                    }
                    app->SetCursorState(!app->char_mode);
                }
                else if (static_cast<int>(para[0]) == GLFW_KEY_S && app->ctr_down && static_cast<int>(para[2]) == GLFW_PRESS && !app->free_mode && !app->char_mode)
                {
                    app->App_SaveScene();
                }
                else if (static_cast<int>(para[0]) == GLFW_KEY_C && app->ctr_down && static_cast<int>(para[2]) == GLFW_PRESS && !app->free_mode && !app->char_mode)
                {
                    app->copy_buf = app->GetRenderer()->GetSelectionID();
                }
                else if (static_cast<int>(para[0]) == GLFW_KEY_V && app->ctr_down && static_cast<int>(para[2]) == GLFW_PRESS && !app->free_mode && !app->char_mode)
                {
                    app->App_CloneEntity();
                }
            });

        app->GetEventListener()->pushEvent(EventType::MouseClick, [](std::vector<double> para)
            {
                if (static_cast<int>(para[3]) == GLFW_PRESS)
                {
                    app->mouse_down = true;
                    app->GetRenderer()->SelectEntityAtCursor();
                }
                else if (static_cast<int>(para[3]) == GLFW_RELEASE)
                {
                    app->mouse_down = false;
                    app->SetCursorShape(GLFW_ARROW_CURSOR);
                    app->gizmo->ParsePropertyValue("Color", "1:1:1:1");
                    app->manipulation = app->manipulation < 7 ? 0 : app->manipulation;

                    if (app->manipulation == 8)
                    {
                        app->App_RecalculateTerrain();
                    }
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

                if (app->transform_target != nullptr)
                {
                    app->AddInputCallback(2, Transformation);
                }
                else
                {
                    app->RemoveInputCallback(2);
                }
            });

        app->GetEventListener()->pushEvent(EventType::FocusChanged, [](std::vector<double> para)
            {
                app->focused = static_cast<int>(para[0]);
            });

        try
        {
            app->AddInputCallback(0, Inputs);
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