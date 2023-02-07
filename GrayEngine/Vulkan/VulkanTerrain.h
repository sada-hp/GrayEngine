#pragma once
#include "VulkanDrawable.h"
#include "Entities/Terrain.h"

namespace GrEngine_Vulkan
{
	class VulkanTerrain : public GrEngine::Terrain, public VulkanDrawable
	{
		struct ComputeVertex {
			glm::vec4 pos;
			glm::vec4 uv;
		};

		struct ComputeSize
		{
			int resolution;
			int width;
			int depth;
			int height;
		};

		ComputeSize size = { 512, 2000, 2000, 400 };

	public:
		VulkanTerrain() {};
		VulkanTerrain(std::vector<Vertex> vertices, std::vector<uint8_t> indices, std::string resource_name) {};
		VulkanTerrain(UINT id) : Terrain(id) {};

		virtual void initObject(VkDevice device, VmaAllocator allocator, GrEngine::Renderer* owner) override;
		bool recordCommandBuffer(VkCommandBuffer commandBuffer, VkExtent2D extent, UINT32 mode) override;
		virtual bool pushConstants(VkCommandBuffer cmd, VkExtent2D extent, UINT32 mode) override;
		void destroyObject() override;
		void calculateCollisions() override;
		void GenerateTerrain(uint16_t resolution) override;

	protected:
		bool createPipelineLayout() override;
		bool createGraphicsPipeline() override;
		bool createDescriptorLayout() override;
		bool createDescriptorPool() override;
		bool createDescriptorSet() override;
		btTriangleMesh* colMesh;


		void* terI;
		ShaderBuffer terOut;
		ShaderBuffer terIn;
		VkPipeline computePipeline;
		VkPipelineLayout computeLayout;
		VkCommandBuffer computeCmd;
		VkCommandPool computePool;
		VkQueue computeQueue;
		VkFence computeFence;
	};
}



