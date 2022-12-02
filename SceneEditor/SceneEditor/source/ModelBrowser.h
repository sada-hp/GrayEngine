#pragma once
#include <GrayEngine.h>
#include "EditorUI.h"
#include <chrono>
#include <glfw/glfw3.h>

namespace GrEngine
{
    class ModelBrowser : public Engine
    {
        static ModelBrowser* _browser;
        EditorUI editorUI;
        EntityInfo dummy_entity;

        /*Background Win32 is used to receive messages from WPF front-end window*/
        static LRESULT CALLBACK HostWindowProcBrowser(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            switch (msg)
            {
            case WM_CLOSE:
                ModelBrowser::closeBrowser();
                DestroyWindow(hwnd);
                break;
            case WM_DESTROY:
                break;
            case WM_SIZE:
                ModelBrowser::redrawDesigner();
                break;
                /*Messages received from the C# WPF front-end part of the editor*/
            case 0x1200: //Load obj file callback
                ModelBrowser::createModelFile((const char*)wParam, (const char*)lParam);
                break;
            case 0x1201: //Clear viewport callback
                ModelBrowser::clearViewport();
                break;
            case 0x1202: //Upload texture file callback
                ModelBrowser::uploadTexture((const char*)lParam, (int)wParam);
                break;
            case 0x1203: //Create model using raw files
                ModelBrowser::loadModelFromFile((const char*)wParam);
                break;
            case 0x1205: //Create model using raw files
                ModelBrowser::loadRawModel((const char*)wParam, (const char*)lParam);
                break;
            case 0x1206: //Load model into the scene
                ModelBrowser::sendToTheScene((const char*)wParam);
                break;
            default:
                return DefWindowProcA(hwnd, msg, wParam, lParam);
            }
            return 0;
        }

    public:
        ModelBrowser(const AppParameters& Properties = AppParameters()) : Engine(Properties)
        {
        }

        ~ModelBrowser()
        {
            _browser = nullptr;
        }

        void init(ModelBrowser* instance)
        {
            if (_browser != nullptr)
            {
                delete _browser;
                _browser = nullptr;
            }

            _browser = instance;
            initModelBrowser();

            _browser->addDummy(&dummy_entity);
            _browser->getAppWindow()->getRenderer()->selectEntity(dummy_entity.EntityID);
            _browser->getAppWindow()->AddInputProccess(Inputs);
        }

        static void StartEngine()
        {
            getEditorUI()->ShowScene();
            _browser->Run();
        }

        static void Inputs()
        {
            static float rotation = 0;
            Renderer* render = _browser->getAppWindow()->getRenderer();
            DrawableObject* drawable = (DrawableObject*)render->selectEntity(_browser->dummy_entity.EntityID);
            Camera* camera = render->getActiveViewport();

            if (drawable != NULL)
            {
                glm::vec3 axis = glm::vec3(2.f + drawable->GetObjectBounds().x, 2.f + drawable->GetObjectBounds().y, 2.f + drawable->GetObjectBounds().z);

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

        static void KillEngine()
        {
            _browser->Stop();
        }

        static EditorUI* getEditorUI()
        {
            return &_browser->editorUI;
        }

        static void uploadTexture(const char* image_path, int material_index)
        {
            _browser->loadImageFromPath(image_path, material_index);
        }

        static void sendToTheScene(const char* filepath)
        {
            std::string path = filepath;
            std::vector<double> para;
            for (char chr : path)
            {
                para.push_back(chr);
            }
            EventListener::blockEvents(true, true);
            EventListener::registerEvent("LoadModel", para);
        }

        static void createModelFile(const char* mesh_path, const char* textures_str = nullptr)
        {
            std::string temp_str = "";
            std::vector<std::string> mat_vector;
            std::string mesh = "";
            std::string file = "";

            if (textures_str)
            {
                std::string mats = textures_str;

                for (char chr : mats)
                {
                    if (chr != '|')
                    {
                        temp_str += chr;
                    }
                    else
                    {
                        mat_vector.push_back(temp_str);
                        temp_str = "";
                        continue;
                    }
                }
            }

            for (char chr : std::string(mesh_path))
            {
                if (chr != '|')
                {
                    temp_str += chr;
                }
                else
                {
                    file = temp_str;
                    temp_str = "";
                }
                mesh = temp_str;
            }

            _browser->createModel(file.c_str(), mesh.c_str(), mat_vector);
        }

        static void loadModelFromFile(const char* filepath)
        {
            _browser->Pause(); //Disable rendering for the time it takes to load the model

            std::string mesh_path = "";
            std::vector<std::string> mat_vector;
            std::unordered_map<std::string, std::string> materials;

            Globals::readGMF(filepath, &mesh_path, &mat_vector);

            auto res = _browser->loadModel(mesh_path.c_str(), mat_vector, &materials);
            
            std::string out_materials;
            std::string out_textures;
            for (auto pair : materials)
            {
                out_materials += pair.first + "|";
                out_textures += pair.second + "|";
            }

            _browser->Unpause(); //Resume rendering

            if (res)
                getEditorUI()->UpdateMaterials((char*)out_materials.c_str(), (char*)out_textures.c_str());
        }

        static void loadRawModel(const char* mesh_path, const char* textures_str = nullptr)
        {
            _browser->Pause(); //Disable rendering for the time it takes to load the model

            std::string temp_str = "";
            std::vector<std::string> mat_vector;
            std::unordered_map<std::string, std::string> materials;

            if (textures_str)
            {
                std::string mats = textures_str;

                for (char chr : mats)
                {
                    if (chr != '|')
                    {
                        temp_str += chr;
                    }
                    else
                    {
                        mat_vector.push_back(temp_str);
                        temp_str = "";
                        continue;
                    }
                }
            }

            auto res = _browser->loadModel(mesh_path, mat_vector, &materials);
            std::string out_materials;
            std::string out_textures;
            for (auto pair : materials)
            {
                out_materials += pair.first + "|";
                out_textures += pair.second + "|";
            }

            _browser->Unpause(); //Resume rendering

            if (res)
                getEditorUI()->UpdateMaterials((char*)out_materials.c_str(), (char*)out_textures.c_str());
        }

        static void clearViewport()
        {
            _browser->Pause();
            _browser->clearScene();
            _browser->Unpause();
        }

        static HWND getViewportHWND()
        {
            return reinterpret_cast<HWND>(_browser->getNativeWindow());
        }

        static void redrawDesigner()
        {
            if (getEditorUI()->wpf_hwnd != nullptr)
            {
                RECT hwin_rect;
                GrEngine::ModelBrowser::getEditorUI()->SetViewportPosition(VIEWPORT_EDITOR);
            }
        }

        static void initModelBrowser()
        {
            getEditorUI()->InitUI(VIEWPORT_MODEL_BROWSER);
            getEditorUI()->SetViewportHWND(getViewportHWND(), 1);
        }

        static void closeBrowser()
        {
            getEditorUI()->destroyUI(VIEWPORT_MODEL_BROWSER);
            KillEngine();
        }
    };
}