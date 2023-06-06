#pragma once
#include <GrayEngine.h>
#include "EditorUI.h"
#include <chrono>
#include <glfw/glfw3.h>

namespace GrEngine
{
    class ModelBrowser
    {
        EditorUI editorUI;
        Entity* dummy_entity = nullptr;
        Object* dummy_mesh = nullptr;
        Engine* pEngine = nullptr;
        Camera* camera = nullptr;
        bool keep = false;

    public:
        ModelBrowser()
        {
        }

        ~ModelBrowser()
        {
            getEditorUI()->destroyUI(VIEWPORT_MODEL_BROWSER);
            pEngine->DeleteEntity(dummy_entity->GetEntityID());
            pEngine->RemoveInputCallback(100);
            delete camera;
        }

        void init(HWND window, Engine* engine)
        {
            pEngine = engine;
            initModelBrowser(window);
            engine->GetRenderer()->SetHighlightingMode(false);
            camera = new Camera();
            engine->GetRenderer()->SetActiveViewport(camera);
            dummy_entity = engine->AddEntity();
            dummy_entity->PositionObjectAt(14000.f, 14000.f, 14000.f);
            dummy_mesh = static_cast<Object*>(dummy_entity->AddNewProperty("Drawable")->GetValueAdress());
            engine->SelectEntity(dummy_entity->GetEntityID());
            engine->AddInputCallback(100, Inputs);

            getEditorUI()->ShowScene();
        }

        void UpdateMaterials(std::string string, int redraw)
        {
            getEditorUI()->UpdateMaterials((char*)string.c_str(), redraw);
        }

        Entity* getDummy()
        {
            return dummy_entity;
        }

        void KeepDummy()
        {
            keep = true;
        }

        static void Inputs()
        {
            constexpr glm::vec3 origin = glm::vec3(14000.f, 14000.f, 14000.f);
            static float rotation = 0;
            Renderer* render = Engine::GetContext()->GetRenderer();
            Object* drawable = Object::FindObject(render->GetSelectedEntity());
            Camera* camera = render->getActiveViewport();

            if (drawable != NULL)
            {
                glm::vec3 axis = glm::vec3(3.f + drawable->GetObjectBounds().x, 3.f + drawable->GetObjectBounds().y, 3.f + drawable->GetObjectBounds().z);

                rotation += GrEngine::Globals::delta_time;
                axis = glm::vec3(axis.z * glm::cos(rotation) + axis.x * glm::sin(rotation), axis.y, axis.z * glm::sin(rotation) - axis.x * glm::cos(rotation));

                camera->SetRotation(glm::lookAt(axis, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
                camera->PositionObjectAt(origin + axis);
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

        void redrawDesigner()
        {
            if (getEditorUI()->wpf_hwnd != nullptr)
            {
                RECT hwin_rect;
                getEditorUI()->SetViewportPosition(VIEWPORT_EDITOR);
            }
        }

        void initModelBrowser(HWND wnd)
        {
            if (!getEditorUI()->InitUI(VIEWPORT_MODEL_BROWSER))
            {
                return;
            }
            getEditorUI()->SetViewportHWND(wnd, 1);
        }
    };
}