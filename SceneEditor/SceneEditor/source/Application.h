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

            getAppWindow()->getRenderer()->getActiveViewport()->LockAxes(0, 0, 89, -89, 0, 0);

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
            EnableWindow(getEditorUI()->wpf_hwnd, FALSE);
            Pause();
            Logger::AllowMessages(MessageMode::Block);
            AppParameters props;
            ModelBrowser* mdlBrowser = new ModelBrowser(props);
            EventListener::setEventsPermissions(false, true);
            BindContext(mdlBrowser);
            mdlBrowser->init(mdlBrowser);
            SetForegroundWindow(mdlBrowser->getEditorUI()->wpf_hwnd);
            mdlBrowser->StartEngine();
            mdlBrowser->Stop();
            delete mdlBrowser;
            EventListener::setEventsPermissions(true, true);
            BindContext(this);
            Logger::AllowMessages(MessageMode::Allow);
            Unpause();
            EnableWindow(getEditorUI()->wpf_hwnd, TRUE);
            SetForegroundWindow(getEditorUI()->wpf_hwnd);
        }

        void toggle_free_mode()
        {
            free_mode = !free_mode;
            getAppWindow()->AppShowCursor(!free_mode);
        }

        void App_UpdateEntity(EntityInfo info)
        {
            getEditorUI()->UpdateEntity(info.EntityID, Globals::StringToCharArray(info.EntityName));
        }

        void getEntityInfo(int ID)
        {
            EntityInfo info = getAppWindow()->getRenderer()->getEntityInfo(ID);

            getEditorUI()->SendEntityInfo(info.EntityID, Globals::StringToCharArray(info.EntityName), 
                Globals::StringToCharArray(Globals::FloatToString(info.Position.x, 2) + ":" + Globals::FloatToString(info.Position.y, 2) + ":" + Globals::FloatToString(info.Position.z, 2)),
                Globals::StringToCharArray(Globals::FloatToString(info.Orientation.x, 2) + ":" + Globals::FloatToString(info.Orientation.y, 2) + ":" + Globals::FloatToString(info.Orientation.z, 2)),
                Globals::StringToCharArray(Globals::FloatToString(info.Scale.x, 2) + ":" + Globals::FloatToString(info.Scale.y, 2) + ":" + Globals::FloatToString(info.Scale.z, 2))
            );
            getAppWindow()->getRenderer()->selectEntity(ID);
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
            if (selected_property == "position")
            {
                Entity* selection = getAppWindow()->getRenderer()->selectEntity(ID);
                auto input = GrEngine::Globals::SeparateString(value, ':');
                std::vector<float> coords;

                for (int ind = 0; ind < input.size(); ind++)
                {
                    try
                    {
                        coords.push_back(std::stof(input[ind]));
                    }
                    catch (...)
                    {
                        coords.push_back(selection->GetObjectPosition()[ind]);
                    }
                }

                selection->PositionObjectAt(coords[0], coords[1], coords[2]);
            }
            else if (selected_property == "orientation")
            {
                Entity* selection = getAppWindow()->getRenderer()->selectEntity(ID);
                auto input = GrEngine::Globals::SeparateString(value, ':');
                std::vector<float> coords;

                for (int ind = 0; ind < input.size(); ind++)
                {
                    try
                    {
                        coords.push_back(std::stof(input[ind]));
                    }
                    catch (...)
                    {
                        coords.push_back(selection->GetObjectPosition()[ind]);
                    }
                }

                selection->SetRotation(coords[0], coords[1], coords[2]);
            }
            else if (selected_property == "name")
            {
                Entity* selection = getAppWindow()->getRenderer()->selectEntity(ID);
                selection->UpdateNameTag(value);
            }
            else if (selected_property == "scale")
            {
                Entity* selection = getAppWindow()->getRenderer()->selectEntity(ID);
                auto input = GrEngine::Globals::SeparateString(value, ':');
                std::vector<float> coords;

                for (int ind = 0; ind < input.size(); ind++)
                {
                    try
                    {
                        coords.push_back(std::stof(input[ind]));
                    }
                    catch (...)
                    {
                        coords.push_back(selection->GetObjectPosition()[ind]);
                    }
                }

                selection->SetObjectScale(coords[0], coords[1], coords[2]);
            }
        }
    };
}