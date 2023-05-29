#pragma once
#include "Engine/Headers/Core/Globals.h"

template<typename T>
struct Resource
{
	Resource(const char* new_name, T new_pointer)
	{
		name = new_name;
		pointer = new_pointer;
	}

	~Resource()
	{
		delete pointer;
		pointer = nullptr;
	}

	T AddLink()
	{
		if (links + 1 != 0)
		{
			links++;
		}

		return pointer;
	}

	void RemoveLink()
	{
		if (links - 1 != UINT64_MAX)
		{
			links--;
		}
		else
		{
			links = 0;
		}
	}

	T PopResource()
	{
		RemoveLink();
		return pointer;
	}

	void Update(T other_pointer)
	{
		pointer = other_pointer;
	}

	bool Compare(T other_pointer)
	{
		return pointer == other_pointer;
	}

	uint64_t getNumOfLinks()
	{
		return links;
	}

	std::string name;
private:
	uint64_t links = 0;
	T pointer;
};

namespace GrEngine
{
	class ResourceManager
	{
	public:
		ResourceManager()
		{

		}

		virtual ~ResourceManager()
		{

		}
	protected:
		std::string NormalizeName(std::string name)
		{
			std::string path = GrEngine::Globals::getExecutablePath();

			if (name.starts_with(path))
			{
				return name.substr(path.size(), name.size() - path.size());
			}
			else
			{
				return name;
			}
		}
	};
};
