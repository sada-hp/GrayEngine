#include "UIBridge.h"

void SceneEditor::LogMessage(const char* msg)
{
	Logger::Out(msg, OutputType::Log);
}

void SceneEditor::InitModelBrowser()
{
	SceneEditor::GetApplication()->initModelBrowser();
}

void SceneEditor::CloseModelBrowser()
{
    SceneEditor::GetApplication()->closeModelBrowser();
}

void SceneEditor::AddEntity()
{
    GrEngine::Entity* ent = SceneEditor::GetApplication()->GetContext()->AddEntity();
    SceneEditor::GetApplication()->App_UpdateEntity(ent);
	SceneEditor::GetApplication()->GetContext()->SelectEntity(ent->GetEntityID());
}

void SceneEditor::UpdateEntityProperty(int ID, const char* selected_property, const char* value)
{
	SceneEditor::GetApplication()->updateEntity(ID, selected_property, value);
}

void SceneEditor::UpdateSkybox(const char* East, const char* West, const char* Top, const char* Bottom, const char* North, const char* South)
{
    std::string solution = GrEngine::Globals::getExecutablePath();
    if ((std::string(East) != "" && std::string(East).substr(0, solution.size()) != solution) || (std::string(West) != "" && std::string(West).substr(0, solution.size()) != solution)
        || (std::string(Top) != "" && std::string(Top).substr(0, solution.size()) != solution) || (std::string(Bottom) != "" && std::string(Bottom).substr(0, solution.size()) != solution)
        || (std::string(North) != "" && std::string(North).substr(0, solution.size()) != solution) || (std::string(South) != "" && std::string(South).substr(0, solution.size()) != solution))
    {
        Logger::Out("Resource outside the solution is being used!", OutputType::Error);
        return;
    }

    GrEngine::Engine::GetContext()->LoadSkybox(std::string(East).erase(0, solution.size()).c_str(), std::string(West).erase(0, solution.size()).c_str(),
        std::string(Top).erase(0, solution.size()).c_str(), std::string(Bottom).erase(0, solution.size()).c_str(),
        std::string(North).erase(0, solution.size()).c_str(), std::string(South).erase(0, solution.size()).c_str());
}

void SceneEditor::GetEntityInfo(int ID)
{
	SceneEditor::GetApplication()->getEntityInfo(ID);
}

void SceneEditor::GetEntitiesList()
{
    SceneEditor::GetApplication()->App_GetAllEntities();
}

void SceneEditor::LoadModelFile(const char* model_path)
{
    std::string solution = GrEngine::Globals::getExecutablePath();
    if (std::string(model_path).substr(0, solution.size()) != solution)
    {
        Logger::Out("Resource outside the solution is being used! %s", OutputType::Error, model_path);
        return;
    }

    auto start = std::chrono::steady_clock::now();

    std::string mesh_path = "";
    std::string coll_path = "";
    std::vector<std::string> textures_vector;
    std::vector<std::string> normals_vector;

    if (!GrEngine::Globals::readGMF(model_path, &mesh_path, &coll_path, &textures_vector, &normals_vector))
        return;

    GrEngine::Entity* target = GrEngine::Engine::GetContext()->GetRenderer()->GetSelectedEntity();
    GrEngine::Object* drawComponent = (GrEngine::Object*)target->GetPropertyValue(PropertyType::Drawable, (void*)nullptr);
    GrEngine::PhysicsObject* physComponent = (GrEngine::PhysicsObject*)target->GetPropertyValue(PropertyType::PhysComponent, (void*)nullptr);

    if (drawComponent != nullptr)
    {
        drawComponent->LoadModel(mesh_path.c_str(), textures_vector, normals_vector);
    }

    if (physComponent != nullptr)
    {
        physComponent->LoadCollisionMesh(coll_path.c_str());
    }

    auto end = std::chrono::steady_clock::now();
    auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    Logger::Out("Model %s loaded in %d ms", OutputType::Log, model_path, (int)time);

    std::vector<std::string> materials = GrEngine::Engine::GetContext()->GetMaterialNames(mesh_path.c_str());
    std::string out_materials;
    for (auto mat : materials)
    {
        out_materials += mat + "|";
    }

    SceneEditor::GetApplication()->GetBrowserContext()->UpdateMaterials(out_materials, 0);
}

void SceneEditor::LoadObject(const char* mesh_path, const char* textures_path)
{
    GrEngine::Engine::GetContext()->GetEventListener()->clearEventQueue();

    std::vector<std::string> tex_vector = GrEngine::Globals::SeparateString(textures_path, '|');
    //std::unordered_map<std::string, std::string> materials;
    std::string solution = GrEngine::Globals::getExecutablePath();

    if (std::string(mesh_path).substr(0, solution.size()) != solution)
    {
        Logger::Out("Resource outside the solution is being used! %s", OutputType::Error, mesh_path);
        return;
    }

    for (std::vector<std::string>::iterator itt = tex_vector.begin(); itt != tex_vector.end(); ++itt)
    {
        if ((*itt) != "" && (*itt).substr(0, solution.size()) != solution)
        {
            Logger::Out("Resource outside the solution is being used! %s",OutputType::Error, (*itt).c_str());
            return;
        }
    }

    GrEngine::Engine::GetContext()->LoadObject(GrEngine::Engine::GetContext()->GetSelectedEntityID(), mesh_path, tex_vector);
    //materials = static_cast<GrEngine::Object*>(GrEngine::Object::FindObject(GrEngine::Engine::GetContext()->GetRenderer()->GetSelectedEntity()))->GetMaterials();

    std::vector<std::string> materials = GrEngine::Engine::GetContext()->GetMaterialNames(mesh_path);
    std::string out_materials;
    std::string out_textures;
    for (auto mat : materials)
    {
        out_materials += mat + "|";
    }

    SceneEditor::GetApplication()->GetBrowserContext()->UpdateMaterials(out_materials, 1);
}

void SceneEditor::AssignTextures(const char* textures_path)
{
    std::vector<std::string> mat_vector = GrEngine::Globals::SeparateString(textures_path, '|');
    std::string solution = GrEngine::Globals::getExecutablePath();
    std::string mats = textures_path;

    for (std::string str : mat_vector)
    {
        if (str != "" && str.substr(0, solution.size()) != solution)
        {
            Logger::Out("Resource outside the solution is being used! %s", OutputType::Error, str.c_str());
            return;
        }
    }

    GrEngine::Object::FindObject(GrEngine::Engine::GetContext()->GetRenderer()->GetSelectedEntity())->AssignTextures(mat_vector);
}

void SceneEditor::CloseContext()
{
    GrEngine::Engine::GetContext()->Stop();
}

void SceneEditor::AddToTheScene(const char* model_path)
{
    std::string path = model_path;
    std::vector<std::any> para;
    for (char chr : path)
    {
        para.push_back(chr);
    }

    GrEngine::Engine::GetContext()->GetEventListener()->clearEventQueue();
    //GrEngine::Engine::GetContext()->Pause();
    //SceneEditor::GetApplication()->ModelBrowser_KeepResource();
    SceneEditor::GetApplication()->GetEventListener()->registerEvent("LoadModel", para);
}

void SceneEditor::CreateModelFile(const char* filename, const char* mesh_path, const char* collision_path, const char* textures)
{
    std::string temp_str = "";
    std::vector<std::string> mat_vector;

    if (textures)
    {
        std::string mats = textures;

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

	GrEngine::Engine::WriteGMF(filename, mesh_path, collision_path, mat_vector);
}

void  SceneEditor::SaveScreenshot(const char* filepath)
{
    GrEngine::Engine::GetContext()->GetRenderer()->SaveScreenshot(filepath);
    Logger::Out("Screenshot saved at %s", OutputType::Log, filepath);
}

void SceneEditor::TogglePhysics()
{
    static bool simation = false;
    simation = !simation;
    GrEngine::Engine::GetContext()->TogglePhysicsState(simation);
}

void SceneEditor::AddNewEntityProperty(int id, const char* property_name)
{
    SceneEditor::GetApplication()->addNewProperty(id, property_name);
}

void SceneEditor::SaveScene(const char* path)
{
    SceneEditor::GetApplication()->App_SaveScene(path);
}

void SceneEditor::LoadScene(const char* path)
{
    SceneEditor::GetApplication()->LoadScene(path);
}

void SceneEditor::GenerateTerrain(int resolution, int x, int y, int z, const char* height, const char* blend, const char* base, const char* red, const char* green, const char* blue,
    const char* base_nrm, const char* red_nrm, const char* green_nrm, const char* blue_nrm, const char* base_dis, const char* red_dis, const char* green_dis, const char* blue_dis)
{
    std::string solution = GrEngine::Globals::getExecutablePath();
    if ((std::string(height) != "" && std::string(height).substr(0, solution.size()) != solution) || (std::string(blend) != "" && std::string(blend).substr(0, solution.size()) != solution)
        || (std::string(base) != "" && std::string(base).substr(0, solution.size()) != solution)
        || (std::string(red) != "" && std::string(red).substr(0, solution.size()) != solution) || (std::string(green) != "" && std::string(green).substr(0, solution.size()) != solution)
        || (std::string(blue) != "" && std::string(blue).substr(0, solution.size()) != solution) || (std::string(base_nrm) != "" && std::string(base_nrm).substr(0, solution.size()) != solution)
        || (std::string(red_nrm) != "" && std::string(red_nrm).substr(0, solution.size()) != solution) || (std::string(green_nrm) != "" && std::string(green_nrm).substr(0, solution.size()) != solution)
        || (std::string(blue_nrm) != "" && std::string(blue_nrm).substr(0, solution.size()) != solution) || (std::string(base_dis) != "" && std::string(base_dis).substr(0, solution.size()) != solution)
        || (std::string(red_dis) != "" && std::string(red_dis).substr(0, solution.size()) != solution) || (std::string(green_dis) != "" && std::string(green_dis).substr(0, solution.size()) != solution)
        || (std::string(blue_dis) != "" && std::string(blue_dis).substr(0, solution.size()) != solution))
    {
        Logger::Out("Resource outside the solution is being used!", OutputType::Error);
        return;
    }

    SceneEditor::GetApplication()->App_GenerateTerrain(resolution, x, y, z, 
        { std::string(height).erase(0, solution.size()), std::string(blend).erase(0, solution.size()),
        std::string(base).erase(0, solution.size()), std::string(red).erase(0, solution.size()),
        std::string(green).erase(0, solution.size()), std::string(blue).erase(0, solution.size()) },
        { std::string(base_nrm).erase(0, solution.size()), std::string(red_nrm).erase(0, solution.size()),
        std::string(green_nrm).erase(0, solution.size()), std::string(blue_nrm).erase(0, solution.size())},
        { std::string(base_dis).erase(0, solution.size()), std::string(red_dis).erase(0, solution.size()),
        std::string(green_dis).erase(0, solution.size()), std::string(blue_dis).erase(0, solution.size()) }
    );
}

void SceneEditor::UpdateTerrain(const char* blend, const char* base, const char* red, const char* green, const char* blue,
    const char* base_nrm, const char* red_nrm, const char* green_nrm, const char* blue_nrm,
    const char* base_dis, const char* red_dis, const char* green_dis, const char* blue_dis)
{
    std::string solution = GrEngine::Globals::getExecutablePath();
    if ((std::string(blend) != "" && std::string(blend).substr(0, solution.size()) != solution)
        || (std::string(base) != "" && std::string(base).substr(0, solution.size()) != solution)
        || (std::string(red) != "" && std::string(red).substr(0, solution.size()) != solution) || (std::string(green) != "" && std::string(green).substr(0, solution.size()) != solution)
        || (std::string(blue) != "" && std::string(blue).substr(0, solution.size()) != solution) || (std::string(base_nrm) != "" && std::string(base_nrm).substr(0, solution.size()) != solution)
        || (std::string(red_nrm) != "" && std::string(red_nrm).substr(0, solution.size()) != solution) || (std::string(green_nrm) != "" && std::string(green_nrm).substr(0, solution.size()) != solution)
        || (std::string(blue_nrm) != "" && std::string(blue_nrm).substr(0, solution.size()) != solution) || (std::string(base_dis) != "" && std::string(base_dis).substr(0, solution.size()) != solution)
        || (std::string(red_dis) != "" && std::string(red_dis).substr(0, solution.size()) != solution) || (std::string(green_dis) != "" && std::string(green_dis).substr(0, solution.size()) != solution)
        || (std::string(blue_dis) != "" && std::string(blue_dis).substr(0, solution.size()) != solution))
    {
        Logger::Out("Resource outside the solution is being used!", OutputType::Error);
        return;
    }

    SceneEditor::GetApplication()->App_UpdateTerrain(
        { std::string(blend).erase(0, solution.size()),
        std::string(base).erase(0, solution.size()), std::string(red).erase(0, solution.size()),
        std::string(green).erase(0, solution.size()), std::string(blue).erase(0, solution.size()) },
        { std::string(base_nrm).erase(0, solution.size()), std::string(red_nrm).erase(0, solution.size()),
        std::string(green_nrm).erase(0, solution.size()), std::string(blue_nrm).erase(0, solution.size()) },
        { std::string(base_dis).erase(0, solution.size()), std::string(red_dis).erase(0, solution.size()),
        std::string(green_dis).erase(0, solution.size()), std::string(blue_dis).erase(0, solution.size()) });
}

void SceneEditor::ToggleBrush(int mode)
{
    SceneEditor::GetApplication()->App_ShowBrush(mode);
}

void SceneEditor::UpdateBrush(int mode, float opacity, float size, float falloff)
{
    SceneEditor::GetApplication()->App_UpdateBrush(mode, opacity, size, falloff);
}

void SceneEditor::SetActiveBrushChannels(bool red, bool green, bool blue)
{
    SceneEditor::GetApplication()->App_SetActiveChannels(red, green, blue);
}

void SceneEditor::ControlKey(bool state)
{
    //(double)glfwGetKeyScancode(GLFW_KEY_LEFT_CONTROL)

    std::vector<double> para = {
    (double)GLFW_KEY_LEFT_CONTROL, (double)0, (double)(state ? GLFW_PRESS : GLFW_RELEASE), (double)0
    };

    SceneEditor::GetApplication()->FocusWindow();
    SceneEditor::GetApplication()->GetEventListener()->registerEvent(EventType::KeyPress, para);
}

void SceneEditor::EscKey(bool state)
{
    //(double)glfwGetKeyScancode(GLFW_KEY_ESCAPE)

    std::vector<double> para = {
    (double)GLFW_KEY_ESCAPE, (double)0, (double)(state ? GLFW_PRESS : GLFW_RELEASE), (double)0
    };

    SceneEditor::GetApplication()->FocusWindow();
    SceneEditor::GetApplication()->GetEventListener()->registerEvent(EventType::KeyPress, para);
}

void SceneEditor::TabKey(bool state)
{
    //(double)glfwGetKeyScancode(GLFW_KEY_TAB)

    std::vector<double> para = {
    (double)GLFW_KEY_TAB, (double)0, (double)(state ? GLFW_PRESS : GLFW_RELEASE), (double)0
    };

    SceneEditor::GetApplication()->FocusWindow();
    SceneEditor::GetApplication()->GetEventListener()->registerEvent(EventType::KeyPress, para);
}

void SceneEditor::SKey(bool state)
{
    //(double)glfwGetKeyScancode(GLFW_KEY_S)

    std::vector<double> para = {
    (double)GLFW_KEY_S, (double)0, (double)(state ? GLFW_PRESS : GLFW_RELEASE), (double)0
    };

    SceneEditor::GetApplication()->FocusWindow();
    SceneEditor::GetApplication()->GetEventListener()->registerEvent(EventType::KeyPress, para);
}

void SceneEditor::ToggleLighting()
{
    SceneEditor::GetApplication()->GetRenderer()->SetUseDynamicLighting(!SceneEditor::GetApplication()->GetRenderer()->IsDynamicLightEnabled());
}

void SceneEditor::CopyEntity()
{
    SceneEditor::GetApplication()->App_UpdateCopyBuffer();
}

void SceneEditor::PasteEntity()
{
    SceneEditor::GetApplication()->App_CloneEntity();
}

void SceneEditor::DeleteEntity()
{
    SceneEditor::GetApplication()->App_RemoveEntity();
}

void SceneEditor::SnapEntity()
{
    SceneEditor::GetApplication()->App_SnapToGround();
}

void SceneEditor::RotateSun(float pitch, float yaw)
{
    SceneEditor::GetApplication()->App_RotateCascade(pitch, yaw);
}

void SceneEditor::SetSunColor(const char* color)
{
    SceneEditor::GetApplication()->App_ColorCascade(color);
}

void SceneEditor::SetAmbientModulator(float value)
{
    SceneEditor::GetApplication()->GetRenderer()->SetAmbientValue(value);
}

float SceneEditor::GetAmbientModulator()
{
    return SceneEditor::GetApplication()->GetRenderer()->GetAmbientValue();
}

void SceneEditor::SetSkyColor(float r, float g, float b)
{
    SceneEditor::GetApplication()->App_SetSkyColor(r, g, b);
}

bool SceneEditor::CheckCascade()
{
    return SceneEditor::GetApplication()->HasCascade();
}

void SceneEditor::AddCascade()
{
    SceneEditor::GetApplication()->App_AddCascadeEntity();
}

const char* SceneEditor::GetCascadeColor()
{
    return SceneEditor::GetApplication()->App_GetCascadeProperty(PropertyType::Color);
}

void SceneEditor::WriteImage(const char* filepath, int width, int height)
{
    SceneEditor::GetApplication()->App_CreateEmptyImage(filepath, width, height);
}

const char* SceneEditor::GetTerrainMask()
{
    return SceneEditor::GetApplication()->App_GetTerrainMask();
}

const char* SceneEditor::GetTerrainColor()
{
    return SceneEditor::GetApplication()->App_GetTerrainColor();
}

const char* SceneEditor::GetTerrainNormal()
{
    return SceneEditor::GetApplication()->App_GetTerrainNormal();
}

const char* SceneEditor::GetTerrainDisplacement()
{
    return SceneEditor::GetApplication()->App_GetTerrainDispacement();
}

const char* SceneEditor::GetSkyColor()
{
    return SceneEditor::GetApplication()->App_GetSkyColor();
}

void SceneEditor::ResetTools()
{
    SceneEditor::GetApplication()->App_ResetTools();
}

void SceneEditor::ClearScene()
{
    SceneEditor::GetApplication()->ClearScene();
}