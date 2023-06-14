#include <pch.h>
#include "VulkanSkybox.h"
#include "VulkanRenderer.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace GrEngine_Vulkan
{
	void VulkanSkybox::initObject(VkDevice device, VmaAllocator allocator, GrEngine::Renderer* owner)
	{
		UINT id = GetEntityID();
		shader_path = "Shaders//background";
		AddNewProperty("CastShadow");
		ParsePropertyValue("CastShadow", "0");
		//Type = GrEngine::EntityType::SkyboxEntity;

		p_Owner = owner;
		resources = &static_cast<VulkanRenderer*>(owner)->GetResourceManager();
		logicalDevice = device;
		memAllocator = allocator;

		if (object_mesh != nullptr)
		{
			resources->RemoveMesh(object_mesh, logicalDevice, memAllocator);
			object_mesh = nullptr;
		}

		auto resource = resources->GetMeshResource("Sky");
		if (resource == nullptr)
		{
			Mesh* default_mesh = new Mesh();

			default_mesh->vertices = {
				{{{ 0.25, 0.25, 0.25, 1.0f },{  1.0f,  1.0f,  1.0f },{ 1.f, 1.0f }}},
				{{{ -0.25, 0.25, -0.25, 1.0f },{  1.0f,  1.0f,  1.0f },{ 0.f, 0.0f }}},
				{{{ -0.25, 0.25 , 0.25, 1.0f },{  1.0f,  1.0f,  1.0f },{ 0.f, 1.f }}},
				{{{ 0.25, 0.25, -0.25, 1.0f },{  1.0f,  1.0f,  1.0f },{ 1.f, 0.0f }}},

				{{{ 0.25, -0.25, 0.25, 1.0f },{  1.0f,  1.0f,  1.0f },{ 1.f, 1.0f }}},
				{{{ -0.25, -0.25, -0.25, 1.0f },{  1.0f,  1.0f,  1.0f },{ 0.f, 0.0f }}},
				{{{ -0.25, -0.25 , 0.25, 1.0f },{  1.0f,  1.0f,  1.0f },{ 0.f, 1.f }}},
				{{{ 0.25, -0.25, -0.25, 1.0f },{  1.0f,  1.0f,  1.0f },{ 1.f, 0.0f }}}
			};

			default_mesh->indices = { 2, 1, 0, 1, 3, 0,
				4, 5, 6, 4, 7, 5,
				6, 2, 0, 0, 4, 6,
				0, 3, 4, 3, 7, 4,
				7, 3, 1, 1, 5, 7,
				1, 2, 6, 6, 5, 1,
			};

			resource = resources->AddMeshResource("Sky", default_mesh);
			object_mesh = resource->AddLink();

			VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, object_mesh->vertices.data(), sizeof(object_mesh->vertices[0]) * object_mesh->vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &object_mesh->vertexBuffer);
			VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, object_mesh->indices.data(), sizeof(object_mesh->indices[0]) * object_mesh->indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &object_mesh->indexBuffer);
		}
		else
		{
			object_mesh = resource->AddLink();
		}

		AssignTextures({ "", "" , "" , "" , "" , "" });
		initialized = true;
	}

	bool VulkanSkybox::recordCommandBuffer(VkCommandBuffer commandBuffer, UINT32 mode)
	{
		if (initialized)
		{
			VulkanDrawable::recordCommandBuffer(commandBuffer, mode);
			return true;
		}

		return false;
	}

	bool VulkanSkybox::AssignTextures(std::array<std::string, 6> sky)
	{
		VulkanRenderer* render_context = static_cast<VulkanRenderer*>(p_Owner);

		for (int i = 0; i < object_texture.size(); i++)
		{
			resources->RemoveTexture(object_texture.at(i), logicalDevice, memAllocator);
			object_texture[i] = nullptr;
		}
		object_texture.resize(0);

		object_texture.push_back(render_context->loadTexture(std::vector<std::string>(sky.begin(), sky.end()), GrEngine::TextureType::Color, VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB, true)->AddLink());

		updateObject();

		return true;
	}

	bool VulkanSkybox::pushConstants(VkCommandBuffer cmd)
	{
		/*orientation relative to the position in a 3D space (?)*/
		ubo.model = glm::translate(glm::mat4(1.f), GetObjectPosition()) * glm::mat4_cast(GetObjectOrientation());
		/*Math for Game Programmers: Understanding Homogeneous Coordinates GDC 2015*/
		vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VertexConstants), &ubo);

		opo.colors = GetPropertyValue(PropertyType::Color, glm::vec4(1));
		vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(VertexConstants), sizeof(FragmentBuffer), &opo);
		return true;
	}

	void VulkanSkybox::createDescriptors()
	{
		descriptorSets.clear();
		descriptorSets.resize(1);
		descriptorSets[0].bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		populateDescriptorSets();

		createDescriptorLayout();
		createDescriptorPool();
		createDescriptorSet();
	}

	void VulkanSkybox::populateDescriptorSets()
	{
		VkDescriptorImageInfo info;
		info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		info.imageView = object_texture[0]->textureImageView;
		info.sampler = object_texture[0]->textureSampler;
		subscribeDescriptor(VK_SHADER_STAGE_VERTEX_BIT, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<VulkanRenderer*>(p_Owner)->viewProjUBO.BufferInfo);
		subscribeDescriptor(VK_SHADER_STAGE_FRAGMENT_BIT, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, info);
	}
}