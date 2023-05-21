#include "pch.h"
#include "VulkanResourceManager.h"
#include "Engine.h"
#include "VulkanAPI.h"

namespace GrEngine_Vulkan
{
	void VulkanResourceManager::RemoveMesh(Mesh* resource, VkDevice device, VmaAllocator allocator)
	{
		if (resource == nullptr) return;

		for (std::vector<Resource<Mesh*>*>::iterator itt = meshResources.begin(); itt != meshResources.end(); ++itt)
		{
			if ((*itt)->Compare(resource))
			{
				Resource<Mesh*>* cur_resource = (*itt);
				Mesh* mesh = cur_resource->PopResource();
				if (cur_resource->getNumOfLinks() == 0)
				{
					Logger::Out("Mesh resource %s was removed", OutputColor::Blue, OutputType::Log, cur_resource->name.c_str());
					VulkanAPI::m_destroyShaderBuffer(device, allocator, &mesh->indexBuffer);
					VulkanAPI::m_destroyShaderBuffer(device, allocator, &mesh->vertexBuffer);
					delete cur_resource;
					cur_resource = nullptr;
					meshResources.erase(itt);
					break;
				}
			}
		}
	}

	void VulkanResourceManager::RemoveTexture(Texture* resource, VkDevice device, VmaAllocator allocator)
	{
		if (resource == nullptr) return;

		for (std::vector<Resource<Texture*>*>::iterator itt = texResources.begin(); itt != texResources.end(); ++itt)
		{
			if ((*itt)->Compare(resource))
			{
				Resource<Texture*>* cur_resource = (*itt);
				Texture* texture = cur_resource->PopResource();
				if (cur_resource->getNumOfLinks() == 0) // && !(string_name.find("empty_texture") != std::string::npos && string_name.size() < 25)
				{
					Logger::Out("Texture resource %s was removed", OutputColor::Blue, OutputType::Log, (*itt)->name.c_str());
					VulkanAPI::m_destroyTexture(device, allocator, texture);
					delete cur_resource;
					cur_resource = nullptr;
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
				(*itt)->AddLink();
				break;
			}
		}
	}

	void VulkanResourceManager::Clean(VkDevice device, VmaAllocator allocator)
	{
		for (std::vector<Resource<Mesh*>*>::iterator itt = meshResources.begin(); itt != meshResources.end(); ++itt)
		{
			Mesh* mesh = (*itt)->PopResource();

			if ((*itt)->getNumOfLinks() == 0)
			{
				VulkanAPI::m_destroyShaderBuffer(device, allocator, &mesh->indexBuffer);
				VulkanAPI::m_destroyShaderBuffer(device, allocator, &mesh->vertexBuffer);
				delete (*itt);
				(*itt) = nullptr;
			}
		}

		for (std::vector<Resource<Texture*>*>::iterator itt = texResources.begin(); itt != texResources.end(); ++itt)
		{
			Texture* texture = (*itt)->PopResource();

			if ((*itt)->getNumOfLinks() == 0)
			{
				VulkanAPI::m_destroyTexture(device, allocator, texture);
				delete (*itt);
				(*itt) = nullptr;
			}
		}

		meshResources.clear();
		texResources.clear();
	}

	Resource<Mesh*>* VulkanResourceManager::AddMeshResource(const char* name, Mesh* pointer)
	{
		Resource<Mesh*>* res;
		std::string string_name = NormalizeName(name);
		res = GetMeshResource(string_name.c_str());
		if (res == nullptr)
		{
			meshResources.push_back(new Resource<Mesh*>(string_name.c_str(), pointer));
			res = meshResources.back();
		}

		return res;
	}

	Resource<Texture*>* VulkanResourceManager::AddTextureResource(const char* name, Texture* pointer)
	{
		Resource<Texture*>* res;
		std::string string_name = NormalizeName(name);
		res = GetTextureResource(string_name.c_str());
		if (res == nullptr)
		{
			texResources.push_back(new Resource<Texture*>(string_name.c_str(), pointer));
			res = texResources.back();
		}
		return res;
	}

	Resource<Texture*>* VulkanResourceManager::AddTextureResource(std::vector<std::string> names, Texture* pointer)
	{
		Resource<Texture*>* res;
		std::string output = "";
		for (std::vector<std::string>::iterator itt = names.begin(); itt != names.end(); ++itt)
		{
			output += NormalizeName((*itt));
		}

		res = GetTextureResource(output.c_str());
		if (res == nullptr)
		{
			texResources.push_back(new Resource<Texture*>(output.c_str(), pointer));
			res = texResources.back();
		}
		return res;
	}

	Resource<Texture*>* VulkanResourceManager::AddTextureResource(std::vector<std::string> names, Texture* pointer, GrEngine::TextureType type)
	{
		Resource<Texture*>* res;
		std::string output = "";
		for (std::vector<std::string>::iterator itt = names.begin(); itt != names.end(); ++itt)
		{
			output += NormalizeName((*itt));
		}


		switch (type)
		{
		case GrEngine::Color:
			output = "Color_" + output;
			break;
		case GrEngine::Normal:
			output = "Normal_" + output;
			break;
		case GrEngine::Height:
			output = "Height_" + output;
			break;
		default:
			break;
		}

		res = GetTextureResource(output.c_str());
		if (res == nullptr)
		{
			texResources.push_back(new Resource<Texture*>(output.c_str(), pointer));
			res = texResources.back();
		}
		return res;
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

	Resource<Texture*>* VulkanResourceManager::GetTextureResource(std::vector<std::string> names, GrEngine::TextureType type)
	{
		std::string output = "";
		for (std::vector<std::string>::iterator itt = names.begin(); itt != names.end(); ++itt)
		{
			output += NormalizeName((*itt));
		}

		switch (type)
		{
		case GrEngine::Color:
			output = "Color_" + output;
			break;
		case GrEngine::Normal:
			output = "Normal_" + output;
			break;
		case GrEngine::Height:
			output = "Height_" + output;
			break;
		default:
			break;
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

	void VulkanResourceManager::CalculateNormals(GrEngine_Vulkan::Mesh* target, bool clockwise)
	{
		if (target != nullptr)
		{
			std::map<UINT, glm::vec4> normals;
			for (int i = 0; i < target->indices.size(); i += 3)
			{
				Vertex& A = target->vertices[target->indices[i]];
				Vertex& B = target->vertices[target->indices[i + 1]];
				Vertex& C = target->vertices[target->indices[i + 2]];
				glm::vec3 faceNormal;
				if (clockwise)
				{
					faceNormal = glm::normalize(glm::cross(glm::vec3(C.pos) - glm::vec3(A.pos), glm::vec3(B.pos) - glm::vec3(A.pos)));

				}
				else
				{
					faceNormal = glm::normalize(glm::cross(glm::vec3(B.pos) - glm::vec3(A.pos), glm::vec3(C.pos) - glm::vec3(A.pos)));
				}

				normals[target->indices[i]] += glm::vec4(faceNormal, normals[target->indices[i]].w++);
				normals[target->indices[i + 1]] += glm::vec4(faceNormal, normals[target->indices[i + 1]].w++);
				normals[target->indices[i + 2]] += glm::vec4(faceNormal, normals[target->indices[i + 2]].w++);
			}

			for (std::map<UINT, glm::vec4>::iterator itt = normals.begin(); itt != normals.end(); ++itt)
			{
				target->vertices[(*itt).first].norm = glm::normalize(glm::vec3((*itt).second.x, (*itt).second.y, (*itt).second.z) / (*itt).second.w);
			}
		}
	}

	void VulkanResourceManager::CalculateTangents(GrEngine_Vulkan::Mesh* target, float u_scale, float v_scale, bool clockwise)
	{
		if (target != nullptr)
		{
			for (int i = 0; i < target->indices.size(); i += 3)
			{
				Vertex& A = target->vertices[target->indices[i]];
				Vertex& B = target->vertices[target->indices[i + 1]];
				Vertex& C = target->vertices[target->indices[i + 2]];

				glm::vec2 uv0 = (A.uv - 0.5f) * u_scale + (0.5f * v_scale);
				glm::vec2 uv1 = (B.uv - 0.5f) * u_scale + (0.5f * v_scale);
				glm::vec2 uv2 = (C.uv - 0.5f) * u_scale + (0.5f * v_scale);

				glm::vec3 diff1;
				glm::vec3 diff2;
				glm::vec2 delta1;
				glm::vec2 delta2;

				if (clockwise)
				{
					diff1 = C.pos - A.pos;
					diff2 = B.pos - A.pos;
					delta1 = glm::vec2(uv2.x, 1.f - uv2.y) - glm::vec2(uv0.x, 1.f - uv0.y);
					delta2 = glm::vec2(uv1.x, 1.f - uv1.y) - glm::vec2(uv0.x, 1.f - uv0.y);
				}
				else
				{
					diff1 = B.pos - A.pos;
					diff2 = C.pos - A.pos;
					delta1 = glm::vec2(uv1.x, 1.f - uv1.y) - glm::vec2(uv0.x, 1.f - uv0.y);
					delta2 = glm::vec2(uv2.x, 1.f - uv2.y) - glm::vec2(uv0.x, 1.f - uv0.y);
				}

				float f = 1.0f / (delta1.x * delta2.y - delta1.y * delta2.x);
				glm::vec3 tangent;
				glm::vec3 bitangent;

				tangent.x = f * (delta2.y * diff1.x - delta1.y * diff2.x);
				tangent.y = f * (delta2.y * diff1.y - delta1.y * diff2.y);
				tangent.z = f * (delta2.y * diff1.z - delta1.y * diff2.z);

				target->vertices[target->indices[i]].tang = tangent;
				target->vertices[target->indices[i + 1]].tang = tangent;
				target->vertices[target->indices[i + 2]].tang = tangent;
			}
		}
	}
}