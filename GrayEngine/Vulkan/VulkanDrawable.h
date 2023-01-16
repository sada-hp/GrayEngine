#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vk_mem_alloc.h>
#include "Engine/Headers/Virtual/Renderer.h"
#include "VulkanResourceManager.h"

namespace GrEngine_Vulkan
{
	class VulkanDrawable
	{
	public:
		
		Texture* object_texture;
		const char* shader_path = "Shaders//default";
		float near_plane = 0.1;
		float far_plane = 1000;

		virtual void initObject(VkDevice device, VmaAllocator allocator, GrEngine::Renderer* owner);
		virtual void destroyObject();
		virtual void updateObject();
		virtual void invalidateTexture();
		virtual bool pushConstants(VkCommandBuffer cmd, VkExtent2D extent, UINT32 mode);
		virtual bool recordCommandBuffer(VkCommandBuffer commandBuffer, VkExtent2D extent, UINT32 mode);

	protected:
		GrEngine::Renderer* p_Owner;
		VkDescriptorSetLayout descriptorSetLayout;
		std::vector<VkDescriptorSet> descriptorSets;
		Mesh* object_mesh;

		VkDescriptorPool descriptorPool;
		VkPipelineLayout pipelineLayout;
		VkPipeline graphicsPipeline;

		UniformBufferObject ubo{};

		bool createDescriptorLayout();
		bool createDescriptorPool();
		bool createPipelineLayout();
		bool createDescriptorSet();
		bool createGraphicsPipeline();
		VulkanResourceManager* resources;
		VkDevice logicalDevice;
		VmaAllocator memAllocator;
	};
}
