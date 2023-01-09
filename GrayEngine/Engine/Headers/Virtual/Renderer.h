#pragma once
#include "Entities/Camera.h"
#include "Entities/DrawableObject.h"
#include "Entities/Entity.h"
#include "Entities/Skybox.h"
#include "Core/Logger.h"

namespace GrEngine
{
	class DllExport Renderer
	{
	public:
		double delta_time = 0;
		bool Initialized = false;
		Camera viewport_camera;

		Renderer() {};
		virtual ~Renderer() {};

		virtual bool init(void* window) = 0;
		virtual void destroy() = 0;
		virtual void RenderFrame() = 0;
		virtual bool assignTextures(std::vector<std::string> textures, Entity* target) = 0;
		virtual bool loadModel(UINT id, const char* mesh_path, std::vector<std::string> textures_vector) = 0;
		virtual bool loadModel(UINT id, const char* model_path) = 0;
		virtual void clearDrawables() = 0;
		virtual Entity* addEntity() = 0;
		std::map<UINT, Entity*>& GetEntitiesList()
		{
			return entities;
		}
		virtual void Update() = 0;
		virtual void createSkybox(const char* East, const char* West, const char* Top, const char* Bottom, const char* North, const char* South) = 0;
		virtual void DeleteEntity(UINT id) = 0;
		inline Camera* getActiveViewport() { return &viewport_camera; };
		virtual Entity* selectEntity(UINT32 ID)
		{ 
			if (auto search = entities.find(ID); search != entities.end())
			{
				selected_entity = ID;
				std::vector<std::any> para = { selected_entity };
				EventListener::registerEvent(EventType::SelectionChanged, para);
				return entities.at(ID);
			}
			return nullptr;
		};
		virtual Entity* GetSelectedEntity() { return entities[selected_entity]; };
		virtual void SaveScreenshot(const char* filepath) = 0;
		virtual void SelectEntityAtCursor() = 0;
		virtual void SetHighlightingMode(bool enabled) = 0;
		virtual void SaveScene(const char* path) = 0;
		virtual void LoadScene(const char* path) = 0;
	protected:
		UINT32 selected_entity = 0;
		std::map<UINT, Entity*> entities;
	};
}