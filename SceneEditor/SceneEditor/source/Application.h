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

        Application(const AppParameters& Properties = AppParameters()) : Engine(Properties)
        {
            BindContext(this);
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
            Run();
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
            //getEditorUI()->SetInputMode(VIEWPORT_EDITOR, 0);
            Pause();
            Logger::AllowMessages(MessageMode::Block);
            AppParameters props;
            ModelBrowser* mdlBrowser = new ModelBrowser(props);
            BindContext(mdlBrowser);
            EventListener::setEventsPermissions(false, true);
            GetRenderer()->SetHighlightingMode(false);
            mdlBrowser->init(mdlBrowser);
            mdlBrowser->StartEngine();
            mdlBrowser->Stop();
            delete mdlBrowser;
            GetRenderer()->SetHighlightingMode(true);
            EventListener::setEventsPermissions(true, true);
            BindContext(this);
            Logger::AllowMessages(MessageMode::Allow);
            Unpause();
            //getEditorUI()->SetInputMode(VIEWPORT_EDITOR, 1);
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

        void App_RemoveEntity(UINT id)
        {
            getEditorUI()->RemoveEntity(id);
        }

        void getEntityInfo(int ID)
        {
            auto props = GetRenderer()->selectEntity(ID)->GetProperties();
            int numProps = props.size();

            for (int i = 0; i < numProps; i++)
            {
                getEditorUI()->SendEntityInfo(ID, (char*)props[i]->property_name, (char*)props[i]->ValueString(), (char*)props[i]->TypeString());
            }
        }

        void App_GetAllEntities()
        {
            auto ent_vec = GetRenderer()->GetEntitiesList();

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
    };
}