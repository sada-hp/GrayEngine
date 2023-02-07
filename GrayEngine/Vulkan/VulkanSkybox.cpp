#include <pch.h>
#include "VulkanSkybox.h"
#include "VulkanAPI.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace GrEngine_Vulkan
{
	void VulkanSkybox::initObject(VkDevice device, VmaAllocator allocator, GrEngine::Renderer* owner)
	{
		p_Owner = owner;
		resources = &static_cast<VulkanAPI*>(owner)->GetResourceManager();
		logicalDevice = device;
		memAllocator = allocator;

		UINT id = GetEntityID();
		shader_path = "Shaders//background";
		Type = "Skybox";

		auto resource = resources->GetMeshResource("default");
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

			default_mesh->indices = { 0, 1, 2, 0, 3, 1,
				4, 5, 6, 4, 7, 5,
				0, 2, 6, 0, 4, 6,
				0, 3, 4, 3, 7, 4,
				1, 3, 7, 1, 5, 7,
				1, 2, 6, 1, 5, 6,
			};

			resource = resources->AddMeshResource("default", default_mesh);
			object_mesh = resource->AddLink();

			VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, object_mesh->vertices.data(), sizeof(object_mesh->vertices[0]) * object_mesh->vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &object_mesh->vertexBuffer);
			VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, object_mesh->indices.data(), sizeof(object_mesh->indices[0]) * object_mesh->indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &object_mesh->indexBuffer);
		}
		else
		{
			object_mesh = resource->AddLink();
		}

		descriptorSets.resize(1);
		descriptorSets[0].bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		descriptorSets[0].set.resize(1);

		createDescriptorLayout();
		createDescriptorPool();
		createDescriptorSet();
		createPipelineLayout();
		createGraphicsPipeline();
	}

	bool VulkanSkybox::recordCommandBuffer(VkCommandBuffer commandBuffer, VkExtent2D extent, UINT32 mode)
	{
		if (object_mesh->vertexBuffer.initialized == true && filled)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &object_mesh->vertexBuffer.Buffer, offsets);
			vkCmdBindIndexBuffer(commandBuffer, object_mesh->indexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT8_EXT);

			//UpdateObjectPosition();
			//UpdateObjectOrientation();
			pushConstants(commandBuffer, extent, mode);

			for (std::vector<DescriptorSet>::iterator itt = descriptorSets.begin(); itt != descriptorSets.end(); ++itt)
			{
				for (std::vector<VkDescriptorSet>::iterator it = (*itt).set.begin(); it != (*itt).set.end(); ++it)
				{
					if ((*itt).bindPoint == VK_PIPELINE_BIND_POINT_GRAPHICS)
						vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &(*it), 0, NULL);
				}
			}

			//vkCmdDraw(commandBuffer, static_cast<uint32_t>(object_mesh.vertices.size()), 1, 0, 0);
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(object_mesh->indices.size()), 1, 0, 0, 0);
			return true;
		}

		return false;
	}

	void VulkanSkybox::UpdateTextures(std::array<std::string, 6> sky)
	{
		filled = false;
		invalidateTexture();
		static_cast<VulkanAPI*>(p_Owner)->assignTextures(std::vector<std::string>(sky.begin(), sky.end()), this);
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
}