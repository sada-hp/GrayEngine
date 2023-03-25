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
        bool mouse_down = false;
        uint8_t manipulation = 0;
        POINTFLOAT manip_start;
        Entity* grid;
        Entity* gizmo;
        Entity* brush;
        Entity* transform_target;
        glm::vec2 direct;
        glm::vec2 obj_center;
        float brush_size = 1;
        float brush_opacity = 1;
        float brush_falloff = 1;
        std::string brush_opacity_string = "";
        std::string brush_size_string = "";
        std::string brush_falloff_string = "";
        bool ctr_down = false;
        int paint_mode = 1;
        bool red_channel = true;
        bool green_channel = false;
        bool blue_channel = false;
        float mask_aspect_x = 1;
        float mask_aspect_y = 1;
        float subdivisions = 1.f;

        bool brush_snap = false;

        Application(const AppParameters& Properties = AppParameters()) : Engine(Properties)
        {
            log_path = getExecutablePath() + std::string("grayengine.log");

            GetRenderer()->getActiveViewport()->LockAxes(0, 0, 89, -89, 0, 0);

            getEditorUI()->InitUI(VIEWPORT_EDITOR);
            getEditorUI()->SetViewportHWND(getViewportHWND(), VIEWPORT_EDITOR);
            Logger::JoinEventListener(GetEventListener());
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
            if (id >0 && id == gizmo->GetEntityID())
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
            else if (manipulation != 7 && manipulation != 8)
            {
                AddChunk(ChunkType::SELECTION, "", std::to_string(id));
                transform_target = GetRenderer()->GetSelectedEntity();

                if (transform_target != nullptr && transform_target != gizmo && transform_target != grid && transform_target->GetEntityID() != 100)
                {
                    static_cast<DrawableObject*>(gizmo)->SetVisisibility(true);
                    gizmo->PositionObjectAt(transform_target->GetObjectPosition());
                    gizmo->SetRotation(transform_target->GetObjectOrientation());
                }
                else if (transform_target == nullptr || transform_target->GetEntityID() == 100)
                {
                    manipulation = 0;
                    static_cast<DrawableObject*>(gizmo)->SetVisisibility(false);
                }
            }
        }

        void App_ShowBrush(int mode)
        {
            if (mode == 1 && manipulation != 7)
            {
                if (foliage_mask != nullptr)
                {
                    static_cast<DrawableObject*>(brush)->SetVisisibility(true);
                    transform_target = GetRenderer()->selectEntity(100);
                    manipulation = 7;
                    paint_mode = 1;
                    GetEventListener()->registerEvent("TerrainBlendMask", { true });
                    GetEventListener()->registerEvent("TerrainSculptMask", { false });
                    App_SendBrushInfo();
                }
                else
                {
                    Logger::Out("Invalid foliage mask selected!", OutputColor::Red, OutputType::Error);
                    GetEventListener()->registerEvent("TerrainSculptMask", { false });
                    GetRenderer()->selectEntity(0);
                    manipulation = 0;
                    return;
                }
            }
            else if (mode == 2 && manipulation != 8)
            {
                static_cast<DrawableObject*>(brush)->SetVisisibility(true);
                transform_target = GetRenderer()->selectEntity(100);
                manipulation = 8;
                paint_mode = 1;
                GetEventListener()->registerEvent("TerrainSculptMask", { true });
                GetEventListener()->registerEvent("TerrainBlendMask", { false });
                App_SendBrushInfo();
            }
            else
            {
                static_cast<DrawableObject*>(brush)->SetVisisibility(false);
                GetRenderer()->selectEntity(0);
                manipulation = 0;
                GetEventListener()->registerEvent("TerrainBlendMask", { false });
                GetEventListener()->registerEvent("TerrainSculptMask", { false });
            }

            FocusViewport();
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
                if (obj.second->GetEntityType() == "Object" && !obj.second->IsStatic())
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

        void App_GenerateTerrain(int resolution, int x, int y, int z, std::array<std::string, 6> maps)
        {
            if (foliage_mask != nullptr)
            {
                free(foliage_mask);
                foliage_mask = nullptr;
            }

            foliage_mask = stbi_load(maps[1].c_str(), &mask_width, &mask_height, &mask_channels, STBI_rgb_alpha);;
            mask_aspect_x = mask_width / 1024;
            mask_aspect_y = mask_height / 1024;
            GenerateTerrain(resolution, x, y, z, maps);
            subdivisions = 1.f / resolution;
        }

        void App_PaintMask()
        {
            if (mouse_down)
            {
                static bool first_call = true;
                static auto time = std::chrono::steady_clock::now();
                auto now = std::chrono::steady_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - time).count();

                if (duration > 100.f || first_call)
                {
                    Terrain::TerrainSize ter_size = static_cast<Terrain*>(transform_target)->GetTerrainSize();

                    int actual_falloff = brush_falloff * mask_aspect_x * mask_aspect_y;

                    float falloff = glm::ceil(actual_falloff);
                    int radius = glm::ceil(brush_size * mask_aspect_x);
                    int radiusx = glm::ceil(brush_size * mask_aspect_x) + falloff;
                    int radiusy = glm::ceil(brush_size * mask_aspect_y) + falloff;

                    int sign = paint_mode == 1 ? 1 : -1;
                    int strength = 255 * brush_opacity * paint_mode * sign;
                    int falloff_part = strength / actual_falloff;

                    glm::vec3 tset = brush->GetObjectPosition();
                    int cent_x = (mask_width / (float)ter_size.width) * (tset.x + (float)ter_size.width/2);
                    int cent_y = mask_height - (mask_height / (float)ter_size.depth) * (tset.z + (float)ter_size.depth/2);

                    float radius_sq = brush_size * brush_size * mask_aspect_x * mask_aspect_y;
                    float falloff_sq = ((brush_size * mask_aspect_x) + actual_falloff) * ((brush_size * mask_aspect_y) + actual_falloff);
                    int limit = mask_width * mask_height * mask_channels;

                    for (int y = -radiusy; y <= radiusy; y++)
                    {
                        for (int x = -radiusx; x <= radiusx; x++)
                        {
                            float point_sq = x * x + y * y;
                            int row = (cent_y + y) * mask_width * mask_channels;
                            int col = (cent_x + x) * mask_channels;

                            if (row + col + 2 >= limit || row + col < 0)
                                continue;

                            if (point_sq <= radius_sq)
                            {
                                int red = foliage_mask[row + col] + strength * red_channel;
                                int green = foliage_mask[row + col + 1] + strength * green_channel;
                                int blue = foliage_mask[row + col + 2] + strength * blue_channel;
                                foliage_mask[row + col] = glm::max(glm::min(red, 255), 0);
                                foliage_mask[row + col + 1] = glm::max(glm::min(green, 255), 0);
                                foliage_mask[row + col + 2] = glm::max(glm::min(blue, 255), 0);
                            }
                            else if (point_sq <= falloff_sq)
                            {
                                int falloff_strength = (strength - ((float)(glm::sqrt(point_sq) - (float)radius) * falloff_part));

                                int red = foliage_mask[row + col] + falloff_strength * red_channel;
                                int green = foliage_mask[row + col + 1] + falloff_strength * green_channel;
                                int blue = foliage_mask[row + col + 2] + falloff_strength * blue_channel;
                                foliage_mask[row + col] = glm::max(glm::min(red, 255), 0);
                                foliage_mask[row + col + 1] = glm::max(glm::min(green, 255), 0);
                                foliage_mask[row + col + 2] = glm::max(glm::min(blue, 255), 0);
                            }
                        }
                    }

                    int two_radx = radiusx * 2;
                    int two_rady = radiusy * 2;
                    static_cast<Terrain*>(transform_target)->UpdateFoliageMask(foliage_mask, two_radx, two_rady, glm::min(glm::max(cent_x - radiusx, 0), mask_width - two_radx), glm::min(glm::max(cent_y - radiusy, 0), mask_height - two_rady));
                    time = now;
                    first_call = false;
                }
            }
        }

        void App_Sculpt()
        {
            if (mouse_down)
            {
                static bool first_call = true;
                static auto time = std::chrono::steady_clock::now();
                auto now = std::chrono::steady_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - time).count();

                if (duration > 100.f || first_call)
                {
                    Terrain::TerrainSize ter_size = static_cast<Terrain*>(transform_target)->GetTerrainSize();
                    float strength = paint_mode == 2 ? -brush_opacity : brush_opacity;
                    glm::vec3 tset = brush->GetObjectPosition();

                    float falloff = glm::ceil(brush_falloff);
                    int radiusx = (brush_size * 2) / (subdivisions * ter_size.width) + falloff;
                    int radiusy = (brush_size * 2) / (subdivisions * ter_size.depth) + falloff;

                    float x_sub = ter_size.width * subdivisions;
                    float z_sub = ter_size.depth * subdivisions;
                    int row = 0.5f + (tset.x + ter_size.width/2) / x_sub;
                    int col = 0.5f + (tset.z + ter_size.depth/2) / z_sub;
                    std::map<UINT, float> offsets;
                    UINT limit = ter_size.resolution * ter_size.resolution;

                    float radius_sq = (brush_size * 2) / (subdivisions * ter_size.width) * (brush_size * 2) / (subdivisions * ter_size.depth);
                    float falloff_sq = ((brush_size * 2) / (subdivisions * ter_size.width) + brush_falloff) * ((brush_size * 2) / (subdivisions * ter_size.depth) + brush_falloff);
                    int falloff_part = strength / falloff;

                    for (int y = -radiusy; y <= radiusy; y++)
                    {
                        for (int x = -radiusx; x <= radiusx; x++)
                        {
                            float point_sq = x * x + y * y;
                            int index = (col + y) * ter_size.resolution + row + x;

                            if (index >= limit || index < 0)
                                continue;

                            if (point_sq <= radius_sq)
                            {
                                if (paint_mode == 4)
                                {
                                    int limit = ter_size.resolution * ter_size.resolution;
                                    float height = static_cast<Terrain*>(transform_target)->GetVertexPosition(index).y;

                                    std::vector<float> neighbors;
                                    if (index - 1 >= 0)
                                        neighbors.push_back(static_cast<Terrain*>(transform_target)->GetVertexPosition(index - 1).y);
                                    if (index + 1 < limit)
                                        neighbors.push_back(static_cast<Terrain*>(transform_target)->GetVertexPosition(index + 1).y);
                                    if (index - ter_size.resolution >= 0)
                                        neighbors.push_back(static_cast<Terrain*>(transform_target)->GetVertexPosition(index - ter_size.resolution).y);
                                    if (index + ter_size.resolution < limit)
                                        neighbors.push_back(static_cast<Terrain*>(transform_target)->GetVertexPosition(index + ter_size.resolution).y);

                                    float average = height;
                                    for (std::vector<float>::iterator itt = neighbors.begin(); itt != neighbors.end(); ++itt)
                                    {
                                        average += *itt;
                                    }
                                    average = average / (neighbors.size() + 1);
                                    float offset = (average - height) * glm::min(strength, 1.f);
                                    offsets[index] = offset;
                                }
                                else
                                {
                                    offsets[index] = strength;
                                }
                            }
                            else if (point_sq <= falloff_sq && paint_mode < 3)
                            {
                                int falloff_strength = strength - ((float)(glm::sqrt(point_sq) - (float)radiusx) * falloff_part);
                                offsets[index] = falloff_strength;
                            }
                            else if (point_sq <= falloff_sq && paint_mode == 3)
                            {
                                float falloff_strength = (falloff - ((float)(glm::sqrt(point_sq) - (float)radiusx))) / falloff;
                                float height = static_cast<Terrain*>(transform_target)->GetVertexPosition(index).y;
                                float dist = strength - height;
                                offsets[index] = height + dist * falloff_strength/2;
                            }
                        }
                    }

                    switch (paint_mode)
                    {
                    case 3:
                        static_cast<Terrain*>(transform_target)->UpdateVertices(offsets);
                        break;
                    default:
                        static_cast<Terrain*>(transform_target)->OffsetVertices(offsets);
                        break;
                    }
                    time = now;
                    first_call = false;
                }
            }
        }

        void App_RecalculateTerrain()
        {
            static_cast<Terrain*>(GetRenderer()->selectEntity(100))->UpdateCollision();
        }

        void App_UpdateSphere()
        {
            if (manipulation >= 7 && transform_target != nullptr && !free_mode && transform_target->GetEntityID() == 100)
            {
                POINTFLOAT point = GetCursorPosition();
                float fov = 60.f;
                POINT vSize = GetWindowSize();
                glm::mat4 model = glm::translate(glm::mat4_cast(GetRenderer()->getActiveViewport()->GetObjectOrientation()), -GetRenderer()->getActiveViewport()->GetObjectPosition());
                glm::mat4 proj = glm::perspective(glm::radians(fov), (float)vSize.x / (float)vSize.y, 0.1f, 1000.0f);
                glm::vec4 view(0, 0, vSize.x, vSize.y);
                proj[1][1] *= -1;

                glm::vec3 tset = glm::unProject(glm::vec3(point.x, point.y, 0.9f), model, proj, view);
                glm::vec3 dir = glm::normalize(tset - GetRenderer()->getActiveViewport()->GetObjectPosition());
                const RayCastResult res = GetPhysics()->CastRayToObject(GetRenderer()->getActiveViewport()->GetObjectPosition(), tset + dir * 1000.f, 100);

                if (res.hasHit)
                    brush->PositionObjectAt(res.hitPos);
            }
        }

        void App_UpdateBrush(int mode, float opacity, float size, float falloff)
        {
            brush_size = size > 0 ? size : brush_size;
            brush_opacity = opacity > 0 ? opacity : brush_opacity;
            brush_falloff = falloff > 0 ? falloff : brush_falloff;
            std::string temp = Globals::FloatToString(brush_size * 2, 5);
            std::string col = Globals::FloatToString(brush_opacity / 2, 5);
            brush->ParsePropertyValue("Scale", (temp + ":" + temp + ":" + temp).c_str());
            brush->ParsePropertyValue("Color", ("1:1:1:" + col).c_str());
            paint_mode = mode > 0 ? mode : paint_mode;
        }

        void App_SendBrushInfo()
        {
            brush_opacity_string = Globals::FloatToString(brush_opacity, 5);
            brush_size_string = Globals::FloatToString(brush_size, 5);
            brush_falloff_string = Globals::FloatToString(brush_falloff, 5);
            getEditorUI()->SendInfoChunk((int)ChunkType::ENTITY_INFO, (char*)"Opacity", (char*)brush_opacity_string.c_str());
            getEditorUI()->SendInfoChunk((int)ChunkType::ENTITY_INFO, (char*)"Size", (char*)brush_size_string.c_str());
            getEditorUI()->SendInfoChunk((int)ChunkType::ENTITY_INFO, (char*)"Falloff", (char*)brush_falloff_string.c_str());
        }

        void App_SetActiveChannels(bool red, bool green, bool blue)
        {
            red_channel = red;
            green_channel = green;
            blue_channel = blue;
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
            grid = GetRenderer()->addEntity();
            static_cast<DrawableObject*>(grid)->DisableCollisions();
            grid->ParsePropertyValue("Transparency", "1");
            //static_cast<DrawableObject*>(grid)->SetVisisibility(false);
            grid->ParsePropertyValue("Shader", "Shaders\\grid");
            static_cast<DrawableObject*>(grid)->GeneratePlaneMesh(1, 1);
            grid->MakeStatic();

            //Move
            gizmo = GetRenderer()->addEntity();
            static_cast<DrawableObject*>(gizmo)->DisableCollisions();
            static_cast<DrawableObject*>(gizmo)->SetVisisibility(false);
            static_cast<DrawableObject*>(gizmo)->LoadMesh("Content\\Editor\\ManipulationTool.obj", nullptr);
            gizmo->ParsePropertyValue("Shader", "Shaders\\gizmo");
            gizmo->AddNewProperty("Color");
            gizmo->ParsePropertyValue("Color", "1:1:1:1");
            gizmo->AddNewProperty("Scale");
            gizmo->MakeStatic();

            //Paint
            brush = GetRenderer()->addEntity();
            brush->ParsePropertyValue("Transparency", "2");
            brush->ParsePropertyValue("Shader", "Shaders\\brush");
            static_cast<DrawableObject*>(brush)->DisableCollisions();
            static_cast<DrawableObject*>(brush)->SetVisisibility(false);
            static_cast<DrawableObject*>(brush)->LoadMesh("Content\\Editor\\PaintingSphere.obj", nullptr);
            brush->AddNewProperty("Color");
            brush->ParsePropertyValue("Color", "1:1:1:0.5");
            brush->AddNewProperty("Scale");
            brush->ParsePropertyValue("Scale", "2:2:2");
            brush->MakeStatic();

            //auto shrek = GetRenderer()->addEntity();
            //static_cast<DrawableObject*>(shrek)->LoadModel((Globals::getExecutablePath() + "Content\\Shrek\\shrek.gmf").c_str());
            //shrek->MoveObject(-1, -4, 0);
            //shrek->Rotate(0, 15, 0);

            //GetRenderer()->getActiveViewport()->Rotate(-90, 0, 0);
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