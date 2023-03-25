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

		bool ready = false;

	public:
		VulkanTerrain() {};
		VulkanTerrain(std::vector<Vertex> vertices, std::vector<uint8_t> indices, std::string resource_name) {};
		VulkanTerrain(UINT id) : Terrain(id) {};

		virtual void initObject(VkDevice device, VmaAllocator allocator, GrEngine::Renderer* owner) override;
		virtual bool pushConstants(VkCommandBuffer cmd) override;
		void destroyObject() override;
		void calculateCollisions() override;
		void GenerateTerrain(int resolution, int width, int height, int depth, std::array<std::string, 6> images) override;
		void UpdateFoliageMask(void* pixels) override;
		void UpdateFoliageMask(void* pixels, uint32_t width, uint32_t height, uint32_t offset_x, uint32_t offset_y) override;
		void OffsetVertices(std::map<UINT, float> offsets) override;
		void UpdateVertices(std::map<UINT, float> offsets) override;
		void UpdateCollision() override;
		glm::vec4& GetVertexPosition(UINT at) override;
		void SaveTerrain(const char* filepath) override;
		bool LoadTerrain(const char* filepath) override;

	protected:
		void populateDescriptorSets();

		bool createPipelineLayout() override;
		bool createGraphicsPipeline() override;
		bool createComputeLayout();
		bool createComputePipeline();
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

		btBvhTriangleMeshShape* colShape;
		bool use_compute = false;
		float maxAABB = 0.f;
		float minAABB = 0.f;
	};
}



