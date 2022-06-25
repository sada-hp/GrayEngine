#pragma once
#include "Camera.h"
#include "DrawableObject.h"

namespace GrEngine
{
	class DllExport Renderer
	{
	public:
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
		virtual DrawableObject* getDrawable() = 0;

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

		static bool readGMF(const std::string& filepath, std::string* mesh, std::vector<std::string>* textures)
		{
			auto buffer = Renderer::readFile(filepath);

			if (buffer.size() == 0)
				return false;

			std::string temp_str = "";
			bool is_mesh = false;

			if (buffer.size() == 0)
				return false;

			for (char chr : buffer)
			{
				if (chr == '<')
				{
					if (temp_str != "")
					{
						if (is_mesh)
						{
							mesh->append(temp_str);;
						}
						else
						{
							textures->push_back(temp_str);
						}

						temp_str = "";
					}

					temp_str += chr;
				}
				else if (chr == '>')
				{
					if (temp_str + chr == "<mesh>")
					{
						is_mesh = true;
					}
					else if (temp_str + chr == "<texture>")
					{
						is_mesh = false;
					}

					temp_str = "";
				}
				else
				{
					temp_str += chr;
				}
			}

			return true;
		}

		static bool writeGMF(const std::string& filepath, const std::string& mesh_path, const std::vector<std::string> textures_vector)
		{
			std::fstream new_file;
			new_file.open(filepath, std::fstream::out | std::ios::trunc);

			if (!new_file)
				return false;

			new_file << "<mesh>" << mesh_path << "<|mesh>";
			for (auto tex : textures_vector)
			{
				new_file << "<texture>" << tex << "<|texture>";
			}
			new_file.close();

			return true;
		}
	};
}