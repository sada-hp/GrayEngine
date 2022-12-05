#pragma once
#include "Engine/Source/Headers/Core.h"

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
				file.close();
				file.open(filename, std::ios::ate | std::ios::binary);
			}

			std::size_t fileSize = (std::size_t)file.tellg();
			std::vector<char> buffer(fileSize);
			file.seekg(0);
			file.read(buffer.data(), fileSize);
			file.close();

			return buffer;
		}

		static bool readGMF(const std::string& filepath, std::string* mesh, std::vector<std::string>* textures)
		{
			auto buffer = readFile(filepath);

			if (buffer.size() == 0)
				return false;

			std::string temp_str = "";
			std::string distro = getExecutablePath();
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
							mesh->append(distro + temp_str);;
						}
						else
						{
							textures->push_back(distro + temp_str);
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

		static std::vector<std::string> SeparateString(std::string target, char separator)
		{
			std::vector<std::string> result;
			std::string temp = "";
			for (auto chr : target)
			{
				if (chr != separator)
				{
					temp += chr;
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

		static char* StringToCharArray(std::string source)
		{
			char* msg = new char[source.size() + 1];
			int i = 0;
			for (char letter : source)
			{
				msg[i++] = (int)letter;
			}
			msg[i] = '\0';

			return msg;
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