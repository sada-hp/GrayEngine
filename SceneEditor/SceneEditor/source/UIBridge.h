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
		ExportDll void __stdcall CloseModelBrowser();
		ExportDll void __stdcall GetEntityInfo(int ID);
		ExportDll void __stdcall GetEntitiesList();
		ExportDll void __stdcall UpdateEntityProperty(int ID, const char* selected_property, const char* value);
		ExportDll void __stdcall UpdateSkybox(const char* East, const char* West, const char* Top, const char* Bottom, const char* North, const char* South);
		ExportDll void __stdcall App_Close();
		ExportDll void __stdcall LoadModelFile(const char* model_path);
		ExportDll void __stdcall LoadObject(const char* mesh_path, const char* textures_path);
		ExportDll void __stdcall AssignTextures(const char* textures_path);
		ExportDll void __stdcall AddToTheScene(const char* model_path);
		ExportDll void __stdcall CloseContext();
		ExportDll void __stdcall CreateModelFile(const char* filename, const char* mesh_path, const char* collision_path, const char* textures);
		ExportDll void __stdcall SaveScreenshot(const char* filepath);
		ExportDll void __stdcall TogglePhysics();
		ExportDll void __stdcall AddNewEntityProperty(int id, const char* property);
		ExportDll void __stdcall SaveScene(const char* path);
		ExportDll void __stdcall LoadScene(const char* path);
		ExportDll void __stdcall GenerateTerrain(int resolution, int x, int y, int z, const char* height, const char* blend, const char* base, const char* red, const char* green, const char* blue,
			const char* base_nrm, const char* red_nrm, const char* green_nrm, const char* blue_nrm,
			const char* base_dis, const char* red_dis, const char* green_dis, const char* blue_dis);
		ExportDll void __stdcall UpdateTerrain(const char* blend, const char* base, const char* red, const char* green, const char* blue, 
			const char* base_nrm, const char* red_nrm, const char* green_nrm, const char* blue_nrm,
			const char* base_dis, const char* red_dis, const char* green_dis, const char* blue_dis);
		ExportDll void __stdcall ToggleBrush(int mode);
		ExportDll void __stdcall ToggleSculpt(int mode);
		ExportDll void __stdcall UpdateBrush(int mode, float opacity, float size, float falloff);
		ExportDll void __stdcall SetActiveBrushChannels(bool red, bool green, bool blue);
		ExportDll void __stdcall ControlKey(bool state);
		ExportDll void __stdcall EscKey(bool state);
		ExportDll void __stdcall TabKey(bool state);
		ExportDll void __stdcall SKey(bool state);
		ExportDll void __stdcall ToggleLighting();
		ExportDll void __stdcall CopyEntity();
		ExportDll void __stdcall PasteEntity();
		ExportDll void __stdcall DeleteEntity();
		ExportDll void __stdcall SnapEntity();
		ExportDll void __stdcall RotateSun(float pitch, float yaw);
		ExportDll void __stdcall SetSunColor(const char* color);
		ExportDll void __stdcall SetAmbientModulator(float value);
		ExportDll float __stdcall GetAmbientModulator();
		ExportDll void __stdcall SetSkyColor(float r, float g, float b);
		ExportDll bool __stdcall CheckCascade();
		ExportDll void __stdcall AddCascade();
		ExportDll const char* __stdcall GetCascadeColor();
		ExportDll void __stdcall WriteImage(const char* filepath, int width, int height);
		ExportDll const char* __stdcall GetTerrainMask();
		ExportDll const char* __stdcall GetTerrainColor();
		ExportDll const char* __stdcall GetTerrainNormal();
		ExportDll const char* __stdcall GetTerrainDisplacement();
		ExportDll const char* __stdcall GetSkyColor();
		ExportDll void __stdcall ResetTools();
		ExportDll void __stdcall ClearScene();
	}
}