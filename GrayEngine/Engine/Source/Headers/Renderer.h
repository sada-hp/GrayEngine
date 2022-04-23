#pragma once
#include <pch.h>
#include <glfw/glfw3.h>
#include "Engine/Source/Headers/Core.h"

namespace GrEngine
{
	class Renderer
	{
	public:
		bool Initialized = false;

		Renderer() {};
		virtual ~Renderer() {};

		virtual bool init(GLFWwindow* window) = 0;
		virtual void destroy() = 0;
		virtual void drawFrame() = 0;
		virtual bool loadImage(const char* image_path, int material_index = 0) = 0;
		virtual bool loadModel(const char* mesh_path, std::vector<std::string> textures_vector, std::string* out_materials_names = nullptr) = 0;
		virtual void clearDrawables() = 0;
		virtual void Update() = 0;

		static std::string getExecutablePath()
		{
			char rawPathName[MAX_PATH];
			GetModuleFileNameA(NULL, rawPathName, MAX_PATH);

			std::string solution_path = rawPathName;
			for (int ind = solution_path.length(); ind > 0; ind--)
			{
				if (solution_path[ind] != '\\')
				{
					solution_path.erase(ind, ind + 1);
				}
				else
				{
					break;
				}
			}

			return solution_path;
		}

		static std::vector<char> readFile(const std::string& filename)
		{
			std::ifstream file(filename, std::ios::ate | std::ios::binary);

			if (!file.is_open())
				throw std::runtime_error("Failed to open file!");

			std::size_t fileSize = (std::size_t)file.tellg();
			std::vector<char> buffer(fileSize);
			file.seekg(0);
			file.read(buffer.data(), fileSize);
			file.close();

			return buffer;
		}
	};
}