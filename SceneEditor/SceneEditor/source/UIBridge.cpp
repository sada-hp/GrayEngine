#include "UIBridge.h"

void SceneEditor::AddEntity()
{
	GrEngine::Application::addEntity();
}

void SceneEditor::LogMessage(const char* msg)
{
	Logger::Out(msg, OutputColor::Blue, OutputType::Log);
}

void SceneEditor::InitModelBrowser()
{
	GrEngine::Application::initModelBrowser();
}

void SceneEditor::UpdateEntityProperty(int ID, const char* selected_property, const char* value)
{
	GrEngine::Application::updateEntity(ID, selected_property, value);
}

void SceneEditor::MB_LoadModelFile(const char* model_path)
{
	GrEngine::ModelBrowser::loadModelFromFile(model_path);
}

void SceneEditor::MB_LoadObjectFile(const char* object_path)
{
	GrEngine::ModelBrowser::loadRawModel(object_path);
}

void SceneEditor::MB_CreateModelFile(const char* mesh, const char* textures)
{
	GrEngine::ModelBrowser::createModelFile(mesh, textures);
}

void SceneEditor::MB_LoadTexture(const char* image_path, int index)
{
	GrEngine::ModelBrowser::uploadTexture(image_path, index);
}

void SceneEditor::MB_AddToTheScene(const char* model_path)
{
	GrEngine::ModelBrowser::sendToTheScene(model_path);
}

void SceneEditor::MB_Close()
{
	GrEngine::ModelBrowser::closeBrowser();
}

void SceneEditor::UpdateSkybox(const char* East, const char* West, const char* Top, const char* Bottom, const char* North, const char* South)
{
	GrEngine::Application::UpdateSkybox(East, West, Top, Bottom, North, South);
}

void SceneEditor::GetEntityInfo(int ID)
{
	GrEngine::Application::getEntityInfo(ID);
}

void SceneEditor::App_Close()
{
	GrEngine::Application::KillEngine();
	GrEngine::Application::getEditorUI()->destroyUI(VIEWPORT_EDITOR);
}