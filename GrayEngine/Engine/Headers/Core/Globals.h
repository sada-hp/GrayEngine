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

		static bool readGMF(const std::string& filepath, std::string* mesh, std::vector<std::string>* textures)
		{
			std::string stream = "";
			std::string distro = getExecutablePath();
			std::ifstream file(distro + filepath, std::ios::ate | std::ios::binary);

			if (!file.is_open())
			{
				file.close();
				file.open(filepath, std::ios::ate | std::ios::binary);
			}

			bool value = false;
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
					if (value)
					{
						mesh->append(stream);
					}
					else
					{
						textures->push_back(stream);
					}
				}
				else if (!block_open && stream == "mesh")
				{
					value = true;
				}
				else if (!block_open && stream == "textures")
				{
					value = false;
				}
			}

			return true;
		}

		static bool writeGMF(const char* filepath, const char* mesh_path, std::vector<std::string>& textures_vector)
		{
			std::fstream new_file;
			new_file.open(filepath, std::fstream::out | std::ios::trunc);

			if (!new_file)
				return false;

			new_file << "mesh\n{\n   " << mesh_path << "\n}\n";
			new_file << "textures\n{\n";
			for (std::vector<std::string>::iterator itt = textures_vector.begin(); itt != textures_vector.end(); ++itt)
			{
				new_file << "   " << (*itt) << '\n';
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