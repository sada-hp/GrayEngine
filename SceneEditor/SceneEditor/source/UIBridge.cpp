#include "UIBridge.h"

void SceneEditor::LogMessage(const char* msg)
{
	Logger::Out(msg, OutputColor::Blue, OutputType::Log);
}

void SceneEditor::InitModelBrowser()
{
	SceneEditor::GetApplication()->initModelBrowser();
}

void SceneEditor::AddEntity()
{
	SceneEditor::GetApplication()->App_UpdateEntity(SceneEditor::GetApplication()->GetContext()->AddEntity());
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
        Logger::Out("Resource outside the solution is being used!", OutputColor::Red, OutputType::Error);
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
    std::unordered_map<std::string, std::string> materials;
    std::string solution = GrEngine::Globals::getExecutablePath();
    if (std::string(model_path).substr(0, solution.size()) != solution)
    {
        Logger::Out("Resource outside the solution is being used! %s", OutputColor::Red, OutputType::Error, model_path);
        return;
    }

    GrEngine::Engine::GetContext()->LoadFromGMF(GrEngine::Engine::GetContext()->GetSelectedEntityID(), std::string(model_path).erase(0, solution.size()).c_str());
    materials = GGetMesh(GrEngine::Engine::GetContext()->GetRenderer()->GetSelectedEntity())->GetMaterials();

    std::string out_materials;
    std::string out_textures;
    for (auto pair : materials)
    {
        out_materials += pair.first + "|";
        out_textures += pair.second + "|";
    }

    GrEngine::Engine::GetContext()->GetEventListener()->registerEvent("RequireMaterialsUpdate", { out_materials, out_textures, 0 });
}

void SceneEditor::LoadObject(const char* mesh_path, const char* textures_path)
{
    GrEngine::Engine::GetContext()->GetEventListener()->clearEventQueue();

    std::vector<std::string> tex_vector = GrEngine::Globals::SeparateString(textures_path, '|');
    std::unordered_map<std::string, std::string> materials;
    std::string solution = GrEngine::Globals::getExecutablePath();

    if (std::string(mesh_path).substr(0, solution.size()) != solution)
    {
        Logger::Out("Resource outside the solution is being used! %s", OutputColor::Red, OutputType::Error, mesh_path);
        return;
    }

    for (std::vector<std::string>::iterator itt = tex_vector.begin(); itt != tex_vector.end(); ++itt)
    {
        if ((*itt) != "" && (*itt).substr(0, solution.size()) != solution)
        {
            Logger::Out("Resource outside the solution is being used! %s", OutputColor::Red, OutputType::Error, (*itt).c_str());
            return;
        }
    }

    GrEngine::Engine::GetContext()->LoadObject(GrEngine::Engine::GetContext()->GetSelectedEntityID(), mesh_path, tex_vector);
    materials = static_cast<GrEngine::DrawableObject*>(GrEngine::Engine::GetContext()->GetRenderer()->GetSelectedEntity())->GetMaterials();

    std::string out_materials;
    std::string out_textures;
    for (auto pair : materials)
    {
        out_materials += pair.first + "|";
        out_textures += pair.second + "|";
    }

    GrEngine::Engine::GetContext()->GetEventListener()->registerEvent("RequireMaterialsUpdate", { out_materials, out_textures, 1});
}

void SceneEditor::AssignTextures(const char* textures_path)
{
    std::string temp_str = "";
    std::vector<std::string> mat_vector;
    std::string solution = GrEngine::Globals::getExecutablePath();
    std::string mats = textures_path;

    for (char chr : mats)
    {
        if (chr != '|')
        {
            temp_str += chr;
        }
        else
        {
            if (temp_str != "" && temp_str.substr(0, solution.size()) != solution)
            {
                Logger::Out("Resource outside the solution is being used! %s", OutputColor::Red, OutputType::Error, temp_str.c_str());
                return;
            }

            mat_vector.push_back(temp_str);
            temp_str = "";
            continue;
        }
    }
    //GrEngine::Engine::GetContext()->AssignTextures(mat_vector, GrEngine::Engine::GetContext()->GetRenderer()->GetSelectedEntity());
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
    GrEngine::Engine::GetContext()->Pause();
    SceneEditor::GetApplication()->GetEventListener()->registerEvent("LoadModel", para);
}

void SceneEditor::CreateModelFile(const char* mesh_path, const char* textures)
{
    std::string temp_str = "";
    std::vector<std::string> mat_vector;
    std::string mesh = "";
    std::string file = "";

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

	GrEngine::Engine::WriteGMF(file.c_str(), mesh.c_str(), mat_vector);
}

void  SceneEditor::SaveScreenshot(const char* filepath)
{
    GrEngine::Engine::GetContext()->GetRenderer()->SaveScreenshot(filepath);
    Logger::Out("Screenshot saved at %s", OutputColor::Green, OutputType::Log, filepath);
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

void SceneEditor::GenerateTerrain(int resolution, int x, int y, int z, const char* height, const char* blend, const char* base, const char* red, const char* green, const char* blue)
{
    std::string solution = GrEngine::Globals::getExecutablePath();
    if ((std::string(height) != "" && std::string(height).substr(0, solution.size()) != solution) || (std::string(blend) != "" && std::string(blend).substr(0, solution.size()) != solution)
        || (std::string(base) != "" && std::string(base).substr(0, solution.size()) != solution)
        || (std::string(red) != "" && std::string(red).substr(0, solution.size()) != solution) || (std::string(green) != "" && std::string(green).substr(0, solution.size()) != solution)
        || (std::string(blue) != "" && std::string(blue).substr(0, solution.size()) != solution))
    {
        Logger::Out("Resource outside the solution is being used!", OutputColor::Red, OutputType::Error);
        return;
    }

    SceneEditor::GetApplication()->App_GenerateTerrain(resolution, x, y, z, 
        { std::string(height).erase(0, solution.size()), std::string(blend).erase(0, solution.size()),
        std::string(base).erase(0, solution.size()), std::string(red).erase(0, solution.size()),
        std::string(green).erase(0, solution.size()), std::string(blue).erase(0, solution.size()) });
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
    SceneEditor::GetApplication()->ctr_down = state;
    SceneEditor::GetApplication()->FocusViewport();
}