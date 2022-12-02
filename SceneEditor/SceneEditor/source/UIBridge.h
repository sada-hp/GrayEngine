#pragma once
#include <SceneEditor.h>
#include <GrayEngine.h>
#include "EditorUI.h"
#include "ModelBrowser.h"
#include "Application.h"

namespace SceneEditor
{
	extern "C"
	{
		ExportDll void __stdcall AddEntity();
		ExportDll void __stdcall LogMessage(const char* msg);
		ExportDll void __stdcall InitModelBrowser();
		ExportDll void __stdcall GetEntityInfo(int ID);
		ExportDll void __stdcall UpdateEntityProperty(int ID, const char* selected_property, const char* value);
		ExportDll void __stdcall UpdateSkybox(const char* East, const char* West, const char* Top, const char* Bottom, const char* North, const char* South);
		ExportDll void __stdcall App_Close();

		ExportDll void __stdcall MB_LoadModelFile(const char* model_path);
		ExportDll void __stdcall MB_LoadObjectFile(const char* object_path);
		ExportDll void __stdcall MB_CreateModelFile(const char* mesh, const char* textures);
		ExportDll void __stdcall MB_LoadTexture(const char* image_path, int index);
		ExportDll void __stdcall MB_AddToTheScene(const char* model_path);
		ExportDll void __stdcall MB_Close();
	}
}