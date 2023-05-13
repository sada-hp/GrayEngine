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
		if (ready)
		{
			VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &object_mesh->vertexBuffer);
			VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &object_mesh->indexBuffer);
		}
		resources->RemoveTexture(foliageMask, logicalDevice, memAllocator);
		resources->RemoveTexture(object_displacement, logicalDevice, memAllocator);

		ready = false;
		VulkanDrawable::destroyObject();
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
		if (object_texture != nullptr)
		{
			for (int i = 0; i < glm::min(4, (int)object_texture->texture_collection.size()); i++)
			{
				res[i] = object_texture->texture_collection[i];
			}
		}

		return res;
	}

	const std::array<std::string, 4> VulkanTerrain::GetNormalTextures()
	{
		std::array<std::string, 4> res;
		if (object_normal != nullptr)
		{
			for (int i = 0; i < glm::min(4, (int)object_normal->texture_collection.size()); i++)
			{
				res[i] = object_normal->texture_collection[i];
			}
		}

		return res;
	}

	const std::array<std::string, 4> VulkanTerrain::GetDisplacementTextures()
	{
		std::array<std::string, 4> res;
		if (object_displacement != nullptr)
		{
			for (int i = 0; i < glm::min(4, (int)object_displacement->texture_collection.size()); i++)
			{
				res[i] = object_displacement->texture_collection[i];
			}
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
			resources->RemoveTexture(object_displacement, logicalDevice, memAllocator);
			delete object_mesh;

			ready = false;
		}

		size = { resolution, width, depth, height };

		use_compute = true;
		VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, &size, sizeof(TerrainSize), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &terIn);
		VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, nullptr, size.resolution * size.resolution * sizeof(ComputeVertex), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &terOut);

		heightMap = static_cast<VulkanRenderer*>(p_Owner)->loadTexture({ images[0] }, VK_IMAGE_VIEW_TYPE_2D_ARRAY)->AddLink();
		//object_normal = static_cast<VulkanRenderer*>(p_Owner)->loadTexture({ "Content\\Terrain\\Stone\\Stone_NM.png" }, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM)->AddLink();
		object_displacement = static_cast<VulkanRenderer*>(p_Owner)->loadTexture({ displacements[0], displacements[1], displacements[2], displacements[3] }, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, true)->AddLink();
		foliageMask = static_cast<VulkanRenderer*>(p_Owner)->loadTexture({ images[1] }, VK_IMAGE_VIEW_TYPE_2D_ARRAY)->AddLink();
		bool res = static_cast<VulkanRenderer*>(p_Owner)->assignTextures({ images[2], images[3], images[4], images[5] }, this, false);
		res = static_cast<VulkanRenderer*>(p_Owner)->assignNormals({ normals[0], normals[1], normals[2], normals[3] }, this, false);
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
		vkCmdBindDescriptorSets(computeCmd, VK_PIPELINE_BIND_POINT_COMPUTE, computeLayout, 0, 1, &descriptorSets[1].set, 0, nullptr);
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
		use_compute = false;
		updateObject();
		physComp->CalculatePhysics();

		ready = true;
		was_updated = true;
	}

	void VulkanTerrain::UpdateTextures(std::array<std::string, 5> images, std::array<std::string, 4> normals, std::array<std::string, 4> displacement)
	{
		std::vector<std::string> textures_v;
		std::vector<std::string> normals_v;
		std::vector<std::string> heights_v;
		textures_v.resize(object_texture->texture_collection.size());
		normals_v.resize(object_normal->texture_collection.size());
		heights_v.resize(object_displacement->texture_collection.size());
		std::copy(object_texture->texture_collection.begin(), object_texture->texture_collection.end(), textures_v.begin());
		std::copy(object_normal->texture_collection.begin(), object_normal->texture_collection.end(), normals_v.begin());
		std::copy(object_displacement->texture_collection.begin(), object_displacement->texture_collection.end(), heights_v.begin());

		if (images[0] != "")
		{
			if (foliageMask != nullptr && images[0] == foliageMask->texture_collection[0])
			{
				static_cast<VulkanRenderer*>(p_Owner)->updateResource(foliageMask, 0);
			}
			else
			{
				resources->RemoveTexture(foliageMask, logicalDevice, memAllocator);
				foliageMask = static_cast<VulkanRenderer*>(p_Owner)->loadTexture({ images[0] }, VK_IMAGE_VIEW_TYPE_2D_ARRAY)->AddLink();
			}
		}

		bool update_col = false;
		bool update_nor = false;
		bool update_dis = false;
		for (int i = 1; i < 5; i++)
		{
			if (images[i] != "")
			{
				textures_v[i - 1] = images[i];
				update_col = true;
			}
		}

		for (int i = 0; i < 4; i++)
		{
			if (normals[i] != "")
			{
				normals_v[i] = normals[i];
				update_nor = true;
			}
		}

		for (int i = 0; i < 4; i++)
		{
			if (displacement[i] != "")
			{
				heights_v[i] = displacement[i];
				update_dis = true;
			}
		}

		if (update_col)
		{
			static_cast<VulkanRenderer*>(p_Owner)->assignTextures(textures_v, this, false);
		}
		if (update_nor)
		{
			static_cast<VulkanRenderer*>(p_Owner)->assignNormals(normals_v, this, false);
		}
		if (update_dis)
		{
			resources->RemoveTexture(object_displacement, logicalDevice, memAllocator);
			object_displacement = static_cast<VulkanRenderer*>(p_Owner)->loadTexture(heights_v, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, true)->AddLink();
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
			Logger::Out("Couldn't create file for saving!", OutputColor::Red, OutputType::Error);
			return;
		}

		new_file << "w " << size.width << std::endl;
		new_file << "h " << size.height << std::endl;
		new_file << "d " << size.depth << std::endl;
		new_file << "r " << size.resolution << std::endl;
		new_file << "m " << (foliageMask->texture_collection[0] == "" ? "empty_texture" : foliageMask->texture_collection[0]) << std::endl;

		for (std::vector<std::string>::iterator itt = object_texture->texture_collection.begin(); itt != object_texture->texture_collection.end(); ++itt)
		{
			new_file << "t " << (*itt == "" ? "empty_texture" : *itt) << std::endl;
		}

		for (std::vector<std::string>::iterator itt = object_normal->texture_collection.begin(); itt != object_normal->texture_collection.end(); ++itt)
		{
			new_file << "nt " << (*itt == "" ? "empty_texture" : *itt) << std::endl;
		}

		for (std::vector<std::string>::iterator itt = object_displacement->texture_collection.begin(); itt != object_displacement->texture_collection.end(); ++itt)
		{
			new_file << "dt " << (*itt == "" ? "empty_texture" : *itt) << std::endl;
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
			resources->RemoveTexture(object_displacement, logicalDevice, memAllocator);
			VulkanAPI::DestroyImage(heightMap->newImage.allocatedImage);
			VulkanAPI::DestroyImage(foliageMask->newImage.allocatedImage);
			delete object_mesh;

			ready = false;
		}

		object_mesh = new Mesh();
		std::ifstream file(filepath, std::ios::ate | std::ios::binary);

		if (!file)
		{
			Logger::Out("Couldn't open terrain %s", OutputColor::Red, OutputType::Error, filepath);
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
				foliageMask = static_cast<VulkanRenderer*>(p_Owner)->loadTexture({ stream }, VK_IMAGE_VIEW_TYPE_2D_ARRAY)->AddLink();
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

		static_cast<VulkanRenderer*>(p_Owner)->assignTextures({ textures[0], textures[1], textures[2], textures[3] }, this, false);
		static_cast<VulkanRenderer*>(p_Owner)->assignNormals({ normals[0], normals[1], normals[2], normals[3] }, this, false);
		object_displacement = static_cast<VulkanRenderer*>(p_Owner)->loadTexture({ displacements[0], displacements[1], displacements[2], displacements[3] }, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, true)->AddLink();
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
		physComp->UpdateCollisionShape(colShape);
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
		return VulkanAPI::CreatePipelineLayout(logicalDevice, {}, { descriptorSets[1].descriptorSetLayout }, &computeLayout);
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
		pipelineInfo.renderPass = reinterpret_cast<VulkanRenderer*>(p_Owner)->getRenderPass();
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

	void VulkanTerrain::populateDescriptorSets()
	{
		descriptorSets.clear();
		descriptorSets.resize(1 + use_compute);
		descriptorSets[0].bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		VkDescriptorImageInfo texInfo{};
		texInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		texInfo.imageView = object_texture->textureImageView;
		texInfo.sampler = object_texture->textureSampler;
		VkDescriptorImageInfo maskInfo{};
		maskInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		maskInfo.imageView = foliageMask->textureImageView;
		maskInfo.sampler = foliageMask->textureSampler;

		subscribeDescriptor(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<VulkanRenderer*>(p_Owner)->viewProjUBO.BufferInfo);
		subscribeDescriptor(VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maskInfo);
		subscribeDescriptor(VK_SHADER_STAGE_FRAGMENT_BIT, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, texInfo);
		subscribeDescriptor(VK_SHADER_STAGE_FRAGMENT_BIT, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, object_normal->texInfo.descriptor);
		subscribeDescriptor(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, object_displacement->texInfo.descriptor);

		int binding = 3;

		if (use_compute)
		{
			VkDescriptorImageInfo heightInfo{};
			heightInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			heightInfo.imageView = heightMap->textureImageView;
			heightInfo.sampler = heightMap->textureSampler;
			descriptorSets[1].bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
			VkDescriptorBufferInfo inBuffer{};
			inBuffer.buffer = terIn.Buffer;
			inBuffer.offset = 0;
			inBuffer.range = sizeof(TerrainSize);
			VkDescriptorBufferInfo outBuffer{};
			outBuffer.buffer = terOut.Buffer;
			outBuffer.offset = 0;
			outBuffer.range = size.resolution * size.resolution * sizeof(ComputeVertex);
			subscribeDescriptor(VK_SHADER_STAGE_COMPUTE_BIT, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, inBuffer, 1);
			subscribeDescriptor(VK_SHADER_STAGE_COMPUTE_BIT, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, outBuffer, 1);
			subscribeDescriptor(VK_SHADER_STAGE_COMPUTE_BIT, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, heightInfo, 1);
		}

		createDescriptorLayout();
		createDescriptorPool();
		createDescriptorSet();
	}
};