#include <pch.h>
#include "VulkanObject.h"
#include "VulkanAPI.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace GrEngine_Vulkan
{
	PickingBufferObject VulkanObject::opo{};

	void VulkanObject::initObject(VkDevice device, VmaAllocator allocator, GrEngine::Renderer* owner)
	{
		p_Owner = owner;
		UINT id = GetEntityID();
		colorID = { id / 1000000 % 1000, id / 1000 % 1000, id % 1000 };

		object_mesh.vertices = {
			{{{ 0.25, 0.25, 0.25, 1.0f },{ 1.f, 1.0f }, colorID}},
			{{{ -0.25, 0.25, -0.25, 1.0f },{ 0.f, 0.0f }, colorID}},
			{{{ -0.25, 0.25 , 0.25, 1.0f },{ 0.f, 1.f }, colorID}},
			{{{ 0.25, 0.25, -0.25, 1.0f },{ 1.f, 0.0f }, colorID}},

			{{{ 0.25, -0.25, 0.25, 1.0f },{ 1.f, 1.0f }, colorID}},
			{{{ -0.25, -0.25, -0.25, 1.0f },{ 0.f, 0.0f }, colorID}},
			{{{ -0.25, -0.25 , 0.25, 1.0f },{ 0.f, 1.f }, colorID}},
			{{{ 0.25, -0.25, -0.25, 1.0f },{ 1.f, 0.0f }, colorID}}
		};

		object_mesh.indices = { 0, 1, 2, 0, 3, 1,
			4, 5, 6, 4, 7, 5,
			0, 2, 6, 0, 4, 6,
			0, 3, 4, 3, 7, 4,
			1, 3, 7, 1, 5, 7,
			1, 2, 6, 1, 5, 6,
		};

		if (object_mesh.vertices.size() > 0)
		{
			VulkanAPI::m_createVkBuffer(device, allocator, object_mesh.vertices.data(), sizeof(object_mesh.vertices[0]) * object_mesh.vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &vertexBuffer);
			VulkanAPI::m_createVkBuffer(device, allocator, object_mesh.indices.data(), sizeof(object_mesh.indices[0]) * object_mesh.indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &indexBuffer);
		}

		createDescriptorLayout(device);
		createDescriptorPool(device);
		createDescriptorSet(device, allocator);
		createPipelineLayout(device);
		createGraphicsPipeline(device);
	}

	bool VulkanObject::pushConstants(VkDevice devicce, VkCommandBuffer cmd, VkExtent2D extent, UINT32 mode)
	{
		/*orientation relative to the position in a 3D space (?)*/
		ubo.model = glm::translate(glm::mat4(1.f), GetObjectPosition()) * glm::mat4_cast(GetObjectOrientation());
		/*Math for Game Programmers: Understanding Homogeneous Coordinates GDC 2015*/
		ubo.view = glm::translate(glm::mat4_cast(p_Owner->getActiveViewport()->UpdateCameraOrientation(0.2)), -p_Owner->getActiveViewport()->UpdateCameraPosition(0.65)); // [ix iy iz w1( = 0)]-direction [jx jy jz w2( = 0)]-direction [kx ky kz w3( = 0)]-direction [tx ty tz w ( = 1)]-position
		ubo.proj = glm::perspective(glm::radians(60.0f), (float)extent.width / (float)extent.height, near_plane, far_plane); //fov, aspect ratio, near clipping plane, far clipping plane
		ubo.proj[1][1] *= -1; //reverse Y coordinate
		ubo.scale = GetPropertyValue("Scale", glm::vec3(1.f));
		vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UniformBufferObject), &ubo);

		opo.draw_mode = mode;
		opo.color_mask = GetPropertyValue("Color", glm::vec4(1.f));
		vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(UniformBufferObject), sizeof(PickingBufferObject), &opo);
		return true;
	}

	void VulkanObject::updateCollisions()
	{
		delete colShape;
		if (colMesh != nullptr)
		{
			delete colMesh;
		}

		colMesh = new btTriangleMesh();
		for (int i = 0; i < object_mesh.vertices.size(); i++)
		{
			colMesh->findOrAddVertex(btVector3(object_mesh.vertices[i].pos.x, object_mesh.vertices[i].pos.y, object_mesh.vertices[i].pos.z), false);
		}
		for (int i = 0; i < object_mesh.indices.size(); i += 3)
		{
			colMesh->addTriangleIndices(object_mesh.indices[i], object_mesh.indices[i + 1], object_mesh.indices[i + 2]);
		}

		colShape = new btBvhTriangleMeshShape(colMesh, false);
		recalculatePhysics();
	}

	bool VulkanObject::LoadModel(const char* model_path)
	{
		auto start = std::chrono::steady_clock::now();

		std::string mesh_path = "";
		std::vector<std::string> textures_vector;
		if (!GrEngine::Globals::readGMF(model_path, &mesh_path, &textures_vector)) return false;

		LoadModel(mesh_path.c_str(), textures_vector);

		auto end = std::chrono::steady_clock::now();
		auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
		Logger::Out("Model %s loaded in %d ms", OutputColor::Gray, OutputType::Log, model_path, (int)time);

		return true;
	}

	bool VulkanObject::LoadModel(const char* mesh_path, std::vector<std::string> textures_vector)
	{
		VulkanObject* ref_obj = this;

		VulkanAPI* inst = static_cast<VulkanAPI*>(p_Owner);
		std::vector<std::string> materials_collection;
		std::vector<std::string>* out_materials_collection = &materials_collection;
		std::map<int, std::future<void>> processes_map;
		std::map<std::string, std::vector<int>> materials_map;

		processes_map[processes_map.size()] = std::async(std::launch::async, [textures_vector, ref_obj, inst]()
			{
				inst->assignTextures(textures_vector, ref_obj);
			});

		processes_map[processes_map.size()] = std::async(std::launch::async, [mesh_path, textures_vector, ref_obj, out_materials_collection, inst]()
			{
				ref_obj->LoadMesh(mesh_path, textures_vector.size() != 0, out_materials_collection);
			});

		for (int ind = 0; ind < processes_map.size(); ind++)
		{
			if (processes_map[ind].valid())
			{
				processes_map[ind].wait();
			}
		}

		updateObject(inst->logicalDevice, inst->getMemAllocator());

		material_names.clear();
		texture_names.clear();

		for (int ind = 0; ind < materials_collection.size(); ind++)
		{
			material_names.push_back(materials_collection[ind]);
		}

		return true;
	}

	bool VulkanObject::LoadMesh(const char* mesh_path, bool useTexturing, std::vector<std::string>* out_materials)
	{
		std::unordered_map<Vertex, uint32_t> uniqueVertices{};
		Assimp::Importer importer;

		auto model = importer.ReadFile(mesh_path, 0);

		if (model == NULL)
		{
			Logger::Out("Could not load the mesh %c%s%c!", OutputColor::Red, OutputType::Error, '"', mesh_path, '"');
			return false;
		}

		object_mesh.indices.clear();
		object_mesh.vertices.clear();
		object_mesh.mesh_path = mesh_path;

		if (colMesh != nullptr)
		{
			delete colMesh;
			colMesh = nullptr;
		}

		colMesh = new btTriangleMesh();

		float highest_pointx = 0.f;
		float highest_pointy = 0.f;
		float highest_pointz = 0.f;

		for (int mesh_ind = 0; mesh_ind < model->mNumMeshes; mesh_ind++)
		{
			auto num_vert = model->mMeshes[mesh_ind]->mNumVertices;
			auto cur_mesh = model->mMeshes[mesh_ind];
			auto name3 = model->mMeshes[mesh_ind]->mName;
			auto uv_ind = mesh_ind;

			for (int vert_ind = 0; vert_ind < num_vert; vert_ind++)
			{
				auto coord = model->mMeshes[mesh_ind]->mTextureCoords[0];

				GrEngine_Vulkan::Vertex vertex{ {
				{ cur_mesh->mVertices[vert_ind].x, cur_mesh->mVertices[vert_ind].y, cur_mesh->mVertices[vert_ind].z, 1.0f },
				{ coord[vert_ind].x, 1.0f - coord[vert_ind].y },
				getColorID(),
				(uint32_t)uv_ind,
				useTexturing} };


				if (uniqueVertices.count(vertex) == 0)
				{
					uniqueVertices[vertex] = static_cast<uint32_t>(object_mesh.vertices.size());
					object_mesh.vertices.push_back(vertex);
					colMesh->findOrAddVertex(btVector3(vertex.pos.x, vertex.pos.y, vertex.pos.z), false);
				}

				if (highest_pointx < cur_mesh->mVertices[vert_ind].x)
					highest_pointx = cur_mesh->mVertices[vert_ind].x;
				if (highest_pointy < cur_mesh->mVertices[vert_ind].y)
					highest_pointy = cur_mesh->mVertices[vert_ind].y;
				if (highest_pointz < cur_mesh->mVertices[vert_ind].z)
					highest_pointz = cur_mesh->mVertices[vert_ind].z;

				int index = uniqueVertices[vertex];
				object_mesh.indices.push_back(index);
			}

			aiString name;
			aiGetMaterialString(model->mMaterials[model->mMeshes[mesh_ind]->mMaterialIndex], AI_MATKEY_NAME, &name);

			if (out_materials)
			{
				out_materials->push_back(name.C_Str());
			}
		}
		SetObjectBounds(glm::vec3{ highest_pointx, highest_pointy, highest_pointz });
		for (int i = 0; i < object_mesh.indices.size(); i += 3)
		{
			colMesh->addTriangleIndices(object_mesh.indices[i], object_mesh.indices[i + 1], object_mesh.indices[i + 2]);
		}

		delete colShape;
		colShape = new btBvhTriangleMeshShape(colMesh, false);
		recalculatePhysics(true);
	}
};