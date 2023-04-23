#include "pch.h"
#include "BulletAPI.h"
#include "BulletResourceManager.h"

namespace GrEngineBullet
{
	Resource<CollisionMesh*>* BulletResourceManager::AddCollisionMeshResource(const char* name, CollisionMesh* pointer)
	{
		std::string string_name = NormalizeName(name);
		colResources.push_back(new Resource<CollisionMesh*>(string_name.c_str(), pointer));
		return colResources.back();
	}

	Resource<CollisionMesh*>* BulletResourceManager::GetCollisionMeshResource(const char* name)
	{
		std::string string_name = NormalizeName(name);

		for (std::vector<Resource<CollisionMesh*>*>::iterator itt = colResources.begin(); itt != colResources.end(); ++itt)
		{
			if ((*itt)->name == string_name)
			{
				return (*itt);
			}
		}

		return nullptr;
	}

	void BulletResourceManager::RemoveCollisionMeshResource(const char* name)
	{
		std::string string_name = NormalizeName(name);

		for (std::vector<Resource<CollisionMesh*>*>::iterator itt = colResources.begin(); itt != colResources.end(); ++itt)
		{
			if ((*itt)->name == string_name)
			{
				Resource<CollisionMesh*>* resource = (*itt);
				resource->RemoveLink();
				if (resource->getNumOfLinks() == 0)
				{
					Logger::Out("Collision resource %s was removed", OutputColor::Blue, OutputType::Log, string_name.c_str());
					delete resource;
					resource = nullptr;
					colResources.erase(itt);
					break;
				}
			}
		}
	}

	void BulletResourceManager::Clean()
	{
		for (std::vector<Resource<CollisionMesh*>*>::iterator itt = colResources.begin(); itt != colResources.end(); ++itt)
		{
			(*itt)->RemoveLink();
			delete (*itt);
		}

		colResources.clear();
	}
};
