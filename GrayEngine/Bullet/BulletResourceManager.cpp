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

	Resource<CollisionMesh*>* BulletResourceManager::GetCollisionMeshResource(const char* name, CollisionType type)
	{
		std::string string_name = NormalizeName(name);
		std::string prefix = "";

		switch (type)
		{
		case ConvexHullMesh:
			prefix = "Hull_";
			break;
		case Mesh:
			prefix = "Mesh_";
			break;
		default:
			break;
		}

		for (std::vector<Resource<CollisionMesh*>*>::iterator itt = colResources.begin(); itt != colResources.end(); ++itt)
		{
			if ((*itt)->name == prefix + string_name)
			{
				return (*itt);
			}
		}

		return nullptr;
	}

	void BulletResourceManager::RemoveCollisionMeshResource(CollisionMesh* resource)
	{
		//std::string string_name = NormalizeName(name);
		if (resource == nullptr) return;

		for (std::vector<Resource<CollisionMesh*>*>::iterator itt = colResources.begin(); itt != colResources.end(); ++itt)
		{
			if ((*itt)->Compare(resource))
			{
				Resource<CollisionMesh*>* resource = (*itt);
				std::string string_name = resource->name;
				resource->RemoveLink();
				if (resource->getNumOfLinks() == 0)
				{
					Logger::Out("Collision resource %s was removed", OutputType::Log, string_name.c_str());
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
			if ((*itt)->getNumOfLinks() == 0)
			{
				delete (*itt);
				(*itt) = nullptr;
				colResources.erase(itt);
				break;
			}
		}

		colResources.clear();
	}
};
