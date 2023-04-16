#include "pch.h"
#include "VulkanResourceManager.h"
#include "VulkanAPI.h"

namespace GrEngine_Vulkan
{
	void VulkanResourceManager::RemoveMesh(const char* name, VkDevice device, VmaAllocator allocator)
	{
		std::string string_name = NormalizeName(name);

		for (std::vector<Resource<Mesh*>*>::iterator itt = meshResources.begin(); itt != meshResources.end(); ++itt)
		{
			if ((*itt)->name == string_name)
			{
				Resource<Mesh*>* resource = (*itt);
				resource->RemoveLink();
				if (resource->getNumOfLinks() == 0)
				{
					Logger::Out("Resource %s was removed", OutputColor::Blue, OutputType::Log, string_name.c_str());
					VulkanAPI::m_destroyShaderBuffer(device, allocator, &resource->PopResource()->indexBuffer);
					VulkanAPI::m_destroyShaderBuffer(device, allocator, &resource->PopResource()->vertexBuffer);
					delete resource;
					meshResources.erase(itt);
					break;
				}
			}
		}
	}

	void VulkanResourceManager::RemoveTexture(const char* name, VkDevice device, VmaAllocator allocator)
	{
		std::string string_name = NormalizeName(name);

		for (std::vector<Resource<Texture*>*>::iterator itt = texResources.begin(); itt != texResources.end(); ++itt)
		{
			if ((*itt)->name == string_name)
			{
				Resource<Texture*>* resource = (*itt);
				resource->RemoveLink();
				if (resource->getNumOfLinks() == 0)
				{
					Logger::Out("Resource %s was removed", OutputColor::Blue, OutputType::Log, string_name.c_str());
					VulkanAPI::m_destroyTexture(device, allocator, resource->PopResource());
					delete resource;
					texResources.erase(itt);
					break;
				}
			}
		}
	}

	void VulkanResourceManager::UpdateTexture(std::vector<std::string> names, VkDevice device, VmaAllocator allocator, Texture* newValue)
	{
		std::string string_name = "";
		for (std::vector<std::string>::iterator itt = names.begin(); itt != names.end(); ++itt)
		{
			string_name += NormalizeName((*itt));
		}

		for (std::vector<Resource<Texture*>*>::iterator itt = texResources.begin(); itt != texResources.end(); ++itt)
		{
			if ((*itt)->name == string_name)
			{
				VulkanAPI::m_destroyTexture(device, allocator, (*itt)->PopResource());
				(*itt)->Update(newValue);
				break;
			}
		}
	}

	void VulkanResourceManager::RemoveTexture(std::vector<std::string> names, VkDevice device, VmaAllocator allocator)
	{
		std::string string_name = "";
		for (std::vector<std::string>::iterator itt = names.begin(); itt != names.end(); ++itt)
		{
			string_name += NormalizeName((*itt));
		}

		for (std::vector<Resource<Texture*>*>::iterator itt = texResources.begin(); itt != texResources.end(); ++itt)
		{
			if ((*itt)->name == string_name)
			{
				Resource<Texture*>* resource = (*itt);
				resource->RemoveLink();
				if (resource->getNumOfLinks() == 0)
				{
					Logger::Out("Resource %s was removed", OutputColor::Blue, OutputType::Log, string_name.c_str());
					VulkanAPI::m_destroyTexture(device, allocator, resource->PopResource());
					delete resource;
					texResources.erase(itt);
				}
				break;
			}
		}
	}

	void VulkanResourceManager::Clean(VkDevice device, VmaAllocator allocator)
	{
		for (std::vector<Resource<Mesh*>*>::iterator itt = meshResources.begin(); itt != meshResources.end(); ++itt)
		{
			(*itt)->RemoveLink();
			VulkanAPI::m_destroyShaderBuffer(device, allocator, &(*itt)->PopResource()->indexBuffer);
			VulkanAPI::m_destroyShaderBuffer(device, allocator, &(*itt)->PopResource()->vertexBuffer);
			delete (*itt);
		}

		for (std::vector<Resource<Texture*>*>::iterator itt = texResources.begin(); itt != texResources.end(); ++itt)
		{
			(*itt)->RemoveLink();
			VulkanAPI::m_destroyTexture(device, allocator, (*itt)->PopResource());
			delete (*itt);
		}

		meshResources.clear();
		texResources.clear();
	}

	Resource<Mesh*>* VulkanResourceManager::AddMeshResource(const char* name, Mesh* pointer)
	{
		std::string string_name = NormalizeName(name);
		meshResources.push_back(new Resource<Mesh*>(string_name.c_str(), pointer));
		return meshResources.back();
	}

	Resource<Texture*>* VulkanResourceManager::AddTextureResource(const char* name, Texture* pointer)
	{
		std::string string_name = NormalizeName(name);
		texResources.push_back(new Resource<Texture*>(string_name.c_str(), pointer));
		return texResources.back();
	}

	Resource<Texture*>* VulkanResourceManager::AddTextureResource(std::vector<std::string> names, Texture* pointer)
	{
		std::string output = "";
		for (std::vector<std::string>::iterator itt = names.begin(); itt != names.end(); ++itt)
		{
			output += NormalizeName((*itt));
		}

		texResources.push_back(new Resource<Texture*>(output.c_str(), pointer));
		return texResources.back();
	}

	Resource<Mesh*>* VulkanResourceManager::GetMeshResource(const char* name)
	{
		std::string string_name = NormalizeName(name);

		for (std::vector<Resource<Mesh*>*>::iterator itt = meshResources.begin(); itt != meshResources.end(); ++itt)
		{
			if ((*itt)->name == string_name)
			{
				return (*itt);
			}
		}

		return nullptr;
	}

	Resource<Texture*>* VulkanResourceManager::GetTextureResource(const char* name)
	{
		std::string string_name = NormalizeName(name);

		for (std::vector<Resource<Texture*>*>::iterator itt = texResources.begin(); itt != texResources.end(); ++itt)
		{
			if ((*itt)->name == string_name)
			{
				return (*itt);
			}
		}

		return nullptr;
	}

	Resource<Texture*>* VulkanResourceManager::GetTextureResource(std::vector<std::string> names)
	{
		std::string output = "";
		for (std::vector<std::string>::iterator itt = names.begin(); itt != names.end(); ++itt)
		{
			output += NormalizeName((*itt));
		}

		for (std::vector<Resource<Texture*>*>::iterator itt = texResources.begin(); itt != texResources.end(); ++itt)
		{
			if ((*itt)->name == output)
			{
				return (*itt);
			}
		}

		return nullptr;
	}
}