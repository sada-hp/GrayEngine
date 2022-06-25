#pragma once
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

            _instance->getAppWindow()->inputs_vector.push_back(Inputs);
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

            Renderer::readGMF(filepath, &mesh_path, &mat_vector);
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
            getEditorUI()->DisableUIWindow();
            AppParameters props;
            std::unique_ptr<ModelBrowser> mdlBrowser = std::make_unique<ModelBrowser>(props);
            mdlBrowser->init(mdlBrowser.get());
            mdlBrowser->StartEngine();
            mdlBrowser->KillEngine();
            getEditorUI()->EnableUIWindow();
        }

        static void toggle_free_mode()
        {
            _instance->free_mode = !_instance->free_mode;
            _instance->getAppWindow()->AppShowCursor(!_instance->free_mode);
            _instance->getAppWindow()->getRenderer()->viewport_camera.cursor_pos = {960 * !_instance->free_mode, 540 * !_instance->free_mode };
        }

        static void Inputs()
        {
            Renderer* render = _instance->getAppWindow()->getRenderer();
            GLFWwindow* window = _instance->getAppWindow()->getWindow();
            POINT cur{ 1,1 };
            float cameraSpeed = 0.25;
            glm::vec3 direction{ 0.f };
            float senstivity = 0.75f;

            if (_instance->getAppWindow()->IsKeyDown(GLFW_KEY_LEFT_SHIFT))
                cameraSpeed = 0.5f;

            if (_instance->getAppWindow()->IsKeyDown(GLFW_KEY_W))
                direction.z -= 1;
            if (_instance->getAppWindow()->IsKeyDown(GLFW_KEY_S))
                direction.z += 1;
            if (_instance->getAppWindow()->IsKeyDown(GLFW_KEY_A))
                direction.x -= 1;
            if (_instance->getAppWindow()->IsKeyDown(GLFW_KEY_D))
                direction.x += 1;

            GetCursorPos(&cur);
            if (_instance->free_mode && render->viewport_camera.cursor_pos != glm::vec2{ 0.f })
            {
                if (glm::abs(render->viewport_camera.cursor_pos.x - (float)(cur.x)) > 0.15f)
                {
                    render->viewport_camera.cam_rot.x -= (render->viewport_camera.cursor_pos.x - (float)cur.x) * senstivity;
                }
                if (glm::abs(render->viewport_camera.cursor_pos.y - (float)cur.y) > 0.15f)
                {
                    render->viewport_camera.cam_rot.y -= (render->viewport_camera.cursor_pos.y - (float)cur.y) * senstivity;
                }
                SetCursorPos(960, 540);
            }
            else if (_instance->free_mode)
            {
                render->viewport_camera.cursor_pos = glm::vec2{ 960, 540 };
                SetCursorPos(960, 540);
            }

            if (_instance->getAppWindow()->IsKeyDown(GLFW_KEY_UP))
                render->viewport_camera.cam_rot.x -= 1;
            if (_instance->getAppWindow()->IsKeyDown(GLFW_KEY_DOWN))
                render->viewport_camera.cam_rot.x += 1;
            if (_instance->getAppWindow()->IsKeyDown(GLFW_KEY_RIGHT))
                render->viewport_camera.cam_rot.y += 1;
            if (_instance->getAppWindow()->IsKeyDown(GLFW_KEY_LEFT))
                render->viewport_camera.cam_rot.y -= 1;

            if (render->viewport_camera.cam_rot.y > 89.f)
                render->viewport_camera.cam_rot.y = 89.f;
            if (render->viewport_camera.cam_rot.y < -89.f)
                render->viewport_camera.cam_rot.y = -89.f;

            glm::quat qPitch = glm::angleAxis(glm::radians(render->viewport_camera.cam_rot.y), glm::vec3(1, 0, 0));
            glm::quat qYaw = glm::angleAxis(glm::radians(render->viewport_camera.cam_rot.x), glm::vec3(0, 1, 0));
            glm::quat qRoll = glm::angleAxis(glm::radians(render->viewport_camera.cam_rot.z), glm::vec3(0, 0, 1));

            render->viewport_camera.cam_orientation = glm::normalize(qPitch * qYaw * qRoll);
            render->viewport_camera.cam_pos += (direction * render->viewport_camera.cam_orientation) * cameraSpeed;
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
    };
}