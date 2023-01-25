#pragma once
#include <SceneEditor.h>
#include <GrayEngine.h>
#include "EditorUI.h"
#include "ModelBrowser.h"

namespace GrEngine
{
    class Application : public Engine
    {
        EditorUI editorUI;
        std::string log_path;


    public:
        bool free_mode = false;
        uint8_t manipulation = 0;
        POINTFLOAT manip_start;
        Entity* grid;
        Entity* gizmo;
        Entity* transform_target;
        glm::vec2 direct;
        glm::vec2 obj_center;

        Application(const AppParameters& Properties = AppParameters()) : Engine(Properties)
        {
            log_path = getExecutablePath() + std::string("grayengine.log");

            GetRenderer()->getActiveViewport()->LockAxes(0, 0, 89, -89, 0, 0);

            getEditorUI()->InitUI(VIEWPORT_EDITOR);
            getEditorUI()->SetViewportHWND(getViewportHWND(), VIEWPORT_EDITOR);
        }

        ~Application()
        {
            Stop();
            getEditorUI()->destroyUI(VIEWPORT_EDITOR);
            TerminateLiraries();
        }

        void StartEngine()
        {
            getEditorUI()->ShowScene();
            LoadTools();
            Run();
        }

        void App_SaveScene(const char* path)
        {
            SaveScene(path);
        }

        void App_UpdateSelection(UINT id)
        {
            if (id == gizmo->GetEntityID())
            {
                if (GetPhysics()->GetSimulationState()) return;

                SetCursorShape(GLFW_HAND_CURSOR);
                glm::mat4 model = glm::translate(glm::mat4_cast(GetRenderer()->getActiveViewport()->GetObjectOrientation()), -GetRenderer()->getActiveViewport()->GetObjectPosition());
                glm::mat4 proj = glm::perspective(glm::radians(60.0f), (float)GetWindowSize().x / (float)GetWindowSize().y, 1.0f, 100.0f);
                glm::vec4 view(0, 0, GetWindowSize().x, GetWindowSize().y);
                proj[1][1] *= -1;
                auto pos = gizmo->GetObjectPosition();

                std::array<int, 3> rgb = GetRenderer()->GetPixelColorAtCursor();
                if (rgb[0] == 255)
                {
                    manipulation = 1;
                    gizmo->ParsePropertyValue("Color", "255:100:100:255");
                }
                else if (rgb[1] == 255)
                {
                    manipulation = 2;
                    gizmo->ParsePropertyValue("Color", "100:255:100:255");
                }
                else if (rgb[2] == 255)
                {
                    manipulation = 3;
                    gizmo->ParsePropertyValue("Color", "100:100:255:255");
                }
                else if (rgb[0] == 254)
                {
                    manipulation = 4;
                    gizmo->ParsePropertyValue("Color", "255:100:100:255");
                    glm::vec3 dir = glm::normalize(glm::vec3(gizmo->GetObjectTransformation()[0][0], gizmo->GetObjectTransformation()[0][1], gizmo->GetObjectTransformation()[0][2]));
                    pos = pos + dir * glm::vec3(static_cast<DrawableObject*>(gizmo)->GetObjectBounds().x) * gizmo->GetPropertyValue("Scale", glm::vec3(1.f));
                }
                else if (rgb[1] == 254)
                {
                    manipulation = 5;
                    gizmo->ParsePropertyValue("Color", "100:255:100:255");
                    glm::vec3 dir = glm::vec3(gizmo->GetObjectTransformation()[1][0], gizmo->GetObjectTransformation()[1][1], gizmo->GetObjectTransformation()[1][2]);
                    pos = pos + dir * glm::vec3(static_cast<DrawableObject*>(gizmo)->GetObjectBounds().y) * gizmo->GetPropertyValue("Scale", glm::vec3(1.f));
                }
                else if (rgb[2] == 254)
                {
                    manipulation = 6;
                    gizmo->ParsePropertyValue("Color", "100:100:255:255");
                    glm::vec3 dir = glm::vec3(gizmo->GetObjectTransformation()[2][0], gizmo->GetObjectTransformation()[2][1], gizmo->GetObjectTransformation()[2][2]);
                    pos = pos + dir * glm::vec3(static_cast<DrawableObject*>(gizmo)->GetObjectBounds().z) * gizmo->GetPropertyValue("Scale", glm::vec3(1.f));
                }
                else
                {
                    return;
                }

                manip_start = GetCursorPosition();
                obj_center = glm::project(pos, model, proj, view);
                direct = glm::normalize(glm::vec2{ obj_center.x - manip_start.x, obj_center.y - manip_start.y });
            }
            else
            {
                getEditorUI()->SetSelectedEntity(id);
                transform_target = GetRenderer()->GetSelectedEntity();

                if (transform_target != nullptr && transform_target != gizmo && transform_target != grid)
                {
                    static_cast<DrawableObject*>(gizmo)->SetVisisibility(true);
                    gizmo->PositionObjectAt(transform_target->GetObjectPosition());
                    gizmo->SetRotation(transform_target->GetObjectOrientation());
                }
                else
                {
                    static_cast<DrawableObject*>(gizmo)->SetVisisibility(false);
                }
            }
        }

        EditorUI* getEditorUI()
        {
            return &editorUI;
        };

        HWND getViewportHWND()
        {
            return reinterpret_cast<HWND>(getNativeWindow());
        }

        void redrawDesiger()
        {
            if (getEditorUI()->wpf_hwnd != nullptr)
            {
                getEditorUI()->SetViewportPosition(VIEWPORT_EDITOR);
            }
        }

        void initModelBrowser()
        {
            Pause();
            Logger::AllowMessages(MessageMode::Block);
            EventListener::setEventsPermissions(false, true);
            std::thread mdl(RunModelBrowser);   
            mdl.join();
            GetRenderer()->SetHighlightingMode(true);
            BindContext(this);
            EventListener::setEventsPermissions(true, true);
            Logger::AllowMessages(MessageMode::Allow);
            Unpause();
        }

        void toggle_free_mode()
        {
            free_mode = !free_mode;
            SetCursorState(!free_mode);
        }

        void App_UpdateEntity(Entity* target)
        {
            getEditorUI()->UpdateEntity(target->GetEntityID(), (char*)target->GetEntityNameTag());
        }

        void App_RemoveEntity()
        {
            if (transform_target != nullptr)
            {
                UINT id = transform_target->GetEntityID();
                GetRenderer()->DeleteEntity(id);
                getEditorUI()->RemoveEntity(id);
                static_cast<GrEngine::DrawableObject*>(gizmo)->SetVisisibility(false);
            }
        }

        void getEntityInfo(int ID)
        {
            auto ent = GetRenderer()->selectEntity(ID);
            auto props = ent->GetProperties();
            int numProps = props.size();

            if (ent->GetEntityType() == "Object")
            {
                for (int i = 0; i < numProps; i++)
                {
                    getEditorUI()->SendEntityInfo(ID, (char*)props[i]->property_name, (char*)props[i]->ValueString(), (char*)props[i]->TypeString());
                }
            }
        }

        void App_GetAllEntities()
        {
            auto ent_vec = GetRenderer()->GetEntitiesList();
            ent_vec.erase(grid->GetEntityID());
            ent_vec.erase(gizmo->GetEntityID());

            for (auto obj : ent_vec)
            {
                if (obj.second->GetEntityType() == "Object")
                {
                    App_UpdateEntity(obj.second);
                }
            }
        }

        void pushToAppLogger(char* message)
        {
            std::fstream log_file;

            log_file.open(log_path, std::fstream::out | std::ios::app);

            log_file << message;

            log_file.close();

            editorUI.UpdateLogger(message);
        }

        void App_UpdateUIProperty(const char* property_name)
        {
            static auto time = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - time).count() > 35.f)
            {
                getEditorUI()->SendEntityInfo(transform_target->GetEntityID(), (char*)property_name, (char*)transform_target->GetProperty(property_name)->ValueString(), (char*)transform_target->GetProperty(property_name)->TypeString());
                time = now;
            }
        }

        void App_UpdateFrameCounter(float new_value)
        {
            getEditorUI()->UpdateFramecounter(new_value);
        }

        void updateEntity(int ID, std::string selected_property, std::string value)
        {
            Entity* selection = SelectEntity(ID);
            selection->ParsePropertyValue(selected_property.c_str(), value.c_str());
        }

        void addNewProperty(UINT ID, const char* property_name)
        {
            Entity* selection = SelectEntity(ID);
            selection->AddNewProperty(property_name);
        }
    private:

        static void RunModelBrowser()
        {
            AppParameters props;
            ModelBrowser* mdlBrowser = new ModelBrowser(props);
            BindContext(mdlBrowser);
            mdlBrowser->init(mdlBrowser);
            mdlBrowser->StartEngine();
            mdlBrowser->Stop();
            delete mdlBrowser;
        }

        void LoadTools()
        {
            //Grid
            grid = GetRenderer()->addEntity();
            static_cast<DrawableObject*>(grid)->DisableCollisions();
            grid->ParsePropertyValue("Shader", "Shaders\\grid");
            grid->MakeStatic();

            //Move
            gizmo = GetRenderer()->addEntity();
            static_cast<DrawableObject*>(gizmo)->DisableCollisions();
            static_cast<DrawableObject*>(gizmo)->LoadMesh((Globals::getExecutablePath() + "Content\\Editor\\ManipulationTool.obj").c_str(), false, nullptr);
            gizmo->ParsePropertyValue("Shader", "Shaders\\gizmo");
            gizmo->AddNewProperty("Color");
            gizmo->ParsePropertyValue("Color", "1:1:1:1");
            gizmo->AddNewProperty("Scale");
            gizmo->MakeStatic();
        }

        void UnloadTools()
        {
            GetRenderer()->DeleteEntity(grid->GetEntityID());
        }
    };
}