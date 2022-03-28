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
		virtual void clearDrawables() = 0;
		virtual void Update() = 0;

		static std::vector<char> readFile(const std::string& filename)
		{
			std::ifstream file(filename, std::ios::ate | std::ios::binary);

			if (!file.is_open())
				throw std::runtime_error("Failed to open file!");

			size_t fileSize = (size_t)file.tellg();
			std::vector<char> buffer(fileSize);
			file.seekg(0);
			file.read(buffer.data(), fileSize);
			file.close();

			return buffer;
		}

		bool Initialized = false;
	};
}