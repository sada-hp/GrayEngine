#pragma once
#include <glfw/glfw3.h>

namespace GrEngine
{
	class Renderer
	{
	public:
		Renderer() {};
		virtual ~Renderer() {};

		virtual bool init(GLFWwindow* window, Renderer* apiInstance) = 0;
		virtual void destroy() = 0;
		virtual void drawFrame() = 0;
		virtual bool loadModel(const char* model_path) = 0;
		virtual bool loadImage(const char* model_path) = 0;
		virtual void clearDrawables() = 0;
		virtual void Update() = 0;

		bool Initialized = false;
	};
}