#pragma once
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <SceneEditor.h>
#include <GrayEngine.h>
#include "EditorUI.h"
#include "ModelBrowser.h"

namespace GrEngine
{
    enum class ChunkType
    {
        FRAMES,
        ENTITY_INFO,
        SELECTION,
        LOG
    };

    struct InfoChunk
    {
        ChunkType type;
        std::string lParam;
        std::string rParam;
    };

    class Application : public Engine
    {
        EditorUI editorUI;
        std::string log_path;
        unsigned char* foliage_mask;
        int mask_width, mask_height, mask_channels;

    public:
        bool free_mode = false;
        uint8_t manipulation = 0;
        POINTFLOAT manip_start;
        Entity* grid;
        Entity* gizmo;
        Entity* brush;
        Entity* transform_target;
        glm::vec2 direct;
        glm::vec2 obj_center;

        Application(const AppParameters& Properties = AppParameters()) : Engine(Properties)
        {
            log_path = getExecutablePath() + std::string("grayengine.log");

            GetRenderer()->getActiveViewport()->LockAxes(0, 0, 89, -89, 0, 0);

            getEditorUI()->InitUI(VIEWPORT_EDITOR);
            getEditorUI()->SetViewportHWND(getViewportHWND(), VIEWPORT_EDITOR);
            Logger::JoinEventListener(GetEventListener());

            foliage_mask = stbi_load("D:\\GrEngine\\GrayEngine\\bin\\Debug-x64\\WorldEditorApp\\Content\\Terrain\\terrain_FM_temp.png", &mask_width, &mask_height, &mask_channels, STBI_rgb_alpha);;
        }

        ~Application()
        {
            if (foliage_mask != nullptr)
            {
                free(foliage_mask);
            }

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

        void LoadScene(const char* path) override
        {
            GetRenderer()->selectEntity(0);
            static_cast<DrawableObject*>(gizmo)->SetVisisibility(false);
            transform_target = nullptr;
            Engine::LoadScene(path);
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

                std::array<byte, 3> rgb = GetRenderer()->GetPixelColorAtCursor();
                if (rgb[0] == 255)
                {
                    manipulation = 1;
                    gizmo->ParsePropertyValue("Color", "255:10:10:255");
                }
                else if (rgb[1] == 255)
                {
                    manipulation = 2;
                    gizmo->ParsePropertyValue("Color", "10:255:10:255");
                }
                else if (rgb[2] == 255)
                {
                    manipulation = 3;
                    gizmo->ParsePropertyValue("Color", "10:10:255:255");
                }
                else if (rgb[0] == 254)
                {
                    manipulation = 4;
                    gizmo->ParsePropertyValue("Color", "255:10:10:255");
                    glm::vec3 dir = glm::normalize(glm::vec3(gizmo->GetObjectTransformation()[0][0], gizmo->GetObjectTransformation()[0][1], gizmo->GetObjectTransformation()[0][2]));
                    pos = pos + dir * glm::vec3(static_cast<DrawableObject*>(gizmo)->GetObjectBounds().x) * gizmo->GetPropertyValue("Scale", glm::vec3(1.f));
                }
                else if (rgb[1] == 254)
                {
                    manipulation = 5;
                    gizmo->ParsePropertyValue("Color", "10:255:10:255");
                    glm::vec3 dir = glm::vec3(gizmo->GetObjectTransformation()[1][0], gizmo->GetObjectTransformation()[1][1], gizmo->GetObjectTransformation()[1][2]);
                    pos = pos + dir * glm::vec3(static_cast<DrawableObject*>(gizmo)->GetObjectBounds().y) * gizmo->GetPropertyValue("Scale", glm::vec3(1.f));
                }
                else if (rgb[2] == 254)
                {
                    manipulation = 6;
                    gizmo->ParsePropertyValue("Color", "10:10:255:255");
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
                AddChunk(ChunkType::SELECTION, "", std::to_string(id));
                transform_target = GetRenderer()->GetSelectedEntity();

                if (transform_target != nullptr && transform_target != gizmo && transform_target != grid && transform_target->GetEntityID() != 100)
                {
                    static_cast<DrawableObject*>(gizmo)->SetVisisibility(true);
                    gizmo->PositionObjectAt(transform_target->GetObjectPosition());
                    gizmo->SetRotation(transform_target->GetObjectOrientation());
                }
                else if (transform_target != nullptr && transform_target->GetEntityID() == 100)
                {
                    manipulation = 7;
                    static_cast<DrawableObject*>(gizmo)->SetVisisibility(false);
                }
                else
                {
                    manipulation = 0;
                    static_cast<DrawableObject*>(gizmo)->SetVisisibility(false);
                }
            }
        }

        void App_PaintMask()
        {
            static auto time = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - time).count();

            int radius = 5;
            if (duration > 35.f)
            {
                glm::vec3 tset = brush->GetObjectPosition();
                int cent_x = (mask_width / 2000.f) * (tset.x + 1000.f);
                int cent_y = mask_height - (mask_height / 2000.f) * (tset.z + 1000.f);

                for (int y = -radius; y <= radius; y++)
                {
                    for (int x = -radius; x <= radius; x++)
                    {
                        if (x * x + y * y <= radius * radius)
                        {
                            int row = (cent_y + y) * mask_width * mask_channels;
                            int col = (cent_x + x) * mask_channels;

                            int red = foliage_mask[row + col] + 10;
                            foliage_mask[row + col] = glm::min(red, 255);
                        }
                    }
                }

                static_cast<Terrain*>(transform_target)->UpdateFoliageMask(foliage_mask);
                time = now;
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
            std::thread mdl(RunModelBrowser);   
            mdl.join();
            GetRenderer()->SetHighlightingMode(true);
            BindContext(this);
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
            AddChunk(ChunkType::LOG, "", message);
        }

        void getEntityInfo(int ID)
        {
            AddChunk(ChunkType::ENTITY_INFO, "", "", ID);
        }

        void App_UpdateUIProperty(const char* property_name)
        {
            AddChunk(ChunkType::ENTITY_INFO, "", "", transform_target->GetEntityID());
        }

        void App_UpdateFrameCounter(float new_value)
        {
            AddChunk(ChunkType::FRAMES, "", Globals::FloatToString(new_value, 1), 1000);
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
            AddChunk(ChunkType::ENTITY_INFO, "", "", ID);
        }

        void UpdateUI()
        {
            SendUIInfo();
        }

        void App_UpdateSphere()
        {
            POINTFLOAT point = GetCursorPosition();
            float depth = GetRenderer()->GetDepthAt(point.x, point.y);

            if (depth > 0.f && !free_mode)
            {
                glm::mat4 model = glm::translate(glm::mat4_cast(GetRenderer()->getActiveViewport()->GetObjectOrientation()), -GetRenderer()->getActiveViewport()->GetObjectPosition());
                glm::mat4 proj = glm::perspective(glm::radians(60.0f), (float)GetWindowSize().x / (float)GetWindowSize().y, 0.1f, 1000.0f);
                glm::vec4 view(0, 0, GetWindowSize().x, GetWindowSize().y);
                proj[1][1] *= -1;
                glm::vec3 tset = glm::unProject(glm::vec3(point.x, point.y, depth), model, proj, view);
                tset += tset - GetRenderer()->getActiveViewport()->GetObjectPosition();
                brush->PositionObjectAt(tset);
            }

        }
    private:
        std::unordered_map<int, InfoChunk> info_chunks;
        std::unordered_map<int, InfoChunk> prev_chunk;
        bool chunk_delivered = false;

        static void RunModelBrowser()
        {
            Logger::AllowMessages(MessageMode::Block);
            AppParameters props;
            ModelBrowser* mdlBrowser = new ModelBrowser(props);
            BindContext(mdlBrowser);
            mdlBrowser->init(mdlBrowser);
            mdlBrowser->StartEngine();
            delete mdlBrowser;
            Logger::AllowMessages(MessageMode::Allow);
        }

        void LoadTools()
        {
            //Grid
            grid = GetRenderer()->addEntity(2000000000);
            static_cast<DrawableObject*>(grid)->DisableCollisions();
            grid->ParsePropertyValue("Shader", "Shaders\\grid");
            static_cast<DrawableObject*>(grid)->GeneratePlaneMesh(1, 1);
            grid->MakeStatic();

            //Move
            gizmo = GetRenderer()->addEntity(2000000001);
            static_cast<DrawableObject*>(gizmo)->DisableCollisions();
            static_cast<DrawableObject*>(gizmo)->LoadMesh((Globals::getExecutablePath() + "Content\\Editor\\ManipulationTool.obj").c_str(), nullptr);
            gizmo->ParsePropertyValue("Shader", "Shaders\\gizmo");
            gizmo->AddNewProperty("Color");
            gizmo->ParsePropertyValue("Color", "1:1:1:1");
            gizmo->AddNewProperty("Scale");
            gizmo->MakeStatic();

            //Paint
            brush = GetRenderer()->addEntity(2000000002);
            brush->ParsePropertyValue("Shader", "Shaders\\brush");
            static_cast<DrawableObject*>(brush)->DisableCollisions();
            static_cast<DrawableObject*>(brush)->LoadMesh((Globals::getExecutablePath() + "Content\\Editor\\PaintingSphere.obj").c_str(), nullptr);
            brush->AddNewProperty("Color");
            brush->ParsePropertyValue("Color", "1:1:1:0.1");
            brush->AddNewProperty("Scale");
            brush->ParsePropertyValue("Scale", "10:10:10");
            brush->MakeStatic();
        }

        void UnloadTools()
        {
            GetRenderer()->DeleteEntity(grid->GetEntityID());
        }

        void AddChunk(ChunkType type, std::string name, std::string value, int chunkID = -1)
        {
            InfoChunk chunk{};
            chunk.type = type;
            chunk.lParam = name;
            chunk.rParam = value;

            if (chunkID < 0)
            {
                info_chunks[info_chunks.size()] = chunk;
            }
            else
            {
                info_chunks[chunkID] = chunk;
            }
        }

        void SendUIInfo()
        {
            static auto time = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - time).count();

            if (duration > 35.f)
            {
                prev_chunk = info_chunks;
                for (std::unordered_map<int, InfoChunk>::iterator itt = prev_chunk.begin(); itt != prev_chunk.end(); ++itt)
                {
                    if ((*itt).second.type == ChunkType::ENTITY_INFO)
                    {
                        sendEntityInfo((*itt).first);
                    }
                    else
                    {
                        getEditorUI()->SendInfoChunk((int)(*itt).second.type, (char*)((*itt).second.lParam.c_str()), (char*)((*itt).second.rParam.c_str()));
                    }
                }

                time = now;
                info_chunks.clear();
            }
        }

        void sendEntityInfo(int ID)
        {
            auto ent = GetRenderer()->selectEntity(ID);
            auto props = ent->GetProperties();
            int numProps = props.size();

            if (ent->GetEntityType() == "Object")
            {
                for (int i = 0; i < numProps; i++)
                {
                    getEditorUI()->SendInfoChunk((int)ChunkType::ENTITY_INFO, (char*)props[i]->property_name, (char*)props[i]->ValueString());
                }
            }
        }
    };
}