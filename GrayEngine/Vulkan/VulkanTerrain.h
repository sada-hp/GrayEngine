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

	public:
		VulkanTerrain() {};
		VulkanTerrain(std::vector<Vertex> vertices, std::vector<uint8_t> indices, std::string resource_name) {};
		VulkanTerrain(UINT id) : Terrain(id) {};

		virtual void initObject(VkDevice device, VmaAllocator allocator, GrEngine::Renderer* owner) override;
		virtual bool pushConstants(VkCommandBuffer cmd) override;
		void updateObject() override;
		void destroyObject() override;
		void calculateCollisions() override;
		void GenerateTerrain(int resolution, int width, int height, int depth, std::array<std::string, 6> images, std::array<std::string, 4> normals, std::array<std::string, 4> displacements) override;
		void UpdateTextures(std::array<std::string, 5> images, std::array<std::string, 4> normals, std::array<std::string, 4> displacement) override;
		void UpdateFoliageMask(void* pixels) override;
		void UpdateFoliageMask(void* pixels, uint32_t width, uint32_t height, uint32_t offset_x, uint32_t offset_y) override;
		void OffsetVertices(std::map<UINT, float> offsets) override;
		void UpdateVertices(std::map<UINT, float> offsets) override;
		void UpdateCollision() override;
		glm::vec4& GetVertexPosition(UINT at) override;
		void SaveTerrain(const char* filepath) override;
		bool LoadTerrain(const char* filepath) override;
		const std::string GetBlendMask() override;
		const std::array<std::string, 4> GetColorTextures();
		const std::array<std::string, 4> GetNormalTextures();
		const std::array<std::string, 4> GetDisplacementTextures();

		void recordShadowPass(VkCommandBuffer cmd, int instances);

	protected:
		void populateDescriptorSets();
		void createDescriptors() override;

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
		std::vector<Texture*> object_displacement;

		btBvhTriangleMeshShape* colShape;
	private:
		void updateShadowPipeline();

		VkPipelineLayout shadowLayout;
		VkPipeline shadowPipeline;

		bool use_compute = false;
		float maxAABB = 0.f;
		float minAABB = 0.f;
		bool ready = false;
		bool was_updated = false;
	};
}



