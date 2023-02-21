#pragma once
#include "VulkanDrawable.h"
#include "Bullet/BulletAPI.h"

namespace GrEngine_Vulkan
{
	class VulkanObject : public GrEngine::DrawableObject, public VulkanDrawable
	{
	public:
		VulkanObject() {};
		VulkanObject(std::vector<Vertex> vertices, std::vector<uint8_t> indices, std::string resource_name) {};
		VulkanObject(UINT id) : DrawableObject(id) {};

		virtual void initObject(VkDevice device, VmaAllocator allocator, GrEngine::Renderer* owner) override;
		virtual bool pushConstants(VkCommandBuffer cmd, VkExtent2D extent, UINT32 mode) override;
		void Refresh() override;

		void updateCollisions() override;
		bool LoadMesh(const char* mesh_path, std::vector<std::string>* out_materials) override;
		bool LoadModel(const char* model_path) override;
		bool LoadModel(const char* mesh_path, std::vector<std::string> textures_vector) override;
		void GeneratePlaneMesh(float width, int subdivisions) override;
		void GenerateBoxMesh(float width, float height, float depth) override;
		inline glm::uvec3 getColorID() { return colorID; };
		static uint32_t selected_id;
		//bool recordCommandBuffer(VkCommandBuffer commandBuffer, VkExtent2D extent, UINT32 mode) override;
		

	protected:
		void populateDescriptorSets() override;

		bool createGraphicsPipeline() override;

		glm::uvec3 colorID = { 0, 0, 0 };
		PickingBufferObject opo;
		btTriangleMesh* colMesh;
		
	};
}

