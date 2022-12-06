#pragma once
#include "Engine/Entities/Camera.h"
#include "Engine/Entities/DrawableObject.h"
#include "Engine/Source/Headers/Entity.h"

namespace GrEngine
{
	class DllExport Renderer
	{
	public:
		double delta_time = 0;
		bool Initialized = false;
		Camera viewport_camera;
		std::map<int, Entity*> entities;

		Renderer() {};
		virtual ~Renderer() {};

		virtual bool init(void* window) = 0;
		virtual void destroy() = 0;
		virtual void drawFrame() = 0;
		virtual bool assignTextures(std::vector<std::string> textures, Entity* target) = 0;
		virtual bool loadModel(const char* mesh_path, std::vector<std::string> textures_vector, std::unordered_map<std::string, std::string>* out_materials_names = nullptr) = 0;
		virtual void clearDrawables() = 0;
		virtual EntityInfo addEntity() = 0;
		virtual void Update() = 0;
		virtual void createSkybox(const char* East, const char* West, const char* Top, const char* Bottom, const char* North, const char* South) = 0;
		inline Camera* getActiveViewport() { return &viewport_camera; };
		inline EntityInfo getEntityInfo(int ID) { return entities[ID]->GetEntityInfo(); };
		virtual Entity* selectEntity(int ID) { selected_entity = ID; return entities[ID]; };
		virtual Entity* GetSelectedEntity() { return entities[selected_entity]; };
		virtual void SaveScreenshot(const char* filepath) = 0;
	protected:
		int selected_entity = 0;
	};
}