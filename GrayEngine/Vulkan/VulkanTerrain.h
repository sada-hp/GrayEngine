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

		ComputeSize size;
		bool ready = false;

	public:
		VulkanTerrain() {};
		VulkanTerrain(std::vector<Vertex> vertices, std::vector<uint8_t> indices, std::string resource_name) {};
		VulkanTerrain(UINT id) : Terrain(id) {};

		virtual void initObject(VkDevice device, VmaAllocator allocator, GrEngine::Renderer* owner) override;
		virtual bool pushConstants(VkCommandBuffer cmd, VkExtent2D extent, UINT32 mode) override;
		void destroyObject() override;
		void calculateCollisions() override;
		void GenerateTerrain(int resolution, int width, int height, int depth, std::array<std::string, 6> images) override;
		void UpdateFoliageMask(void* pixels) override;

	protected:
		void populateDescriptorSets();

		bool createPipelineLayout() override;
		bool createGraphicsPipeline() override;
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
		Texture* heightMap;
		Texture* foliageMask;
	};
}



