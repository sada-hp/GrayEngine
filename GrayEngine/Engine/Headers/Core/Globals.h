#pragma once
#include "Core/Core.h"

namespace GrEngine
{
	class DllExport Globals
	{
	public:
		static double delta_time;

		static std::string getExecutablePath()
		{
			char rawPathName[MAX_PATH];
			GetModuleFileNameA(NULL, rawPathName, MAX_PATH);

			std::string solution_path = rawPathName;
			for (size_t ind = solution_path.length(); ind > 0; ind--)
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
			{
				/*file.close();
				file.open(filename, std::ios::ate | std::ios::binary);*/
				return {};
			}

			std::size_t fileSize = (std::size_t)file.tellg();
			std::vector<char> buffer(fileSize);
			file.seekg(0);
			file.read(buffer.data(), fileSize);
			file.close();

			return buffer;
		}

		template<typename T>
		static bool VectorContains(std::vector<T>& vec, T value)
		{
			for (int i = 0; i < vec.size(); i++)
			{
				if (vec[i] == value)
				{
					return true;
				}
			}

			return false;
		}

		static bool readGMF(const std::string& filepath, std::string* mesh, std::string* collisions, std::vector<std::string>* textures, std::vector<std::string>* normals)
		{
			if (filepath == "")
				return false;

			std::string stream = "";
			std::string distro = getExecutablePath();
			std::ifstream file;
			if (!filepath.starts_with(distro))
			{
				file.open(distro + filepath, std::ios::ate | std::ios::binary);
			}
			else
			{
				file.open(filepath, std::ios::ate | std::ios::binary);
			}

			if (!file.is_open())
			{
				//file.close();
				//file.open(filepath, std::ios::ate | std::ios::binary);
				return false;
			}

			int value = 0;
			bool block_open = false;
			file.seekg(0);

			while (file >> stream)
			{
				if (stream == "{")
				{
					block_open = true;
				}
				else if (stream == "}")
				{
					block_open = false;
				}
				else if (block_open)
				{
					if (value == 0)
					{
						if (textures != nullptr)
						{
							textures->push_back(stream);
						}
					}
					else if (value == 1)
					{
						if (mesh != nullptr)
						{
							mesh->append(stream);
						}
					}
					else if (value == 2)
					{
						if (collisions != nullptr)
						{
							collisions->append(stream);
						}
					}
					else if (value == 3)
					{
						if (normals != nullptr)
						{
							normals->push_back(stream);
						}
					}
				}
				else if (!block_open && stream == "mesh")
				{
					value = 1;
				}
				else if (!block_open && stream == "collision")
				{
					value = 2;
				}
				else if (!block_open && stream == "normals")
				{
					value = 3;
				}
				else if (!block_open && stream == "textures")
				{
					value = 0;
				}
			}

			return true;
		}

		static bool writeGMF(const char* filepath, const char* mesh_path, const char* colliision_path, std::vector<std::string>& textures_vector)
		{
			std::fstream new_file;
			std::string distro = getExecutablePath();
			if (!std::string(filepath).starts_with(distro))
			{
				new_file.open(distro + filepath, std::ios::out | std::ios::trunc);
			}
			else
			{
				new_file.open(filepath, std::ios::out | std::ios::trunc);
			}

			if (!new_file)
				return false;

			new_file << "mesh\n{\n   " << mesh_path << "\n}\n";
			new_file << "collision\n{\n   " << colliision_path << "\n}\n";
			new_file << "textures\n{\n";
			for (std::vector<std::string>::iterator itt = textures_vector.begin(); itt != textures_vector.end(); ++itt)
			{
				if ((*itt).starts_with(":Color:"))
				{
					std::string name = (*itt).substr(7, (*itt).size() - 7);
					name = name == "" ? "empty_texture" : name;
					new_file << "   " << name << '\n';
				}
				else if (!(*itt).starts_with(":Normal:"))
				{
					std::string name = (*itt);
					name = name == "" ? "empty_texture" : name;
					new_file << "   " << name << '\n';
				}
			}
			new_file << "}\n\0";
			new_file << "normals\n{\n";
			for (std::vector<std::string>::iterator itt = textures_vector.begin(); itt != textures_vector.end(); ++itt)
			{
				if ((*itt).starts_with(":Normal:"))
				{
					std::string name = (*itt).substr(8, (*itt).size() - 8);
					name = name == "" ? "empty_texture" : name;
					new_file << "   " << name << '\n';
				}
			}
			new_file << "}\n\0";
			new_file.close();

			return true;
		}

		static std::vector<std::string> SeparateString(std::string target, char separator)
		{
			std::vector<std::string> result;
			std::string temp = "";
			for (std::string::iterator itt = target.begin(); itt != target.end(); ++itt)
			{
				if ((*itt) != separator)
				{
					temp += (*itt);
				}
				else
				{
					result.push_back(temp);
					temp = "";
				}
			}
			if (temp != "") result.push_back(temp);

			return result;
		}

		static std::string FloatToString(float value, size_t precision)
		{
			std::stringstream stream;
			stream.precision(precision);
			stream << std::fixed;
			stream << value;

			return stream.str();
		}
	};
}