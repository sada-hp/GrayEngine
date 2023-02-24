#pragma once
#include <GrayEngine.h>
#include "EditorUI.h"
#include <chrono>
#include <glfw/glfw3.h>

namespace GrEngine
{
    class ModelBrowser : public Engine
    {
        EditorUI editorUI;
        Entity* dummy_entity;

    public:
        ModelBrowser(const AppParameters& Properties = AppParameters()) : Engine(Properties)
        {
        }

        ~ModelBrowser()
        {
            getEditorUI()->destroyUI(VIEWPORT_MODEL_BROWSER);
            dummy_entity = nullptr;
        }

        void init(ModelBrowser* instance)
        {
            initModelBrowser();
            GetRenderer()->SetHighlightingMode(false);
            dummy_entity = AddEntity();
            SelectEntity(dummy_entity->GetEntityID());
            AddInputCallback(0, Inputs);

            GetEventListener()->pushEvent("RequireMaterialsUpdate", [](std::vector<std::any> para)
                {
                    if (para.size() > 1)
                    {
                        dynamic_cast<ModelBrowser*>(GetContext())->getEditorUI()->UpdateMaterials((char*)std::any_cast<std::string>(para[0]).c_str(), (char*)std::any_cast<std::string>(para[1]).c_str(), std::any_cast<int>(para[2]));
                    }
                });
        }

        void StartEngine()
        {
            getEditorUI()->ShowScene();
            Run();
        }

        static void Inputs()
        {
            static float rotation = 0;
            Renderer* render = GetContext()->GetRenderer();
            DrawableObject* drawable = (DrawableObject*)render->GetSelectedEntity();
            Camera* camera = render->getActiveViewport();

            if (drawable != NULL)
            {
                glm::vec3 axis = glm::vec3(3.f + drawable->GetObjectBounds().x, 3.f + drawable->GetObjectBounds().y, 3.f + drawable->GetObjectBounds().z);

                rotation += GrEngine::Globals::delta_time;
                axis = glm::vec3(axis.z * glm::cos(rotation) + axis.x * glm::sin(rotation), axis.y, axis.z * glm::sin(rotation) - axis.x * glm::cos(rotation));

                camera->SetRotation(glm::lookAt(axis, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
                camera->PositionObjectAt(axis);
            }
            else
            {
                camera->SetRotation(glm::lookAt(glm::vec3(1000.f, 1000.f, 1000.f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
                camera->PositionObjectAt(glm::vec3(1000.f, 1000.f, 1000.f));
            }
        }

        EditorUI* getEditorUI()
        {
            return &editorUI;
        }

        HWND getViewportHWND()
        {
            return reinterpret_cast<HWND>(getNativeWindow());
        }

        void redrawDesigner()
        {
            if (getEditorUI()->wpf_hwnd != nullptr)
            {
                RECT hwin_rect;
                getEditorUI()->SetViewportPosition(VIEWPORT_EDITOR);
            }
        }

        void initModelBrowser()
        {

            if (!getEditorUI()->InitUI(VIEWPORT_MODEL_BROWSER))
            {
                Stop();
                return;
            }
            getEditorUI()->SetViewportHWND(getViewportHWND(), 1);
        }
    };
}