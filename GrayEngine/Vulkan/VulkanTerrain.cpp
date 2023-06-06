#include "pch.h"
#include "VulkanTerrain.h"
#include "VulkanRenderer.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace GrEngine_Vulkan
{
	void VulkanTerrain::initObject(VkDevice device, VmaAllocator allocator, GrEngine::Renderer* owner)
	{
		properties.push_back(new StringProperty(PropertyType::Shader, "Shaders\\terrain", this));
		shader_path = "Shaders\\terrain";

		p_Owner = owner;
		resources = &static_cast<VulkanRenderer*>(owner)->GetResourceManager();
		logicalDevice = device;
		memAllocator = allocator;

		AddNewProperty("PhysComponent");
		physComp->SetKinematic(2);
	}

	void VulkanTerrain::destroyObject()
	{
		VulkanAPI::DestroyPipeline(shadowPipeline);
		VulkanAPI::DestroyPipelineLayout(shadowLayout);
		if (ready)
		{
			VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &object_mesh->vertexBuffer);
			VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &object_mesh->indexBuffer);
		}
		resources->RemoveTexture(foliageMask, logicalDevice, memAllocator);
		for (int i = 0; i < 4; i++)
		{
			resources->RemoveTexture(object_displacement[i], logicalDevice, memAllocator);
			object_displacement[i] = nullptr;
			resources->RemoveTexture(object_texture[i], logicalDevice, memAllocator);
			object_texture[i] = nullptr;
			resources->RemoveTexture(object_normal[i], logicalDevice, memAllocator);
			object_normal[i] = nullptr;
		}
		object_displacement.resize(0);
		object_texture.resize(0);
		object_normal.resize(0);

		ready = false;
		VulkanDrawable::destroyObject();
	}

	void VulkanTerrain::updateObject()
	{
		VulkanDrawable::updateObject();
		updateShadowPipeline();
	}

	void VulkanTerrain::recordShadowPass(VkCommandBuffer cmd, int instances)
	{
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(cmd, 0, 1, &object_mesh->vertexBuffer.Buffer, offsets);
		vkCmdBindIndexBuffer(cmd, object_mesh->indexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowLayout, 0, 1, &descriptorSets[1].set, 0, NULL);
		ubo.model = GetObjectTransformation();
		ubo.scale = glm::vec4(GetPropertyValue(PropertyType::Scale, glm::vec3(1.f)), 1.f);
		vkCmdPushConstants(cmd, shadowLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VertexConstants), &ubo);
		vkCmdDrawIndexed(cmd, static_cast<uint32_t>(object_mesh->indices.size()), instances, 0, 0, 0);
	}

	void VulkanTerrain::UpdateFoliageMask(void* pixels)
	{
		static_cast<VulkanRenderer*>(p_Owner)->updateResource(foliageMask, (byte*)pixels);
		VulkanAPI::DestroyPipeline(computePipeline);
		VulkanAPI::DestroyPipelineLayout(computeLayout);
		VulkanAPI::DestroyPipeline(graphicsPipeline);
		VulkanAPI::DestroyPipelineLayout(pipelineLayout);
		updateObject();
	}

	void VulkanTerrain::UpdateFoliageMask(void* pixels, uint32_t width, uint32_t height, uint32_t offset_x, uint32_t offset_y)
	{
		static_cast<VulkanRenderer*>(p_Owner)->updateResource(foliageMask, (byte*)pixels, width, height, offset_x, offset_y);
		VulkanAPI::DestroyPipeline(computePipeline);
		VulkanAPI::DestroyPipelineLayout(computeLayout);
		VulkanAPI::DestroyPipeline(graphicsPipeline);
		VulkanAPI::DestroyPipelineLayout(pipelineLayout);
		updateObject();
	}

	const std::string VulkanTerrain::GetBlendMask()
	{
		if (foliageMask != nullptr)
		{
			return foliageMask->texture_collection[0];
		}
		else
		{
			return "";
		}
	}

	const std::array<std::string, 4> VulkanTerrain::GetColorTextures()
	{
		std::array<std::string, 4> res;
		for (int i = 0; i < glm::min((int)object_texture.size(), 4); i++)
		{
			res[i] = object_texture[i]->texture_collection[0];
		}

		return res;
	}

	const std::array<std::string, 4> VulkanTerrain::GetNormalTextures()
	{
		std::array<std::string, 4> res;
		for (int i = 0; i < glm::min((int)object_normal.size(), 4); i++)
		{
			res[i] = object_normal[i]->texture_collection[0];
		}

		return res;
	}

	const std::array<std::string, 4> VulkanTerrain::GetDisplacementTextures()
	{
		std::array<std::string, 4> res;
		for (int i = 0; i < glm::min((int)object_displacement.size(), 4); i++)
		{
			res[i] = object_displacement[i]->texture_collection[0];
		}

		return res;
	}

	void VulkanTerrain::GenerateTerrain(int resolution, int width, int height, int depth, std::array<std::string, 6> images, std::array<std::string, 4> normals, std::array<std::string, 4> displacements)
	{
		if (ready)
		{
			VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &object_mesh->vertexBuffer);
			VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &object_mesh->indexBuffer);
			resources->RemoveTexture(foliageMask, logicalDevice, memAllocator);
			for (int i = 0; i < 4; i++)
			{
				if (i < object_displacement.size())
				{
					resources->RemoveTexture(object_displacement[i], logicalDevice, memAllocator);
					object_displacement[i] = nullptr;
				}
				if (i < object_texture.size())
				{
					resources->RemoveTexture(object_texture[i], logicalDevice, memAllocator);
					object_texture[i] = nullptr;
				}
				if (i < object_normal.size())
				{
					resources->RemoveTexture(object_normal[i], logicalDevice, memAllocator);
					object_normal[i] = nullptr;
				}
			}
			object_displacement.resize(0);
			object_normal.resize(0);
			object_texture.resize(0);
			delete object_mesh;

			ready = false;
		}

		size = { resolution, width, depth, height };

		use_compute = true;
		VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, &size, sizeof(TerrainSize), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &terIn);
		VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, nullptr, size.resolution * size.resolution * sizeof(ComputeVertex), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &terOut);

		heightMap = static_cast<VulkanRenderer*>(p_Owner)->loadTexture({ images[0] }, GrEngine::TextureType::Height, VK_IMAGE_VIEW_TYPE_2D)->AddLink();
		foliageMask = static_cast<VulkanRenderer*>(p_Owner)->loadTexture({ images[1] }, GrEngine::TextureType::Color, VK_IMAGE_VIEW_TYPE_2D)->AddLink();
		bool res = static_cast<VulkanRenderer*>(p_Owner)->assignTextures({ images[2], images[3], images[4], images[5] }, this, GrEngine::TextureType::Color, false);
		res = static_cast<VulkanRenderer*>(p_Owner)->assignTextures({ normals[0], normals[1], normals[2], normals[3] }, this, GrEngine::TextureType::Normal, false);
		for (int i = 0; i < 4; i++)
		{
			object_displacement.push_back(static_cast<VulkanRenderer*>(p_Owner)->loadTexture({ displacements[i] }, GrEngine::TextureType::Height, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, true)->AddLink());
		}
		updateObject();

		createComputeLayout();
		createComputePipeline();

		VkCommandPoolCreateInfo cmdPoolInfo{};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = static_cast<VulkanRenderer*>(p_Owner)->compute_bit.value();
		VulkanAPI::CreateCommandPool(logicalDevice, static_cast<VulkanRenderer*>(p_Owner)->compute_bit.value(), &computePool);
		VulkanAPI::AllocateCommandBuffers(logicalDevice, computePool, &computeCmd, 1);
		VulkanAPI::BeginCommandBuffer(computeCmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		vkCmdBindPipeline(computeCmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
		vkCmdBindDescriptorSets(computeCmd, VK_PIPELINE_BIND_POINT_COMPUTE, computeLayout, 0, 1, &descriptorSets[2].set, 0, nullptr);
		vkCmdDispatch(computeCmd, 10, 1, 1);
		VulkanAPI::CreateVkFence(logicalDevice, &computeFence);
		VulkanAPI::GetDeviceQueue(logicalDevice, static_cast<VulkanRenderer*>(p_Owner)->compute_bit.value(), &computeQueue);
		VulkanAPI::EndAndSubmitCommandBuffer(logicalDevice, computePool, computeCmd, computeQueue, computeFence);

		vkWaitForFences(logicalDevice, 1, &computeFence, VK_TRUE, UINT64_MAX);
		std::vector<ComputeVertex> vert;
		vert.resize(resolution * resolution);
		vmaMapMemory(memAllocator, terOut.Allocation, (void**)&terI);
		memcpy_s(vert.data(), resolution * resolution * sizeof(ComputeVertex), terI, resolution * resolution * sizeof(ComputeVertex));
		vmaUnmapMemory(memAllocator, terOut.Allocation);

		std::unordered_map<Vertex, uint32_t> uniqueVertices{};
		object_mesh = new Mesh();

		int row = 0;
		int col = 0;
		for (std::vector<ComputeVertex>::iterator itt = vert.begin(); itt != vert.end(); ++itt)
		{
			GrEngine_Vulkan::Vertex vertex{};
			vertex.pos = (*itt).pos;
			vertex.uv = { (*itt).uv.x, (*itt).uv.y };
			vertex.uv_index = 0;

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(object_mesh->vertices.size());
				object_mesh->vertices.push_back(vertex);
			}

			if (row + 1 < resolution && col + 1 < resolution)
			{
				int index = resolution * row + col;
				object_mesh->indices.push_back(index);
				object_mesh->indices.push_back(index + resolution);
				object_mesh->indices.push_back(index + 1);

				object_mesh->indices.push_back(index + resolution);
				object_mesh->indices.push_back(index + resolution + 1);
				object_mesh->indices.push_back(index + 1);
			}

			col++;
			row += col / resolution;
			col %= resolution;
		}

		VulkanResourceManager::CalculateNormals(object_mesh);
		VulkanResourceManager::CalculateTangents(object_mesh, 200, 200);
		VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, object_mesh->vertices.data(), sizeof(object_mesh->vertices[0]) * object_mesh->vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &object_mesh->vertexBuffer);
		VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, object_mesh->indices.data(), sizeof(object_mesh->indices[0]) * object_mesh->indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &object_mesh->indexBuffer);
		calculateCollisions();

		VulkanAPI::DestroyCommandPool(computePool);
		VulkanAPI::DestroyFence(computeFence);
		VulkanAPI::DestroyPipeline(computePipeline);
		VulkanAPI::DestroyPipelineLayout(computeLayout);
		VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &terIn);
		VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &terOut);
		resources->RemoveTexture(heightMap, logicalDevice, memAllocator);
		heightMap = nullptr;
		use_compute = false;
		updateObject();
		physComp->CalculatePhysics();

		ready = true;
		was_updated = true;
	}

	void VulkanTerrain::UpdateTextures(std::array<std::string, 5> images, std::array<std::string, 4> normals, std::array<std::string, 4> displacement)
	{
		if (images[0] != "")
		{
			if (foliageMask != nullptr && images[0] == foliageMask->texture_collection[0])
			{
				static_cast<VulkanRenderer*>(p_Owner)->updateResource(foliageMask, 0);
			}
			else
			{
				resources->RemoveTexture(foliageMask, logicalDevice, memAllocator);
				foliageMask = static_cast<VulkanRenderer*>(p_Owner)->loadTexture({ images[0] }, GrEngine::TextureType::Color, VK_IMAGE_VIEW_TYPE_2D)->AddLink();
			}
		}

		for (int i = 1; i < 5; i++)
		{
			if (images[i] != "" && images[i] != object_texture[i - 1]->texture_collection[0])
			{
				resources->RemoveTexture(object_texture[i - 1], logicalDevice, memAllocator);
				object_texture[i - 1] = static_cast<VulkanRenderer*>(p_Owner)->loadTexture({ images[i] }, GrEngine::TextureType::Color, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM)->AddLink();
			}
		}

		for (int i = 0; i < 4; i++)
		{
			if (normals[i] != "" && normals[i] != object_normal[i]->texture_collection[0])
			{
				resources->RemoveTexture(object_normal[i], logicalDevice, memAllocator);
				object_normal[i] = static_cast<VulkanRenderer*>(p_Owner)->loadTexture({ normals[i] }, GrEngine::TextureType::Normal, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM)->AddLink();
			}
		}

		for (int i = 0; i < 4; i++)
		{
			if (displacement[i] != "" && displacement[i] != object_displacement[i]->texture_collection[0])
			{
				resources->RemoveTexture(object_displacement[i], logicalDevice, memAllocator);
				object_displacement[i] = static_cast<VulkanRenderer*>(p_Owner)->loadTexture({ displacement[i] }, GrEngine::TextureType::Height, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, true)->AddLink();
			}
		}

		updateObject();
		was_updated = true;
	}

	void VulkanTerrain::OffsetVertices(std::map<UINT, float> offsets)
	{
		byte* vertexbase;
		byte* indexbase;
		int numFaces = colMesh->getIndexedMeshArray()[0].m_numTriangles;
		colShape->getMeshInterface()->getLockedVertexIndexBase(&vertexbase, colMesh->getIndexedMeshArray()[0].m_numVertices, colMesh->getIndexedMeshArray()[0].m_vertexType, colMesh->getIndexedMeshArray()[0].m_vertexStride, &indexbase, colMesh->getIndexedMeshArray()[0].m_triangleIndexStride, numFaces, colMesh->getIndexedMeshArray()[0].m_indexType);

		btVector4* vertArray = (btVector4*)vertexbase;
		byte* data;
		vmaMapMemory(memAllocator, object_mesh->vertexBuffer.Allocation, (void**)&data);

		for (std::map<UINT, float>::iterator itt = offsets.begin(); itt != offsets.end(); ++itt)
		{
			object_mesh->vertices[(*itt).first].pos.y += (*itt).second;
			memcpy(data + sizeof(Vertex) * (*itt).first, &object_mesh->vertices[(*itt).first], sizeof(Vertex));
			vertArray[(*itt).first] = btVector4(vertArray[(*itt).first].x(), vertArray[(*itt).first].y() + (*itt).second, vertArray[(*itt).first].z(), 1);

			maxAABB = glm::max(maxAABB, object_mesh->vertices[(*itt).first].pos.y);
			minAABB = glm::min(minAABB, object_mesh->vertices[(*itt).first].pos.y);
		}

		colShape->getMeshInterface()->unLockVertexBase(0);
		vmaUnmapMemory(memAllocator, object_mesh->vertexBuffer.Allocation);

		btVector3 aabbMax = colShape->getLocalAabbMax();
		btVector3 aabbMin = colShape->getLocalAabbMin();
		aabbMax.setY(maxAABB);
		aabbMin.setY(minAABB);
		colMesh->setPremadeAabb(aabbMin, aabbMax);
		colShape->getOptimizedBvh()->setQuantizationValues(aabbMin, aabbMax);
		colShape->partialRefitTree(aabbMin, aabbMax);
		physComp->CalculatePhysics();

		was_updated = true;
	}

	void VulkanTerrain::UpdateVertices(std::map<UINT, float> offsets)
	{
		byte* vertexbase;
		byte* indexbase;
		int numFaces = colMesh->getIndexedMeshArray()[0].m_numTriangles;

		colShape->getMeshInterface()->getLockedVertexIndexBase(&vertexbase, colMesh->getIndexedMeshArray()[0].m_numVertices, colMesh->getIndexedMeshArray()[0].m_vertexType, colMesh->getIndexedMeshArray()[0].m_vertexStride, &indexbase, colMesh->getIndexedMeshArray()[0].m_triangleIndexStride, numFaces, colMesh->getIndexedMeshArray()[0].m_indexType);
		btVector4* vertArray = (btVector4*)vertexbase;
		byte* data;
		vmaMapMemory(memAllocator, object_mesh->vertexBuffer.Allocation, (void**)&data);

		for (std::map<UINT, float>::iterator itt = offsets.begin(); itt != offsets.end(); ++itt)
		{
			object_mesh->vertices[(*itt).first].pos.y = (*itt).second;
			memcpy(data + sizeof(Vertex) * (*itt).first, &object_mesh->vertices[(*itt).first], sizeof(Vertex));
			vertArray[(*itt).first] = btVector4(object_mesh->vertices[(*itt).first].pos.x, object_mesh->vertices[(*itt).first].pos.y, object_mesh->vertices[(*itt).first].pos.z, 1);
			maxAABB = glm::max(maxAABB, object_mesh->vertices[(*itt).first].pos.y);
			minAABB = glm::min(minAABB, object_mesh->vertices[(*itt).first].pos.y);
		}

		
		colShape->getMeshInterface()->unLockVertexBase(0);
		vmaUnmapMemory(memAllocator, object_mesh->vertexBuffer.Allocation);

		btVector3 aabbMax = colShape->getLocalAabbMax();
		btVector3 aabbMin = colShape->getLocalAabbMin();
		aabbMax.setY(maxAABB);
		aabbMin.setY(minAABB);
		colMesh->setPremadeAabb(aabbMin, aabbMax);
		colShape->getOptimizedBvh()->setQuantizationValues(aabbMin, aabbMax);
		colShape->partialRefitTree(aabbMin, aabbMax);
		physComp->CalculatePhysics();

		was_updated = true;
	}

	glm::vec4& VulkanTerrain::GetVertexPosition(UINT at)
	{
		return object_mesh->vertices[at].pos;
	}

	void VulkanTerrain::SaveTerrain(const char* filepath)
	{
		if (!was_updated) return;

		std::fstream new_file;
		new_file.open(filepath, std::fstream::out | std::ios::trunc);

		if (!new_file)
		{
			Logger::Out("Couldn't create file for saving!", OutputType::Error);
			return;
		}

		new_file << "w " << size.width << std::endl;
		new_file << "h " << size.height << std::endl;
		new_file << "d " << size.depth << std::endl;
		new_file << "r " << size.resolution << std::endl;
		new_file << "m " << (foliageMask->texture_collection[0] == "" ? "empty_texture" : foliageMask->texture_collection[0]) << std::endl;

		for (std::vector<Texture*>::iterator itt = object_texture.begin(); itt != object_texture.end(); ++itt)
		{
			new_file << "t " << ((*itt)->texture_collection[0] == "" ? "empty_texture" : (*itt)->texture_collection[0]) << std::endl;
		}

		for (std::vector<Texture*>::iterator itt = object_normal.begin(); itt != object_normal.end(); ++itt)
		{
			new_file << "nt " << ((*itt)->texture_collection[0] == "" ? "empty_texture" : (*itt)->texture_collection[0]) << std::endl;
		}

		for (std::vector<Texture*>::iterator itt = object_displacement.begin(); itt != object_displacement.end(); ++itt)
		{
			new_file << "dt " << ((*itt)->texture_collection[0] == "" ? "empty_texture" : (*itt)->texture_collection[0]) << std::endl;
		}

		for (std::vector<GrEngine_Vulkan::Vertex>::iterator itt = object_mesh->vertices.begin(); itt != object_mesh->vertices.end(); ++itt)
		{
			new_file << "v " << (*itt).pos.x << " " << (*itt).pos.y << " " << (*itt).pos.z << std::endl;
			new_file << "u " << (*itt).uv.x << " " << (*itt).uv.y << std::endl;
			new_file << "ev" << std::endl;
		}

		new_file << '\0';
		new_file.close();
		was_updated = false;
	}

	bool VulkanTerrain::LoadTerrain(const char* filepath)
	{
		if (ready)
		{
			VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &object_mesh->vertexBuffer);
			VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &object_mesh->indexBuffer);
			resources->RemoveTexture(foliageMask, logicalDevice, memAllocator);
			for (int i = 0; i < 4; i++)
			{
				if (i < object_displacement.size())
				{
					resources->RemoveTexture(object_displacement[i], logicalDevice, memAllocator);
					object_displacement[i] = nullptr;
				}
				if (i < object_texture.size())
				{
					resources->RemoveTexture(object_texture[i], logicalDevice, memAllocator);
					object_texture[i] = nullptr;
				}
				if (i < object_normal.size())
				{
					resources->RemoveTexture(object_normal[i], logicalDevice, memAllocator);
					object_normal[i] = nullptr;
				}
			}
			object_displacement.resize(0);
			object_normal.resize(0);
			object_texture.resize(0);
			delete object_mesh;

			ready = false;
		}

		object_mesh = new Mesh();
		std::ifstream file(filepath, std::ios::ate | std::ios::binary);

		if (!file)
		{
			Logger::Out("Couldn't open terrain %s", OutputType::Error, filepath);
			return false;
		}

		file.seekg(0);
		std::string stream;
		int vertex = 0;
		Vertex vert{};
		size = { 1, 1, 1, 1 };
		std::array<std::string, 4> textures;
		std::array<std::string, 4> normals;
		std::array<std::string, 4> displacements;
		int map_index = 0;
		int normal_index = 0;
		int height_index = 0;

		while (file >> stream && !file.eof())
		{
			if (stream == "v")
			{
				file >> stream;
				vert.pos[0] = std::atof(stream.c_str());
				file >> stream;
				vert.pos[1] = std::atof(stream.c_str());
				file >> stream;
				vert.pos[2] = std::atof(stream.c_str());
			}
			else if (stream == "u")
			{
				file >> stream;
				vert.uv[0] = std::atof(stream.c_str());
				file >> stream;
				vert.uv[1] = std::atof(stream.c_str());
			}
			else if (stream == "w")
			{
				file >> stream;
				size.width = std::atof(stream.c_str());
			}
			else if (stream == "h")
			{
				file >> stream;
				size.height = std::atof(stream.c_str());
			}
			else if (stream == "d")
			{
				file >> stream;
				size.depth = std::atof(stream.c_str());
			}
			else if (stream == "r")
			{
				file >> stream;
				size.resolution = std::atof(stream.c_str());
			}
			else if (stream == "ev")
			{
				if ((vertex + 1) / size.resolution < size.resolution - 1 && (vertex + 1) % size.resolution > 0)
				{
					object_mesh->indices.push_back(vertex);
					object_mesh->indices.push_back(vertex + size.resolution);
					object_mesh->indices.push_back(vertex + 1);

					object_mesh->indices.push_back(vertex + size.resolution);
					object_mesh->indices.push_back(vertex + size.resolution + 1);
					object_mesh->indices.push_back(vertex + 1);
				}

				vertex++;
				object_mesh->vertices.push_back(vert);
				vert = {};
			}
			else if (stream == "m")
			{
				file >> stream;
				foliageMask = static_cast<VulkanRenderer*>(p_Owner)->loadTexture({ stream }, GrEngine::TextureType::Color, VK_IMAGE_VIEW_TYPE_2D)->AddLink();
			}
			else if (stream == "t" && map_index < 4)
			{
				file >> stream;
				if (stream != "empty_texture")
				{
					textures[map_index] = stream;
				}

				map_index++;
			}
			else if (stream == "nt" && normal_index < 4)
			{
				file >> stream;
				if (stream != "empty_texture")
				{
					normals[normal_index] = stream;
				}

				normal_index++;
			}
			else if (stream == "dt" && height_index < 4)
			{
				file >> stream;
				if (stream != "empty_texture")
				{
					displacements[height_index] = stream;
				}

				height_index++;
			}
		}
		file.close();

		static_cast<VulkanRenderer*>(p_Owner)->assignTextures({ textures[0], textures[1], textures[2], textures[3] }, this, GrEngine::TextureType::Color, false);
		static_cast<VulkanRenderer*>(p_Owner)->assignTextures({ normals[0], normals[1], normals[2], normals[3] }, this, GrEngine::TextureType::Normal, false);
		for (int i = 0; i < 4; i++)
		{
			object_displacement.push_back(static_cast<VulkanRenderer*>(p_Owner)->loadTexture({ displacements[i] }, GrEngine::TextureType::Height, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, true)->AddLink());
		}

		VulkanResourceManager::CalculateNormals(object_mesh);
		VulkanResourceManager::CalculateTangents(object_mesh, 200, 200);
		VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, object_mesh->vertices.data(), sizeof(object_mesh->vertices[0]) * object_mesh->vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &object_mesh->vertexBuffer);
		VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, object_mesh->indices.data(), sizeof(object_mesh->indices[0]) * object_mesh->indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &object_mesh->indexBuffer);
		calculateCollisions();
		updateObject();
		ready = true;

		return true;
	}

	void VulkanTerrain::calculateCollisions()
	{
		std::unordered_map<GrEngine_Vulkan::Vertex, uint32_t> uniqueVertices{};

		if (colMesh != nullptr)
		{
			delete colMesh;
			colMesh = nullptr;
			delete colShape;
		}

		colMesh = new btTriangleMesh();
		int index = 0;
		maxAABB = object_mesh->vertices.front().pos.y;
		minAABB = object_mesh->vertices.front().pos.y;
		for (std::vector<GrEngine_Vulkan::Vertex>::iterator itt = object_mesh->vertices.begin(); itt != object_mesh->vertices.end(); ++itt)
		{
			colMesh->findOrAddVertex(btVector3((*itt).pos.x, (*itt).pos.y, (*itt).pos.z), false);
			maxAABB = glm::max(maxAABB, (*itt).pos.y);
			minAABB = glm::min(minAABB, (*itt).pos.y);
		}

		for (int i = 3; i < object_mesh->indices.size(); i += 3)
		{
			colMesh->addTriangleIndices(object_mesh->indices[i - 3], object_mesh->indices[i - 2], object_mesh->indices[i - 1]);
		}

		colShape = new btBvhTriangleMeshShape(colMesh, true);
		physComp->AddCollisionResource("TerrainComonent", colShape);
		physComp->CalculatePhysics();
	}

	void VulkanTerrain::UpdateCollision()
	{
		VulkanResourceManager::CalculateNormals(object_mesh);
		VulkanResourceManager::CalculateTangents(object_mesh, 200, 200);
		btVector3 aabbMax = colShape->getLocalAabbMax();
		btVector3 aabbMin = colShape->getLocalAabbMin();

		colShape->refitTree(aabbMin, aabbMax);
		physComp->CalculatePhysics();
	}

	bool VulkanTerrain::pushConstants(VkCommandBuffer cmd)
	{
		return true;
	}

	bool VulkanTerrain::createPipelineLayout()
	{
		return VulkanAPI::CreatePipelineLayout(logicalDevice, { }, { descriptorSets[0].descriptorSetLayout }, &pipelineLayout);
	}

	bool VulkanTerrain::createComputeLayout()
	{
		return VulkanAPI::CreatePipelineLayout(logicalDevice, {}, { descriptorSets[2].descriptorSetLayout }, &computeLayout);
	}

	bool VulkanTerrain::createGraphicsPipeline()
	{
		std::string solution_path = GrEngine::Globals::getExecutablePath();

		std::vector<char> vertShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_vert.spv");
		std::vector<char> teseShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_tese.spv");
		std::vector<char> tescShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_tesc.spv");
		std::vector<char> geomShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_geom.spv");
		std::vector<char> fragShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_frag.spv");
		

		VkShaderModule shaders[5] = { VulkanAPI::m_createShaderModule(logicalDevice, vertShaderCode),
			VulkanAPI::m_createShaderModule(logicalDevice, geomShaderCode),
			VulkanAPI::m_createShaderModule(logicalDevice, fragShaderCode),
			VulkanAPI::m_createShaderModule(logicalDevice, teseShaderCode),
			VulkanAPI::m_createShaderModule(logicalDevice, tescShaderCode)
		};

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = shaders[0];
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo geomShaderStageInfo{};
		geomShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		geomShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
		geomShaderStageInfo.module = shaders[1];
		geomShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = shaders[2];
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo teseShaderStageInfo{};
		teseShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		teseShaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		teseShaderStageInfo.module = shaders[3];
		teseShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo tescShaderStageInfo{};
		tescShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		tescShaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		tescShaderStageInfo.module = shaders[4];
		tescShaderStageInfo.pName = "main";

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { vertShaderStageInfo, geomShaderStageInfo, fragShaderStageInfo, teseShaderStageInfo, tescShaderStageInfo };

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();

		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = nullptr;
		viewportState.scissorCount = 1;
		viewportState.pScissors = nullptr;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		//rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
		rasterizer.lineWidth = 1.0f;
		//rasterizer.cullMode = VK_CULL_MODE_NONE;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		VkPipelineMultisampleStateCreateInfo multisampleState{};
		multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleState.sampleShadingEnable = VK_FALSE;
		multisampleState.minSampleShading = 0;

		std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;
		blendAttachmentStates.resize(4);
		blendAttachmentStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendAttachmentStates[0].blendEnable = VK_FALSE;
		blendAttachmentStates[1].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendAttachmentStates[1].blendEnable = VK_FALSE;
		blendAttachmentStates[2].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendAttachmentStates[2].blendEnable = VK_FALSE;
		blendAttachmentStates[3].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendAttachmentStates[3].blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlendState{};
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.logicOpEnable = VK_FALSE;
		colorBlendState.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
		colorBlendState.pAttachments = blendAttachmentStates.data();

		std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicCreateInfo{};
		dynamicCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicCreateInfo.pNext = nullptr;
		dynamicCreateInfo.flags = 0;
		dynamicCreateInfo.dynamicStateCount = (uint32_t)dynamicStates.size();
		dynamicCreateInfo.pDynamicStates = dynamicStates.data();

		VkPipelineDepthStencilStateCreateInfo _depthStencil{};
		_depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		_depthStencil.pNext = nullptr;
		_depthStencil.depthTestEnable = VK_TRUE;
		_depthStencil.depthWriteEnable = VK_TRUE;
		_depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		_depthStencil.depthBoundsTestEnable = VK_FALSE;
		_depthStencil.minDepthBounds = 0.0f; // Optional
		_depthStencil.maxDepthBounds = 1.0f; // Optional
		_depthStencil.stencilTestEnable = VK_FALSE;

		VkPipelineTessellationStateCreateInfo tesselationInfo{};
		tesselationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
		tesselationInfo.patchControlPoints = 3;

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = shaderStages.size();
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampleState;
		pipelineInfo.pColorBlendState = &colorBlendState;
		pipelineInfo.pDynamicState = &dynamicCreateInfo;
		pipelineInfo.pTessellationState = &tesselationInfo;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = static_cast<VulkanRenderer*>(p_Owner)->getRenderPass();
		pipelineInfo.subpass = 0;
		pipelineInfo.pDepthStencilState = &_depthStencil;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		if (VulkanAPI::CreateGraphicsPipeline(logicalDevice, &pipelineInfo, &graphicsPipeline) != true)
			return false;

		vkDestroyShaderModule(logicalDevice, shaders[0], nullptr);
		vkDestroyShaderModule(logicalDevice, shaders[1], nullptr);
		vkDestroyShaderModule(logicalDevice, shaders[2], nullptr);
		vkDestroyShaderModule(logicalDevice, shaders[3], nullptr);
		vkDestroyShaderModule(logicalDevice, shaders[4], nullptr);

		return true;
	}

	void VulkanTerrain::updateShadowPipeline()
	{
		VulkanAPI::DestroyPipeline(shadowPipeline);
		VulkanAPI::DestroyPipelineLayout(shadowLayout);

		VkPushConstantRange pushConstant;
		pushConstant.offset = 0;
		pushConstant.size = sizeof(VertexConstants);
		pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VulkanAPI::CreatePipelineLayout(logicalDevice, { pushConstant }, { descriptorSets[1].descriptorSetLayout }, &shadowLayout);

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();

		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;
		VkPipelineRasterizationStateCreateInfo rasterizationState{};
		rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationState.depthClampEnable = VK_TRUE;
		rasterizationState.rasterizerDiscardEnable = VK_FALSE;
		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationState.lineWidth = 1.0f;
		rasterizationState.cullMode = VK_CULL_MODE_NONE;
		//rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
		rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationState.depthBiasEnable = VK_TRUE;
		rasterizationState.depthBiasConstantFactor = 2.75f;
		rasterizationState.depthBiasClamp = 0.01f;
		rasterizationState.depthBiasSlopeFactor = 2.75f;

		VkPipelineColorBlendStateCreateInfo colorBlendState{};
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.attachmentCount = 0;
		colorBlendState.pAttachments = nullptr;

		VkPipelineDepthStencilStateCreateInfo depthStencilState{};
		depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilState.pNext = nullptr;
		depthStencilState.depthTestEnable = VK_TRUE;
		depthStencilState.depthWriteEnable = VK_TRUE;
		depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencilState.depthBoundsTestEnable = VK_FALSE;
		depthStencilState.minDepthBounds = 0.0f; // Optional
		depthStencilState.maxDepthBounds = 1.0f; // Optional
		depthStencilState.stencilTestEnable = VK_FALSE;

		std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.pNext = nullptr;
		dynamicState.flags = 0;
		dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
		dynamicState.pDynamicStates = dynamicStates.data();

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = nullptr;
		viewportState.scissorCount = 1;
		viewportState.pScissors = nullptr;

		VkPipelineMultisampleStateCreateInfo multisampleState{};
		multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleState.sampleShadingEnable = VK_FALSE;
		multisampleState.minSampleShading = 0;

		std::string solution_path = GrEngine::Globals::getExecutablePath();
		std::vector<char> vertShaderCode = GrEngine::Globals::readFile(solution_path + "Shaders//shadows_vert.spv");
		std::vector<char> geomShaderCode = GrEngine::Globals::readFile(solution_path + "Shaders//shadows_geom.spv");
		VkShaderModule shaders[2] = { VulkanAPI::m_createShaderModule(logicalDevice, vertShaderCode), VulkanAPI::m_createShaderModule(logicalDevice, geomShaderCode) };

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = shaders[0];
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo geomShaderStageInfo{};
		geomShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		geomShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
		geomShaderStageInfo.module = shaders[1];
		geomShaderStageInfo.pName = "main";

		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = { vertShaderStageInfo, geomShaderStageInfo };

		uint32_t spec = static_cast<VulkanRenderer*>(p_Owner)->lightsCount();
		std::array<VkSpecializationMapEntry, 1> entries;
		entries[0].constantID = 0;
		entries[0].offset = 0;
		entries[0].size = sizeof(uint32_t);

		VkSpecializationInfo specializationInfo;
		specializationInfo.mapEntryCount = entries.size();
		specializationInfo.pMapEntries = entries.data();
		specializationInfo.dataSize = sizeof(uint32_t);
		specializationInfo.pData = &spec;

		shaderStages[0].pSpecializationInfo = &specializationInfo;

		VkGraphicsPipelineCreateInfo pipelineCI{};

		pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCI.layout = shadowLayout;
		pipelineCI.renderPass = static_cast<VulkanRenderer*>(p_Owner)->shadowPass;
		pipelineCI.basePipelineIndex = 0;
		pipelineCI.pVertexInputState = &vertexInputInfo;
		pipelineCI.pInputAssemblyState = &inputAssembly;
		pipelineCI.pRasterizationState = &rasterizationState;
		pipelineCI.pColorBlendState = &colorBlendState;
		pipelineCI.pMultisampleState = &multisampleState;
		pipelineCI.pViewportState = &viewportState;
		pipelineCI.pDepthStencilState = &depthStencilState;
		pipelineCI.pDynamicState = &dynamicState;
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();
		pipelineCI.subpass = 0;

		bool res = VulkanAPI::CreateGraphicsPipeline(logicalDevice, &pipelineCI, &shadowPipeline);

		vkDestroyShaderModule(logicalDevice, shaders[0], nullptr);
		vkDestroyShaderModule(logicalDevice, shaders[1], nullptr);
	}

	bool VulkanTerrain::createComputePipeline()
	{
		std::string solution_path = GrEngine::Globals::getExecutablePath();

		std::vector<char> compShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_comp.spv");
		VkShaderModule shader = VulkanAPI::m_createShaderModule(logicalDevice, compShaderCode);

		VkPipelineShaderStageCreateInfo compShaderStageInfo{};
		compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		compShaderStageInfo.module = shader;
		compShaderStageInfo.pName = "main";

		VkComputePipelineCreateInfo computeInfo{};
		computeInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computeInfo.layout = computeLayout;
		computeInfo.stage = compShaderStageInfo;

		if (VulkanAPI::CreateComputePipeline(logicalDevice, &computeInfo, &computePipeline) != true)
			return false;

		vkDestroyShaderModule(logicalDevice, shader, nullptr);

		return true;
	}

	void VulkanTerrain::createDescriptors()
	{
		descriptorSets.clear();
		descriptorSets.resize(2 + use_compute);
		descriptorSets[0].bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		descriptorSets[1].bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		populateDescriptorSets();

		createDescriptorLayout();
		createDescriptorPool();
		createDescriptorSet();
	}


	void VulkanTerrain::populateDescriptorSets()
	{
		//VkDescriptorImageInfo texInfo{};
		//texInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		//texInfo.imageView = object_texture->textureImageView;
		//texInfo.sampler = object_texture->textureSampler;
		VkDescriptorImageInfo maskInfo{};
		maskInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		maskInfo.imageView = foliageMask->textureImageView;
		maskInfo.sampler = foliageMask->textureSampler;

		subscribeDescriptor(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<VulkanRenderer*>(p_Owner)->viewProjUBO.BufferInfo);
		subscribeDescriptor(VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maskInfo);

		std::vector<VkDescriptorImageInfo> imageInfo;
		std::vector<VkDescriptorImageInfo> normalsInfo;
		std::vector<VkDescriptorImageInfo> disInfo;
		if (object_texture.size() > 0)
		{
			for (int i = 0; i < object_texture.size(); i++)
			{
				imageInfo.push_back(object_texture[i]->texInfo.descriptor);
			}
			subscribeDescriptor(VK_SHADER_STAGE_FRAGMENT_BIT, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfo);
		}

		if (object_normal.size() > 0)
		{
			for (int i = 0; i < object_normal.size(); i++)
			{
				normalsInfo.push_back(object_normal[i]->texInfo.descriptor);
			}
			subscribeDescriptor(VK_SHADER_STAGE_FRAGMENT_BIT, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, normalsInfo);
		}
		
		if (object_displacement.size() > 0)
		{
			for (int i = 0; i < object_displacement.size(); i++)
			{
				disInfo.push_back(object_displacement[i]->texInfo.descriptor);
			}
			subscribeDescriptor(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, disInfo);
		}

		subscribeDescriptor(VK_SHADER_STAGE_VERTEX_BIT, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<VulkanRenderer*>(p_Owner)->shadowBuffer.BufferInfo, 1);

		int binding = 3;

		if (use_compute)
		{
			VkDescriptorImageInfo heightInfo{};
			heightInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			heightInfo.imageView = heightMap->textureImageView;
			heightInfo.sampler = heightMap->textureSampler;
			descriptorSets[2].bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
			VkDescriptorBufferInfo inBuffer{};
			inBuffer.buffer = terIn.Buffer;
			inBuffer.offset = 0;
			inBuffer.range = sizeof(TerrainSize);
			VkDescriptorBufferInfo outBuffer{};
			outBuffer.buffer = terOut.Buffer;
			outBuffer.offset = 0;
			outBuffer.range = size.resolution * size.resolution * sizeof(ComputeVertex);
			subscribeDescriptor(VK_SHADER_STAGE_COMPUTE_BIT, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, inBuffer, 2);
			subscribeDescriptor(VK_SHADER_STAGE_COMPUTE_BIT, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, outBuffer, 2);
			subscribeDescriptor(VK_SHADER_STAGE_COMPUTE_BIT, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, heightInfo, 2);
		}
	}
};