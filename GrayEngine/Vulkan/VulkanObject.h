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

		bool LoadMesh(const char* mesh_path) override;
		bool LoadModel(const char* gmf_path, const char* mesh_path, std::vector<std::string> textures_vector, std::vector<std::string> normals_vector) override;
		void GeneratePlaneMesh(float width, int subdivisions) override;
		void GenerateBoxMesh(float width, float height, float depth) override;
		void GenerateSphereMesh(double radius, int rings, int slices) override;
		void updateObject() override;
		static uint32_t selected_id;
		void destroyObject() override;

		void recordSelection(VkCommandBuffer cmd, VkExtent2D extent, UINT32 mode);
		void recordShadowPass(VkCommandBuffer cmd, int instances);
	protected:
		void populateDescriptorSets() override;
		bool createGraphicsPipeline() override;
		void createDescriptors() override;

		FragmentBuffer opo;

	private:
		void updateSelectionPipeline();
		void updateShadowPipeline();

		VkPipelineLayout selectionLayout;
		VkPipeline selectionPipeline;

		VkPipelineLayout shadowLayout;
		VkPipeline shadowPipeline;

		bool selectable = false;
		float alpha_threshold = 0.5f;
	};
}

