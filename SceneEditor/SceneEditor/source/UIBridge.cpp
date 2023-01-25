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
    GrEngine::Engine::GetContext()->LoadSkybox(East, West, Top, Bottom, North, South);
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
    GrEngine::Engine::GetContext()->LoadFromGMF(GrEngine::Engine::GetContext()->GetSelectedEntityID(), model_path);
    materials = static_cast<GrEngine::DrawableObject*>(GrEngine::Engine::GetContext()->GetRenderer()->GetSelectedEntity())->GetMaterials();

    std::string out_materials;
    std::string out_textures;
    for (auto pair : materials)
    {
        out_materials += pair.first + "|";
        out_textures += pair.second + "|";
    }

    EventListener::registerEvent("RequireMaterialsUpdate", { out_materials, out_textures, 0 });
}

void SceneEditor::LoadObject(const char* mesh_path, const char* textures_path)
{
    EventListener::clearEventQueue();

    std::string temp_str = "";
    std::vector<std::string> tex_vector = GrEngine::Globals::SeparateString(textures_path, '|');
    std::unordered_map<std::string, std::string> materials;

    GrEngine::Engine::GetContext()->LoadObject(GrEngine::Engine::GetContext()->GetSelectedEntityID(), mesh_path, tex_vector);
    materials = static_cast<GrEngine::DrawableObject*>(GrEngine::Engine::GetContext()->GetRenderer()->GetSelectedEntity())->GetMaterials();

    std::string out_materials;
    std::string out_textures;
    for (auto pair : materials)
    {
        out_materials += pair.first + "|";
        out_textures += pair.second + "|";
    }

    EventListener::registerEvent("RequireMaterialsUpdate", { out_materials, out_textures, 1});
}

void SceneEditor::AssignTextures(const char* textures_path)
{
    std::string temp_str = "";
    std::vector<std::string> mat_vector;

    std::string mats = textures_path;

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
    GrEngine::Engine::GetContext()->AssignTextures(mat_vector, GrEngine::Engine::GetContext()->GetRenderer()->GetSelectedEntity());
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

    EventListener::clearEventQueue();
    GrEngine::Engine::GetContext()->Pause();
    EventListener::registerEvent("LoadModel", para);
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