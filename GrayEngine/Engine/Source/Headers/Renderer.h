#pragma once
#include "Engine/Entities/Camera.h"
#include "Engine/Entities/DrawableObject.h"

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
		virtual void drawFrame() = 0;
		virtual bool loadImage(const char* image_path, int material_index = 0) = 0;
		virtual bool loadModel(const char* mesh_path, std::vector<std::string> textures_vector, std::unordered_map<std::string, std::string>* out_materials_names = nullptr) = 0;
		virtual void clearDrawables() = 0;
		virtual void Update() = 0;
		virtual void ShowGrid() = 0;
		virtual DrawableObject* getDrawable() = 0;
		inline Camera* getActiveViewport() { return &viewport_camera; };
	};
}