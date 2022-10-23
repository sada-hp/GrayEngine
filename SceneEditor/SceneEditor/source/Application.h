#pragma once
#include <SceneEditor.h>
#include <GrayEngine.h>
#include "EditorUI.h"
#include "ModelBrowser.h"
#include <resource.h>

namespace GrEngine
{
    class Application : public Engine
    {
        static Application* _instance;
        EditorUI editorUI;
        std::string log_path;
        bool free_mode = false;
        glm::vec2 old_cursor_pos;

    public:
        static LRESULT CALLBACK HostWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) //Background Win32 is used to receive messages from WPF front-end window
        {
            switch (msg)
            {
            case WM_CLOSE:
                GrEngine::Application::KillEngine();
                GrEngine::Application::getEditorUI()->destroyUI(VIEWPORT_EDITOR);
                DestroyWindow(hwnd);
                break;
            case WM_DESTROY:
                break;
            case WM_SIZE:
                GrEngine::Application::redrawDesiger();
                break;
                /*Messages received from the C# WPF front-end part of the editor*/
            case 0x1119:
                GrEngine::Application::LogMessage((const char*)wParam);
                break;
            case 0x1200: //Load obj file callback
                //GrEngine::Application::loadModel((const char*)lParam);
                break;
            case 0x1201: //Clear viewport callback
                GrEngine::Application::clearViewport();
                break;
            case 0x1202: //Upload texture file callback
                GrEngine::Application::uploadTexture((const char*)lParam);
                break;
            case 0x1203: //Open the model browser
                GrEngine::Application::initModelBrowser();
                break;
            case 0x1204: //Add entity
                GrEngine::Application::addEntity();
                break;
            case 0x1205: //Retrieve entity info
                GrEngine::Application::retrieveEntityInfo((int)wParam);
                break;
            case 0x1206: //Retrieve entity info
                //GrEngine::Application::updateEntity((const char*)wParam, (int)lParam);
                break;
            default:
                return DefWindowProcA(hwnd, msg, wParam, lParam);
            }
            return 0;
        }

        Application(const AppParameters& Properties = AppParameters()) : Engine(Properties)
        {
            if (_instance != nullptr)
                delete _instance;

            _instance = this;
            log_path = getExecutablePath() + std::string("grayengine.log");

            EventListener::pushEvent(EventType::Step, [](std::vector<double> para)
                {
                    if (para.size() > 0)
                    {
                        _instance->editorUI.UpdateFramecounter((float)para[0]);
                    }
                });

            EventListener::pushEvent("LoadModel", [](std::vector<double> para)
                {
                    std::string model_path = "";
                    for (auto chr : para)
                    {
                        model_path += (char)chr;
                    }

                    Application::loadModelFromFile(model_path.c_str());
                });

            EventListener::pushEvent(EventType::KeyPress, [](std::vector<double> para)
                {
                    if (para[0] == GLFW_KEY_ESCAPE && para[2] == GLFW_PRESS)
                    {
                        toggle_free_mode();
                    }
                });

            _instance->getAppWindow()->getRenderer()->getActiveViewport()->LockAxes(0, 0, 89, -89, 0, 0);
            _instance->getAppWindow()->AddInputProccess(Inputs);
            _instance->getAppWindow()->getRenderer()->ShowGrid();
        }

        ~Application()
        {
            TerminateLiraries();
        }

        static void initAppLogger()
        {
            EventListener::pushEvent(EventType::Log, [](std::vector<double> para)
                {
                    Application::pushToAppLogger(para);
                }
            );
        }

        static void LogMessage(const char* msg)
        {
            Logger::Out(msg, OutputColor::Blue, OutputType::Log);
        }

        static void StartEngine()
        {
            _instance->Run();
        }

        static void KillEngine()
        {
            _instance->Stop();
        }

        static EditorUI* getEditorUI()
        {
            return &_instance->editorUI;
        };

        static void loadModelFromFile(const char* filepath)
        {
            std::string mesh_path = "";
            std::vector<std::string> mat_vector;
            std::unordered_map<std::string, std::string> materials;

            Globals::readGMF(filepath, &mesh_path, &mat_vector);
            auto res = _instance->loadModel(mesh_path.c_str(), mat_vector, &materials);
        }

        static void uploadTexture(const char* image_path)
        {
            _instance->loadImageFromPath(image_path);
        }

        static void clearViewport()
        {
            _instance->clearScene();
        }

        static HWND getViewportHWND()
        {
            return reinterpret_cast<HWND>(_instance->getNativeWindow());
        }

        static void redrawDesiger()
        {
            if (getEditorUI()->wpf_hwnd != nullptr)
            {
                RECT hwin_rect;
                GetClientRect(getEditorUI()->cpphwin_hwnd, &hwin_rect);
                MoveWindow(getEditorUI()->wpf_hwnd, 0, 0, hwin_rect.right - hwin_rect.left, hwin_rect.bottom - hwin_rect.top, TRUE);
                GrEngine::Application::getEditorUI()->SetViewportPosition(VIEWPORT_EDITOR);
            }
        }

        static void initModelBrowser()
        {
            Logger::AllowMessages(MessageMode::Block);
            EventListener::blockEvents();
            _instance->isPaused = true;
            getEditorUI()->DisableUIWindow();
            AppParameters props;
            std::unique_ptr<ModelBrowser> mdlBrowser = std::make_unique<ModelBrowser>(props);
            mdlBrowser->init(mdlBrowser.get());
            mdlBrowser->StartEngine();
            mdlBrowser->KillEngine();
            EventListener::blockEvents(true, true);
            Logger::AllowMessages(MessageMode::Allow);
            getEditorUI()->EnableUIWindow();
            _instance->isPaused = false;
        }

        static void toggle_free_mode()
        {
            _instance->free_mode = !_instance->free_mode;
            _instance->getAppWindow()->AppShowCursor(!_instance->free_mode);
            _instance->old_cursor_pos = { 960 * _instance->free_mode, 540 * _instance->free_mode };
            SetCursorPos(960, 540);
        }

        static void addEntity()
        {
            EntityInfo ent;

            _instance->addDummy(&ent);

            char* msg = new char[ent.EntityName.size() + 1];
            int i = 0;
            for (char letter : ent.EntityName)
            {
                msg[i++] = (int)letter;
            }
            msg[i] = '\0';

            getEditorUI()->UpdateEntity(ent.EntityID, msg);
        }

        static void retrieveEntityInfo(int ID)
        {
            EntityInfo info = _instance->getAppWindow()->getRenderer()->getEntityInfo(ID);
            char* msg = new char[info.EntityName.size() + 1];
            int i = 0;
            for (char letter : info.EntityName)
            {
                msg[i++] = (int)letter;
            }
            msg[i] = '\0';

            getEditorUI()->SendEntityInfo(info.EntityID, msg, info.Position.x, info.Position.y, info.Position.z);
            _instance->getAppWindow()->getRenderer()->selectEntity(ID);
        }

        static void Inputs()
        {
            Renderer* render = _instance->getAppWindow()->getRenderer();
            GLFWwindow* window = _instance->getAppWindow()->getWindow();
            Camera* camera = render->getActiveViewport();
            POINT cur{ 1,1 };
            float cameraSpeed = 10;
            glm::vec3 direction{ 0.f };
            glm::vec3 orientation{ 0.f };
            float senstivity = 0.75f;

            if (_instance->getAppWindow()->IsKeyDown(GLFW_KEY_LEFT_SHIFT))
                cameraSpeed = cameraSpeed * 4;

            if (_instance->getAppWindow()->IsKeyDown(GLFW_KEY_W))
                direction.z -= 1;
            if (_instance->getAppWindow()->IsKeyDown(GLFW_KEY_S))
                direction.z += 1;
            if (_instance->getAppWindow()->IsKeyDown(GLFW_KEY_A))
                direction.x -= 1;
            if (_instance->getAppWindow()->IsKeyDown(GLFW_KEY_D))
                direction.x += 1;

            GetCursorPos(&cur);
            if (_instance->free_mode && _instance->old_cursor_pos != glm::vec2{ 0.f })
            {
                if (glm::abs(_instance->old_cursor_pos.x - (float)(cur.x)) > 0.55f)
                {
                    orientation.x -= (_instance->old_cursor_pos.x - (float)cur.x) * senstivity;
                }
                if (glm::abs(_instance->old_cursor_pos.y - (float)cur.y) > 0.55f)
                {
                    orientation.y -= (_instance->old_cursor_pos.y - (float)cur.y) * senstivity;
                }
                SetCursorPos(960, 540);
            }
            else if (_instance->free_mode)
            {
                _instance->old_cursor_pos = glm::vec2{ 960, 540 };
                SetCursorPos(960, 540);
            }

            if (_instance->getAppWindow()->IsKeyDown(GLFW_KEY_UP))
                orientation.y -= 1;
            if (_instance->getAppWindow()->IsKeyDown(GLFW_KEY_DOWN))
                orientation.y += 1;
            if (_instance->getAppWindow()->IsKeyDown(GLFW_KEY_RIGHT))
                orientation.x += 1;
            if (_instance->getAppWindow()->IsKeyDown(GLFW_KEY_LEFT))
                orientation.x -= 1;

            camera->Rotate(orientation);
            camera->MoveObject((direction * camera->GetObjectOrientation()) * cameraSpeed);
        }

        static void pushToAppLogger(std::vector<double> para)
        {
            std::fstream log_file;
            std::string message = "";
            char* msg = new char[para.size() + 2];
            int i = 0;
            for (char letter : para)
            {
                msg[i++] = (int)letter;
            }
            msg[i++] = '\n';
            msg[i] = '\0';


            log_file.open(_instance->log_path, std::fstream::out | std::ios::app);

            log_file << msg;

            log_file.close();

            _instance->editorUI.UpdateLogger(msg);

            delete[] msg;
        }

        static void updateEntity(int ID, std::string selected_property, std::string value)
        {
            if (selected_property == "position")
            {
                Entity* selection = _instance->getAppWindow()->getRenderer()->selectEntity(ID);
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
                Entity* selection = _instance->getAppWindow()->getRenderer()->selectEntity(ID);
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
                Entity* selection = _instance->getAppWindow()->getRenderer()->selectEntity(ID);
                selection->UpdateNameTag(value);
            }
        }
    };
}