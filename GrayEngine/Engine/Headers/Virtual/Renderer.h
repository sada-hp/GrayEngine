#pragma once
#include "Entities/Camera.h"
#include "Entities/Entity.h"
#include "Entities/Skybox.h"
#include "Entities/Properties/Drawable.h"
#include "Entities/Properties/Light.h"
#include "Core/Logger.h"

namespace GrEngine
{
	class DllExport Renderer
	{
	public:
		struct FogSettings {
			glm::vec3 color;
			float density;
			float height;
		};

		double delta_time = 0;
		bool Initialized = false;
		Camera* viewport_camera;

		Renderer() {};
		virtual ~Renderer() {};

		virtual bool init(void* window) = 0;
		virtual void destroy() = 0;
		virtual void RenderFrame() = 0;
		virtual bool updateTexture(Entity* target, int textureIndex) = 0;
		virtual void clearDrawables() = 0;
		virtual void waitForRenderer() = 0;
		virtual Entity* addEntity() = 0;
		virtual Entity* addEntity(UINT ID) = 0;
		virtual void addEntity(Entity* entity) = 0;
		virtual Object* InitDrawableObject(Entity* ownerEntity) = 0;
		virtual LightObject* InitSpotlightObject(Entity* ownerEntity) = 0;
		virtual LightObject* InitCascadeLightObject(Entity* ownerEntity) = 0;
		virtual LightObject* InitPointLightObject(Entity* ownerEntity) = 0;
		virtual LightObject* InitOmniLightObject(Entity* ownerEntity) = 0;
		virtual Entity* CloneEntity(UINT id) = 0;
		virtual void VSyncState(bool state) = 0;
		virtual void SetUseDynamicLighting(bool state) = 0;
		bool IsDynamicLightEnabled() { return use_dynamic_lighting; }
		std::map<UINT, Entity*>& GetEntitiesList()
		{
			return entities;
		}
		virtual void UpdateFogParameters(FogSettings para) = 0;
		virtual void Update() = 0;
		virtual void LoadTerrain(int resolution, int width, int height, int depth, std::array<std::string, 6> maps, std::array<std::string, 4> normals, std::array<std::string, 4> displacement) = 0;
		virtual void LoadTerrain(const char* filepath) = 0;
		virtual void createSkybox(const char* East, const char* West, const char* Top, const char* Bottom, const char* North, const char* South) = 0;
		virtual void DeleteEntity(UINT id) = 0;
		virtual void ParseCVar(const char* cvar, const char* val) = 0;
		virtual void SetAmbientValue(float new_value)
		{
			ambient = new_value;
		}
		virtual float GetAmbientValue()
		{
			return ambient;
		}
		inline Camera* getActiveViewport() { return viewport_camera; };
		inline void SetActiveViewport(Camera* viewport) { viewport_camera = viewport; };
		virtual Entity* selectEntity(UINT32 ID)
		{ 
			if (auto search = entities.find(ID); search != entities.end())
			{
				selected_entity = ID;
				std::vector<double> para = { static_cast<double>(selected_entity) };
				listener->registerEvent(EventType::SelectionChanged, para);
				return entities.at(ID);
			}
			return nullptr;
		};
		virtual Entity* GetSelectedEntity() 
		{ 
			return selected_entity == 0 ? nullptr : entities[selected_entity];
		};
		virtual std::vector<Entity*> GetEntitiesOfType(EntityType type)
		{
			std::vector<Entity*> res;
			for (std::map<UINT, GrEngine::Entity*>::iterator itt = entities.begin(); itt != entities.end(); ++itt)
			{
				if (((*itt).second->GetEntityType() & type) != 0)
				{
					res.push_back((*itt).second);
				}
			}

			return res;
		};
		UINT32& GetSelectionID() { return selected_entity; };
		virtual void SaveScreenshot(const char* filepath) = 0;
		virtual void SelectEntityAtCursor() = 0;
		virtual std::array<byte, 3> GetPixelColorAtCursor() = 0;
		virtual void SetHighlightingMode(bool enabled) = 0;
		virtual void SaveScene(const char* path) = 0;
		virtual void LoadScene(const char* path) = 0;
		virtual std::vector<std::string> GetMaterialNames(const char* mesh_path) = 0;
		EventListener* listener;

		static float NearPlane;
		static float FarPlane;
	protected:
		UINT32 selected_entity = 0;
		std::map<UINT, Entity*> entities;
		std::map<UINT, Object*> drawables;
		std::map<UINT, LightObject*> lights;
		bool use_dynamic_lighting = false;
		float ambient = 0.15f;
	};
}