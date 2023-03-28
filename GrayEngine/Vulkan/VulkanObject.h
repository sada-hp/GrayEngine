#pragma once
#include "VulkanDrawable.h"
#include "Bullet/BulletAPI.h"

namespace GrEngine_Vulkan
{
	class VulkanObject : public GrEngine::Object, public VulkanDrawable
	{
	public:
		VulkanObject(GrEngine::Entity* owningEntity) : GrEngine::Object(owningEntity)  {};

		virtual void initObject(VkDevice device, VmaAllocator allocator, GrEngine::Renderer* owner) override;
		virtual bool pushConstants(VkCommandBuffer cmd) override;
		void Refresh() override;
		void CalculateNormals() override;

		void updateCollisions() override;
		bool LoadMesh(const char* mesh_path, std::vector<std::string>* out_materials) override;
		bool LoadModel(const char* model_path) override;
		bool LoadModel(const char* mesh_path, std::vector<std::string> textures_vector) override;
		void GeneratePlaneMesh(float width, int subdivisions) override;
		void GenerateBoxMesh(float width, float height, float depth) override;
		static uint32_t selected_id;
		void destroyObject() override;

		void recordSelection(VkCommandBuffer cmd, VkExtent2D extent, UINT32 mode);
	protected:
		void populateDescriptorSets() override;
		bool createGraphicsPipeline() override;

		PickingBufferObject opo;
		btTriangleMesh* colMesh;

	private:
		void updateSelectionPipeline();

		VkPipelineLayout selectionLayout;
		VkPipeline selectionPipeline;

		bool selectable = false;
	};
}

