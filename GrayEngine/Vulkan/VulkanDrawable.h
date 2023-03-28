#pragma once
#include "VulkanResourceManager.h"
#include "Engine/Headers/Virtual/Renderer.h"

namespace GrEngine_Vulkan
{
	class VulkanDrawable
	{
	public:
		Texture* object_texture;
		std::string shader_path = "Shaders//default";
		float near_plane = 0.1;
		float far_plane = 1000;

		virtual void initObject(VkDevice device, VmaAllocator allocator, GrEngine::Renderer* owner);
		virtual void destroyObject();
		virtual void updateObject();
		virtual void invalidateTexture();
		virtual bool pushConstants(VkCommandBuffer cmd);
		virtual bool recordCommandBuffer(VkCommandBuffer commandBuffer, UINT32 mode);
		virtual bool draw(VkCommandBuffer commandBuffer);

	protected:
		void subscribeDescriptor(VkShaderStageFlags shaderStage, uint8_t binding, VkDescriptorType descType, VkDescriptorImageInfo imageInfo, int targetLayout = 0);
		void subscribeDescriptor(VkShaderStageFlags shaderStage, uint8_t binding, VkDescriptorType descType, VkDescriptorBufferInfo bufferInfo, int targetLayout = 0);
		virtual void populateDescriptorSets() = 0;

		GrEngine::Renderer* p_Owner;
		std::vector<DescriptorSet> descriptorSets;
		Mesh* object_mesh;

		VkPipelineLayout pipelineLayout;
		VkPipeline graphicsPipeline;

		VertexConstants ubo{};

		virtual bool createDescriptorLayout();
		virtual bool createDescriptorPool();
		virtual bool createPipelineLayout();
		virtual bool createDescriptorSet();
		virtual bool createGraphicsPipeline();
		VulkanResourceManager* resources;
		VkDevice logicalDevice;
		VmaAllocator memAllocator;

		int transparency = 0;
		int double_sided = 0;
	};
}
