#pragma once
#include <pch.h>
#include <glfw/glfw3.h>
#include "Engine/Source/Headers/Core.h"

namespace GrEngine
{
	class Renderer
	{
	public:
		Renderer() {};
		virtual ~Renderer() {};

		virtual bool init(GLFWwindow* window) = 0;
		virtual void destroy() = 0;
		virtual void drawFrame() = 0;
		virtual bool loadModel(const char* mesh_path, std::string* out_materials = nullptr) = 0;
		virtual bool loadImage(const char* image_path, int material_index = 0) = 0;
		virtual void clearDrawables() = 0;
		virtual void Update() = 0;

		bool Initialized = false;
	};
}