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

void SceneEditor::MB_AddToTheScene(const char* model_path)
{
	GrEngine::ModelBrowser::sendToTheScene(model_path);
}

void SceneEditor::MB_Close()
{
	GrEngine::ModelBrowser::closeBrowser();
}