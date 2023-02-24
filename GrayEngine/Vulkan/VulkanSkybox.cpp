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
		Type = "Skybox";

		p_Owner = owner;
		resources = &static_cast<VulkanRenderer*>(owner)->GetResourceManager();
		logicalDevice = device;
		memAllocator = allocator;

		if (object_mesh != nullptr)
		{
			resources->RemoveMesh(object_mesh->mesh_path.c_str(), logicalDevice, memAllocator);
			object_mesh = nullptr;
		}

		auto resource = resources->GetMeshResource("Sky");
		if (resource == nullptr)
		{
			Mesh* default_mesh = new Mesh();

			default_mesh->vertices = {
				{{{ 0.25, 0.25, 0.25, 1.0f },{ 1.f, 1.0f }}},
				{{{ -0.25, 0.25, -0.25, 1.0f },{ 0.f, 0.0f }}},
				{{{ -0.25, 0.25 , 0.25, 1.0f },{ 0.f, 1.f }}},
				{{{ 0.25, 0.25, -0.25, 1.0f },{ 1.f, 0.0f }}},

				{{{ 0.25, -0.25, 0.25, 1.0f },{ 1.f, 1.0f }}},
				{{{ -0.25, -0.25, -0.25, 1.0f },{ 0.f, 0.0f }}},
				{{{ -0.25, -0.25 , 0.25, 1.0f },{ 0.f, 1.f }}},
				{{{ 0.25, -0.25, -0.25, 1.0f },{ 1.f, 0.0f }}}
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

		static_cast<VulkanRenderer*>(p_Owner)->assignTextures({ }, this);
	}

	bool VulkanSkybox::recordCommandBuffer(VkCommandBuffer commandBuffer, VkExtent2D extent, UINT32 mode)
	{
		if (object_mesh->vertexBuffer.initialized == true && filled)
		{
			VulkanDrawable::recordCommandBuffer(commandBuffer, extent, mode);
			return true;
		}

		return false;
	}

	void VulkanSkybox::UpdateTextures(std::array<std::string, 6> sky)
	{
		filled = false;
		//invalidateTexture();
		static_cast<VulkanRenderer*>(p_Owner)->assignTextures(std::vector<std::string>(sky.begin(), sky.end()), this);
		filled = true;
	}

	bool VulkanSkybox::pushConstants(VkCommandBuffer cmd, VkExtent2D extent, UINT32 mode)
	{
		/*orientation relative to the position in a 3D space (?)*/
		ubo.model = glm::translate(glm::mat4(1.f), GetObjectPosition()) * glm::mat4_cast(GetObjectOrientation());
		/*Math for Game Programmers: Understanding Homogeneous Coordinates GDC 2015*/
		ubo.view = glm::translate(glm::mat4_cast(p_Owner->getActiveViewport()->UpdateCameraOrientation(0.2)), -p_Owner->getActiveViewport()->UpdateCameraPosition(0.65)); // [ix iy iz w1( = 0)]-direction [jx jy jz w2( = 0)]-direction [kx ky kz w3( = 0)]-direction [tx ty tz w ( = 1)]-position
		ubo.proj = glm::perspective(glm::radians(60.0f), (float)extent.width / (float)extent.height, near_plane, far_plane); //fov, aspect ratio, near clipping plane, far clipping plane
		ubo.proj[1][1] *= -1; //reverse Y coordinate
		vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UniformBufferObject), &ubo);
		return true;
	}

	void VulkanSkybox::populateDescriptorSets()
	{
		descriptorSets.clear();
		descriptorSets.resize(1);
		descriptorSets[0].bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		VkDescriptorImageInfo info;
		info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		info.imageView = object_texture->textureImageView;
		info.sampler = object_texture->textureSampler;
		subscribeDescriptor(VK_SHADER_STAGE_FRAGMENT_BIT, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, info);

		createDescriptorLayout();
		createDescriptorPool();
		createDescriptorSet();
	}
}