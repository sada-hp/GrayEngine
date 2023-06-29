#pragma once
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <SceneEditor.h>
#include <GrayEngine.h>
#include "EditorUI.h"
#include "ModelBrowser.h"
#include "Character.h"

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
        bool was_mask_updated = false;
        std::string fm;
        int mask_width, mask_height, mask_channels;
        ModelBrowser* mdlBrowser;
        Camera* camera;

        std::string returning_string = "";

    public:
        UINT copy_buf = 0;
        std::string loaded_scene_path = "";
        bool focused = true;
        bool free_mode = false;
        bool mouse_down = false;
        bool char_mode = false;
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
        Character* player;

        Application(const AppParameters& Properties = AppParameters()) : Engine(Properties)
        {
            log_path = getExecutablePath() + std::string("grayengine.log");

            getEditorUI()->InitUI(VIEWPORT_EDITOR);
            getEditorUI()->SetViewportHWND(getViewportHWND(), VIEWPORT_EDITOR);
            Logger::JoinEventListener(GetEventListener());
        }

        ~Application()
        {
            if (foliage_mask != nullptr)
            {
                free(foliage_mask);
                foliage_mask = nullptr;
            }

            Stop();
            getEditorUI()->destroyUI(VIEWPORT_EDITOR);
            TerminateLiraries();
        }

        void StartEngine()
        {
            GetRenderer()->getActiveViewport()->LockAxes(89, -89, 0, 0, 0, 0);
            camera = GetRenderer()->getActiveViewport();
            camera->PositionObjectAt(0, 3, 4);
            camera->SetRotation(30, 0, 0);

            GetWindowContext()->AllowResize(false);
            GetWindowContext()->ShowBorder(false);
            redrawDesiger();
            getEditorUI()->ShowScene();
            LoadTools();
            App_GetAllEntities();
            //GetRenderer()->UpdateFogParameters({ {0.15f, 0.125f, 0.15f}, 0.05f, 0.075f });
            GetRenderer()->FarPlane = 1000.f;
            Run();
        }

        void LoadScene(const char* path) override
        {
            static_cast<Object*>(Object::FindObject(gizmo))->SetVisisibility(false);
            if (foliage_mask != nullptr)
            {
                free(foliage_mask);
                foliage_mask = nullptr;
                fm = "";
            }
            Engine::LoadScene(path);
            loaded_scene_path = path;
            if (GetRenderer()->GetEntitiesList().count(100) > 0)
            {
                fm = Globals::getExecutablePath() + static_cast<Terrain*>(GetRenderer()->GetEntitiesList().at(100))->GetBlendMask();
                foliage_mask = stbi_load(fm.c_str(), &mask_width, &mask_height, &mask_channels, STBI_rgb_alpha);
                mask_aspect_x = mask_width / 1024.f;
                mask_aspect_y = mask_height / 1024.f;
            }

            transform_target = nullptr;
            manipulation = 0;
            GetRenderer()->selectEntity(0);
        }

        void App_SaveScene(const char* path)
        {
            SaveScene(path);
            loaded_scene_path = path;

            if (foliage_mask != nullptr && was_mask_updated)
            {
                stbi_write_png(fm.c_str(), mask_width, mask_height, 4, foliage_mask, mask_width * mask_channels);
                was_mask_updated = false;
            }
        }

        void App_SaveScene()
        {
            if (loaded_scene_path == "")
            {
                return;
            }

            SaveScene(loaded_scene_path.c_str());

            if (foliage_mask != nullptr && was_mask_updated)
            {
                stbi_write_png(fm.c_str(), mask_width, mask_height, 4, foliage_mask, mask_width * mask_channels);
                was_mask_updated = false;
            }
        }

        void ClearScene() override
        {
            Engine::ClearScene();
            if (foliage_mask != nullptr)
            {
                free(foliage_mask);
                foliage_mask = nullptr;
            }
            static_cast<Object*>(Object::FindObject(brush))->SetVisisibility(false);
            GetEventListener()->registerEvent("TerrainBlendMask", { false });
            GetEventListener()->registerEvent("TerrainSculptMask", { false });

            copy_buf = 0;
            loaded_scene_path = "";
            manipulation = 0;
            mask_aspect_x = 1;
            mask_aspect_y = 1;
            subdivisions = 1.f;
            mask_width = 0;
            mask_height = 0;
            mask_channels = 0;
            fm = "";
            was_mask_updated = false;

            camera->PositionObjectAt(0, 3, 4);
            camera->SetRotation(30, 0, 0);
        }

        void App_RotateCascade(float pitch, float yaw)
        {
            GrEngine::Entity* casent = GetRenderer()->GetEntitiesOfType(EntityType::CascadeLightEntity)[0];

            glm::quat q = glm::quat_cast(glm::mat3(1.f));
            q = q * glm::angleAxis(glm::radians(yaw), glm::vec3(0, 1, 0));
            q = q * glm::angleAxis(glm::radians(pitch), glm::vec3(1, 0, 0));
            q = q * glm::angleAxis(glm::radians(casent->GetPitchYawRoll().z), glm::vec3(0, 0, 1));
            casent->PositionObjectAt(q * glm::vec3(1, 1, 1));
        }

        void App_ColorCascade(const char* color_string)
        {
            GrEngine::Entity* casent = GetRenderer()->GetEntitiesOfType(EntityType::CascadeLightEntity)[0];
            casent->ParsePropertyValue(PropertyType::Color, color_string);
        }

        void App_AddCascadeEntity()
        {
            GrEngine::Entity* casent = GetRenderer()->addEntity();
            casent->PositionObjectAt(1, 1, 1);
            casent->AddNewProperty(PropertyType::Color);
            casent->AddNewProperty(PropertyType::CascadeLight);
        }

        bool HasCascade()
        {
            return GetRenderer()->GetEntitiesOfType(EntityType::CascadeLightEntity).size() > 0;
        }

        const char* App_GetCascadeProperty(PropertyType prop)
        {
            GrEngine::Entity* casent = GetRenderer()->GetEntitiesOfType(EntityType::CascadeLightEntity)[0];
            returning_string = casent->GetProperty(prop)->ValueString();

            return returning_string.c_str();
        }

        void App_UpdateSelection(UINT id)
        {
            if (id > 0 && id == gizmo->GetEntityID())
            {
                if (GetPhysics()->GetSimulationState()) return;

                SetCursorShape(GLFW_HAND_CURSOR);
                glm::mat4 model = glm::translate(glm::mat4_cast(GetRenderer()->getActiveViewport()->GetObjectOrientation()), -GetRenderer()->getActiveViewport()->GetObjectPosition());
                glm::mat4 proj = glm::perspective(glm::radians(60.0f), (float)GetWindowSize().x / (float)GetWindowSize().y, 1.0f, 100.0f);
                glm::vec4 view(0, 0, GetWindowSize().x, GetWindowSize().y);
                proj[1][1] *= -1;
                auto pos = gizmo->GetObjectPosition();
                Object* gizmo_object = static_cast<Object*>(gizmo->GetProperty("Drawable")->GetValueAdress());

                std::array<byte, 3> rgb = GetRenderer()->GetPixelColorAtCursor();
                if (rgb[0] / 255.f == 1.f)
                {
                    manipulation = 1;
                    gizmo->ParsePropertyValue("Color", "1:0.1:0.1:1");
                }
                else if (rgb[1] / 255.f == 1.f)
                {
                    manipulation = 2;
                    gizmo->ParsePropertyValue("Color", "0.1:1:0.1:1");
                }
                else if (rgb[2] / 255.f == 1.f)
                {
                    manipulation = 3;
                    gizmo->ParsePropertyValue("Color", "0.1:0.1:1:1");
                }
                else if (rgb[0]/255.f >= 0.5f)
                {
                    manipulation = 4;
                    gizmo->ParsePropertyValue("Color", "1:0.1:0.1:1");
                    glm::vec3 dir = glm::normalize(glm::vec3(gizmo->GetObjectTransformation()[0][0], gizmo->GetObjectTransformation()[0][1], gizmo->GetObjectTransformation()[0][2]));
                    pos = pos + dir * (glm::vec3(gizmo_object->GetObjectBounds().x) - glm::vec3(0.1f)) * gizmo->GetPropertyValue("Scale", glm::vec3(1.f));
                }
                else if (rgb[1] / 255.f >= 0.5f)
                {
                    manipulation = 5;
                    gizmo->ParsePropertyValue("Color", "0.1:1:0.1:1");
                    glm::vec3 dir = glm::vec3(gizmo->GetObjectTransformation()[1][0], gizmo->GetObjectTransformation()[1][1], gizmo->GetObjectTransformation()[1][2]);
                    pos = pos + dir * (glm::vec3(gizmo_object->GetObjectBounds().y) - glm::vec3(0.1f)) * gizmo->GetPropertyValue("Scale", glm::vec3(1.f));
                }
                else if (rgb[2] / 255.f >= 0.5f)
                {
                    manipulation = 6;
                    gizmo->ParsePropertyValue("Color", "0.1:0.1:1:1");
                    glm::vec3 dir = glm::vec3(gizmo->GetObjectTransformation()[2][0], gizmo->GetObjectTransformation()[2][1], gizmo->GetObjectTransformation()[2][2]);
                    pos = pos + dir * (glm::vec3(gizmo_object->GetObjectBounds().z) - glm::vec3(0.1f)) * gizmo->GetPropertyValue("Scale", glm::vec3(1.f));
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
                    Object* gizmo_object = static_cast<Object*>(gizmo->GetProperty("Drawable")->GetValueAdress());
                    gizmo_object->SetVisisibility(true);
                    gizmo->PositionObjectAt(transform_target->GetObjectPosition());
                    gizmo->SetRotation(transform_target->GetObjectOrientation());
                }
                else if (transform_target == nullptr || transform_target->GetEntityID() == 100)
                {
                    Object* gizmo_object = static_cast<Object*>(gizmo->GetProperty("Drawable")->GetValueAdress());
                    manipulation = 0;
                    gizmo_object->SetVisisibility(false);
                }
            }
        }

        void App_ShowBrush(int mode)
        {
            transform_target = GetRenderer()->selectEntity(100);
            if (transform_target != nullptr)
            {
                float resolution = static_cast<Terrain*>(transform_target)->GetTerrainSize().resolution;
                subdivisions = 1.f / resolution;

                if (mode == 1 && manipulation != 7)
                {
                    if (foliage_mask == nullptr)
                    {
                        fm = Globals::getExecutablePath() + static_cast<Terrain*>(transform_target)->GetBlendMask();
                        foliage_mask = stbi_load(fm.c_str(), &mask_width, &mask_height, &mask_channels, STBI_rgb_alpha);
                        mask_aspect_x = mask_width / 1024;
                        mask_aspect_y = mask_height / 1024;
                    }

                    if (foliage_mask != nullptr)
                    {
                        transform_target = GetRenderer()->selectEntity(100);
                        static_cast<Object*>(Object::FindObject(brush))->SetVisisibility(true);
                        manipulation = 7;
                        paint_mode = 1;
                        GetEventListener()->registerEvent("TerrainBlendMask", { true });
                        GetEventListener()->registerEvent("TerrainSculptMask", { false });
                        App_SendBrushInfo();
                    }

                }
                else if (mode == 2 && manipulation != 8)
                {
                    static_cast<Object*>(Object::FindObject(brush))->SetVisisibility(true);
                    manipulation = 8;
                    paint_mode = 1;
                    GetEventListener()->registerEvent("TerrainSculptMask", { true });
                    GetEventListener()->registerEvent("TerrainBlendMask", { false });
                    App_SendBrushInfo();
                }
                else
                {
                    static_cast<Object*>(Object::FindObject(brush))->SetVisisibility(false);
                    GetRenderer()->selectEntity(0);
                    manipulation = 0;
                    GetEventListener()->registerEvent("TerrainBlendMask", { false });
                    GetEventListener()->registerEvent("TerrainSculptMask", { false });
                }
            }
            else
            {
                Logger::Out("No valid terrain found in the world!", OutputType::Error);
                GetEventListener()->registerEvent("TerrainSculptMask", { false });
                GetEventListener()->registerEvent("TerrainBlendMask", { false });
                static_cast<Object*>(Object::FindObject(brush))->SetVisisibility(false);
                GetRenderer()->selectEntity(0);
                manipulation = 0;
                return;
            }
        }

        EditorUI* getEditorUI()
        {
            return &editorUI;
        };

        HWND getViewportHWND()
        {
            return static_cast<HWND>(getNativeWindow());
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
            Logger::AllowMessages(MessageMode::Block);
            GetEventListener()->setEventsPermissions(false, true);
            mdlBrowser = new ModelBrowser();
            mdlBrowser->init(getViewportHWND(), this);
            getEditorUI()->SetInputMode(VIEWPORT_EDITOR, 0);
        }

        void closeModelBrowser()
        {
            getEditorUI()->SetInputMode(VIEWPORT_EDITOR, 1);
            getEditorUI()->SetViewportHWND(getViewportHWND(), VIEWPORT_EDITOR);
            getEditorUI()->ShowScene();
            GetRenderer()->SetActiveViewport(camera);
            if (transform_target != nullptr)
            {
                SelectEntity(transform_target->GetEntityID());
            }
            GetEventListener()->pollEngineEvents();
            delete mdlBrowser;
            Logger::AllowMessages(MessageMode::Allow);
            GetRenderer()->SetHighlightingMode(true);
            GetEventListener()->setEventsPermissions(true, true);
        }

        ModelBrowser* GetBrowserContext()
        {
            return mdlBrowser;
        }

        void toggle_free_mode()
        {
            if (char_mode) return;
            free_mode = !free_mode;
            SetCursorState(!free_mode);
        }

        void App_UpdateEntity(Entity* target)
        {
            int id = target->GetEntityID();
            getEditorUI()->UpdateEntity(target->GetEntityID(), (char*)target->GetEntityNameTag());
        }

        void App_RemoveEntity()
        {
            if (transform_target != nullptr && transform_target != gizmo)
            {
                UINT id = transform_target->GetEntityID();
                DeleteEntity(id);
                getEditorUI()->RemoveEntity(id);
                static_cast<GrEngine::Object*>(GrEngine::Object::FindObject(gizmo))->SetVisisibility(false);
            }
        }

        void App_GetAllEntities()
        {
            auto ent_vec = GetRenderer()->GetEntitiesList();

            for (auto obj : ent_vec)
            {
                if ((obj.second->GetEntityType() != EntityType::SkyboxEntity && obj.second->GetEntityType() != EntityType::TerrainEntity) && obj.second->GetEntityType() != EntityType::CascadeLightEntity && !obj.second->IsStatic())
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

        void App_UpdateCopyBuffer()
        {
            if (transform_target != nullptr)
            {
                copy_buf = transform_target->GetEntityID();
            }
        }

        void App_GenerateTerrain(int resolution, int x, int y, int z, std::array<std::string, 6> maps, std::array<std::string, 4> normals, std::array<std::string, 4> displacement)
        {
            if (foliage_mask != nullptr)
            {
                free(foliage_mask);
                foliage_mask = nullptr;
            }

            fm = (Globals::getExecutablePath() + maps[1]);
            foliage_mask = stbi_load(fm.c_str(), &mask_width, &mask_height, &mask_channels, STBI_rgb_alpha);
            mask_aspect_x = mask_width / 1024;
            mask_aspect_y = mask_height / 1024;
            GenerateTerrain(resolution, x, y, z, maps, normals, displacement);
            subdivisions = 1.f / resolution;
        }

        void App_UpdateTerrain(std::array<std::string, 5> maps, std::array<std::string, 4> normals, std::array<std::string, 4> displacement)
        {
            if (GetRenderer()->GetEntitiesList().count(100) > 0)
            {
                Terrain* ter = static_cast<Terrain*>(GetRenderer()->GetEntitiesList().at(100));
                ter->UpdateTextures(maps, normals, displacement);

                if (maps[0] != "")
                {
                    if (foliage_mask != nullptr)
                    {
                        free(foliage_mask);
                        foliage_mask = nullptr;
                    }

                    fm = (Globals::getExecutablePath() + maps[0]);
                    foliage_mask = stbi_load(fm.c_str(), &mask_width, &mask_height, &mask_channels, STBI_rgb_alpha);
                }
            }
        }

        const char* App_GetTerrainMask()
        {
            if (GetRenderer()->GetEntitiesList().count(100) > 0)
            {
                Terrain* ter = static_cast<Terrain*>(GetRenderer()->GetEntitiesList().at(100));
                returning_string = ter->GetBlendMask();
                return returning_string.c_str();
            }
            else
            {
                return "";
            }
        }

        const char* App_GetTerrainColor()
        {
            if (GetRenderer()->GetEntitiesList().count(100) > 0)
            {
                Terrain* ter = static_cast<Terrain*>(GetRenderer()->GetEntitiesList().at(100));
                auto arr = ter->GetColorTextures();
                returning_string = arr[0] + '|' + arr[1] + '|' + arr[2] + '|' + arr[3];
                return returning_string.c_str();
            }
            else
            {
                return "";
            }
        }

        const char* App_GetTerrainNormal()
        {
            if (GetRenderer()->GetEntitiesList().count(100) > 0)
            {
                Terrain* ter = static_cast<Terrain*>(GetRenderer()->GetEntitiesList().at(100));
                auto arr = ter->GetNormalTextures();
                returning_string = arr[0] + '|' + arr[1] + '|' + arr[2] + '|' + arr[3];
                return returning_string.c_str();
            }
            else
            {
                return "";
            }
        }

        const char* App_GetTerrainDispacement()
        {
            if (GetRenderer()->GetEntitiesList().count(100) > 0)
            {
                Terrain* ter = static_cast<Terrain*>(GetRenderer()->GetEntitiesList().at(100));
                auto arr = ter->GetDisplacementTextures();
                returning_string = arr[0] + '|' + arr[1] + '|' + arr[2] + '|' + arr[3];
                return returning_string.c_str();
            }
            else
            {
                return "";
            }
        }

        void App_PaintMask()
        {
            if (mouse_down)
            {
                static bool first_call = true;
                static auto time = std::chrono::steady_clock::now();
                auto now = std::chrono::steady_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - time).count();

                if (duration > 50.f || first_call)
                {
                    Terrain::TerrainSize ter_size = static_cast<Terrain*>(transform_target)->GetTerrainSize();

                    float actual_falloff = brush_falloff * mask_aspect_x * mask_aspect_y;

                    float radius = brush_size * mask_aspect_x;
                    int radiusx = glm::ceil(brush_size * mask_aspect_x + actual_falloff);
                    int radiusy = glm::ceil(brush_size * mask_aspect_y + actual_falloff);

                    int strength = 255 * brush_opacity;
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
                                int red = paint_mode == 1 ? glm::max((int)foliage_mask[row + col], strength * red_channel) : foliage_mask[row + col] - strength * red_channel;
                                int green = paint_mode == 1 ? glm::max((int)foliage_mask[row + col + 1], strength * green_channel) : foliage_mask[row + col + 1] - strength * green_channel;
                                int blue = paint_mode == 1 ? glm::max((int)foliage_mask[row + col + 2], strength * blue_channel) : foliage_mask[row + col + 2] - strength * blue_channel;
                                foliage_mask[row + col] = glm::clamp(red, 0, 255);
                                foliage_mask[row + col + 1] = glm::clamp(green, 0, 255);
                                foliage_mask[row + col + 2] = glm::clamp(blue, 0, 255);
                            }
                            else if (point_sq <= falloff_sq)
                            {
                                int falloff_strength = strength - ((float)(glm::sqrt(point_sq) - (float)radius) * falloff_part);

                                int red = paint_mode == 1 ? glm::max((int)foliage_mask[row + col], falloff_strength * red_channel) : foliage_mask[row + col] - falloff_strength * red_channel;
                                int green = paint_mode == 1 ? glm::max((int)foliage_mask[row + col + 1], falloff_strength * green_channel) : foliage_mask[row + col + 1] - falloff_strength * green_channel;
                                int blue = paint_mode == 1 ? glm::max((int)foliage_mask[row + col + 2], falloff_strength * blue_channel) : foliage_mask[row + col + 2] - falloff_strength * blue_channel;
                                foliage_mask[row + col] = glm::clamp(red, 0, 255);
                                foliage_mask[row + col + 1] = glm::clamp(green, 0, 255);
                                foliage_mask[row + col + 2] = glm::clamp(blue, 0, 255);
                            }
                        }
                    }

                    int two_radx = radiusx * 2;
                    int two_rady = radiusy * 2;
                    static_cast<Terrain*>(transform_target)->UpdateFoliageMask(foliage_mask, two_radx, two_rady, glm::min(glm::max(cent_x - radiusx, 0), mask_width - two_radx), glm::min(glm::max(cent_y - radiusy, 0), mask_height - two_rady));
                    time = now;
                    first_call = false;
                }

                was_mask_updated = true;
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
                POINT vSize = GetWindowSize();
                glm::mat4 model = glm::translate(glm::mat4_cast(GetRenderer()->getActiveViewport()->GetObjectOrientation()), -GetRenderer()->getActiveViewport()->GetObjectPosition());
                glm::mat4 proj = GetRenderer()->getActiveViewport()->GetProjectionMatrix((float)vSize.x / vSize.y, 0.1f, GetRenderer()->FarPlane);
                glm::vec4 view(0, 0, vSize.x, vSize.y);

                glm::vec3 tset = glm::unProject(glm::vec3(point.x, point.y, 0.9f), model, proj, view);
                glm::vec3 dir = glm::normalize(tset - GetRenderer()->getActiveViewport()->GetObjectPosition());
                const RayCastResult res = GetPhysics()->CastRayToObject(GetRenderer()->getActiveViewport()->GetObjectPosition(), tset + dir * 1000.f, 100);

                if (res.hasHit)
                    brush->PositionObjectAt(res.hitPos);
            }
        }

        void App_UpdateBrush(int mode, float opacity, float size, float falloff)
        {
            if (size >= 0)
            {
                brush_size = glm::max(size, 0.1f);
            }
            if (opacity >= 0)
            {
                brush_opacity = glm::max(opacity, 0.1f);
            }
            if (falloff >= 0)
            {
                brush_falloff = glm::max(falloff, 0.1f);
            }
            std::string temp = Globals::FloatToString(brush_size * 2, 5);
            std::string col = Globals::FloatToString(brush_opacity / 2, 5);
            brush->ParsePropertyValue("Scale", (temp + ":" + temp + ":" + temp).c_str());
            brush->ParsePropertyValue("Color", ("1:1:1:" + col).c_str());
            paint_mode = mode > 0 ? mode : paint_mode;
            App_SendBrushInfo();
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

        void App_SetUpCharacter()
        {
            player->PositionObjectAt(GetRenderer()->getActiveViewport()->GetObjectPosition() - glm::vec3(0, 2.5f, 0));
            GetRenderer()->getActiveViewport()->SetParentEntity(player);
            GetRenderer()->getActiveViewport()->PositionObjectAt({ 0, 2.5f, 0 });
            free_mode = false;
        }

        void App_ClearCharacter()
        {
            GetRenderer()->getActiveViewport()->SetParentEntity(nullptr);
            GetRenderer()->getActiveViewport()->PositionObjectAt(glm::vec3{ 0, 2.5f, 0 } + player->GetObjectPosition());
            free_mode = false;
        }

        void App_CloneEntity()
        {
            if (copy_buf != 0)
            {
                auto ent = GetRenderer()->CloneEntity(copy_buf);
                App_UpdateEntity(ent);
                GetRenderer()->selectEntity(ent->GetEntityID());

                POINTFLOAT point = GetCursorPosition();
                float fov = 60.f;
                POINT vSize = GetWindowSize();
                glm::mat4 model = glm::translate(glm::mat4_cast(GetRenderer()->getActiveViewport()->GetObjectOrientation()), -GetRenderer()->getActiveViewport()->GetObjectPosition());
                glm::mat4 proj = GetRenderer()->getActiveViewport()->GetProjectionMatrix((float)vSize.x / vSize.y, 0.1f, GetRenderer()->FarPlane);
                glm::vec4 view(0, 0, vSize.x, vSize.y);

                glm::vec3 tset = glm::unProject(glm::vec3(point.x, point.y, 0.9f), model, proj, view);
                glm::vec3 dir = glm::normalize(tset - GetRenderer()->getActiveViewport()->GetObjectPosition());
                const RayCastResult res = GetPhysics()->CastRayToObject(GetRenderer()->getActiveViewport()->GetObjectPosition(), tset + dir * 1000.f, 100);

                if (res.hasHit)
                {
                    glm::vec3 pos = res.hitPos;
                    ent->PositionObjectAt(pos);
                }
            }
        }

        void App_SnapToGround()
        {
            if (transform_target != nullptr)
            {
                glm::vec3 pos = transform_target->GetObjectPosition();
                RayCastResult res = GetPhysics()->CastRayToObject(pos + glm::vec3(0, 1, 0), pos - glm::vec3(0, 1000, 0), 100);

                if (!res.hasHit)
                {
                    res = GetPhysics()->CastRayToObject(pos - glm::vec3(0, 1, 0), pos + glm::vec3(0, 1000, 0), 100);
                    res.hitNorm = -res.hitNorm;
                }

                if (res.hasHit)
                {
                    transform_target->PositionObjectAt(res.hitPos);
                    //transform_target->Rotate(glm::angleAxis(res.hitNorm.x, glm::vec3(1, 0, 0)));
                    //transform_target->Rotate(glm::angleAxis(res.hitNorm.y, glm::vec3(0, 1, 0)));
                    //transform_target->Rotate(glm::angleAxis(res.hitNorm.z, glm::vec3(0, 0, 1)));
                    float x = glm::acos(glm::dot(res.hitNorm, glm::vec3(1, 0, 0)));
                    float y = glm::acos(glm::dot(res.hitNorm, glm::vec3(0, 1, 0)));
                    float z = glm::acos(glm::dot(res.hitNorm, glm::vec3(0, 0, 1)));
                    transform_target->SetRotation(90.f - glm::degrees(z), glm::degrees(y), glm::degrees(x) - 90.f);
                }
            }
        }

        void App_CreateEmptyImage(const char* filepath, int width, int height)
        {
            void* pixels = malloc(width * height * 4);
            memset(pixels, 0, width * height * 4);
            stbi_write_png(filepath, width, height, 4, pixels, width * 4);
            free(pixels);
        }

        void App_SetSkyColor(float r, float g, float b)
        {
            std::vector<Entity*> ent = GetRenderer()->GetEntitiesOfType(EntityType::SkyboxEntity);

            if (ent.size() > 0)
            {
                std::string color = std::to_string(r) + ":" + std::to_string(g) + ":" + std::to_string(b) + ":1";
                Skybox* sky = static_cast<Skybox*>(ent[0]);
                sky->ParsePropertyValue(PropertyType::Color, color.c_str());
            }
        }

        const char* App_GetSkyColor()
        {
            returning_string = "1:1:1:1";
            std::vector<Entity*> ent = GetRenderer()->GetEntitiesOfType(EntityType::SkyboxEntity);

            if (ent.size() > 0)
            {
                Skybox* sky = static_cast<Skybox*>(ent[0]);
                returning_string = sky->GetProperty(PropertyType::Color)->ValueString();
            }

            return returning_string.c_str();
        }
        
        void App_ResetTools()
        {
            manipulation = 0;
            static_cast<Object*>(Object::FindObject(brush))->SetVisisibility(false);
            GetEventListener()->registerEvent("TerrainBlendMask", { false });
            GetEventListener()->registerEvent("TerrainSculptMask", { false });
            SelectEntity(0);
        }

    private:
        std::unordered_map<int, InfoChunk> info_chunks;
        std::unordered_map<int, InfoChunk> prev_chunk;
        bool chunk_delivered = false;

        void LoadTools()
        {
            //Gizmo
            gizmo = GetRenderer()->addEntity(900900);
            Object* gizmo_object = static_cast<Object*>(gizmo->AddNewProperty(PropertyType::Drawable)->GetValueAdress());
            gizmo_object->DisableCollisions();
            gizmo_object->SetVisisibility(false);
            gizmo_object->LoadMesh("Content\\Editor\\ManipulationTool.obj");
            gizmo->ParsePropertyValue(PropertyType::Shader, "Shaders\\gizmo");
            gizmo->AddNewProperty(PropertyType::Color);
            gizmo->ParsePropertyValue(PropertyType::Color, "1:1:1:1");
            gizmo->AddNewProperty(PropertyType::Scale);
            gizmo->AddNewProperty(PropertyType::CastShadow);
            gizmo->ParsePropertyValue(PropertyType::CastShadow, "0");
            gizmo->MakeStatic();

            //Grid
            grid = GetRenderer()->addEntity(900901);
            Object* grid_object = static_cast<Object*>(grid->AddNewProperty(PropertyType::Drawable)->GetValueAdress());
            grid_object->GeneratePlaneMesh(1, 1);
            grid_object->DisableCollisions();
            //grid_object->SetVisisibility(false);
            grid->AddNewProperty(PropertyType::Transparency);
            grid->ParsePropertyValue(PropertyType::Transparency, "1");
            grid->ParsePropertyValue(PropertyType::Shader, "Shaders\\grid");
            grid->AddNewProperty(PropertyType::CastShadow);
            grid->ParsePropertyValue(PropertyType::CastShadow, "0");
            grid->MakeStatic();

            //Paint
            brush = GetRenderer()->addEntity(900902);
            Object* brush_object = static_cast<Object*>(brush->AddNewProperty(PropertyType::Drawable)->GetValueAdress());
            brush->AddNewProperty(PropertyType::Transparency);
            brush->ParsePropertyValue(PropertyType::Transparency, "1");
            brush->ParsePropertyValue(PropertyType::Shader, "Shaders\\brush");
            brush_object->DisableCollisions();
            brush_object->SetVisisibility(false);
            brush_object->GenerateSphereMesh(1, 32, 32);
            brush->AddNewProperty(PropertyType::Color);
            brush->ParsePropertyValue(PropertyType::Color, "1:1:1:0.5");
            brush->AddNewProperty(PropertyType::Scale);
            brush->ParsePropertyValue(PropertyType::Scale, "2:2:2");
            brush->AddNewProperty(PropertyType::CastShadow);
            brush->ParsePropertyValue(PropertyType::CastShadow, "0");
            brush->MakeStatic();

            player = new Character(900903);
            player->MakeStatic();
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

            if (ent->GetEntityType() != EntityType::SkyboxEntity && ent->GetEntityType() != EntityType::TerrainEntity && ent->GetEntityType() != EntityType::CascadeLightEntity)
            {
                for (int i = 0; i < numProps; i++)
                {
                    getEditorUI()->SendInfoChunk((int)ChunkType::ENTITY_INFO, (char*)props[i]->PropertyNameString(), (char*)props[i]->ValueString());
                }
            }
        }
    };
};