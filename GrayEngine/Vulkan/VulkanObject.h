#pragma once
#include "VulkanDrawable.h"

namespace GrEngine_Vulkan
{
	class VulkanObject : public GrEngine::DrawableObject, public VulkanDrawable
	{
	public:
		VulkanObject() {};
		VulkanObject(UINT id) : DrawableObject(id) {};

		void initObject(VkDevice device, VmaAllocator allocator, GrEngine::Renderer* owner) override;
		virtual bool pushConstants(VkDevice devicce, VkCommandBuffer cmd, VkExtent2D extent, UINT32 mode) override;

		void updateCollisions() override;
		bool LoadMesh(const char* mesh_path, bool useTexturing, std::vector<std::string>* out_materials) override;
		bool LoadModel(const char* model_path) override;
		bool LoadModel(const char* mesh_path, std::vector<std::string> textures_vector) override;
		inline glm::uvec3 getColorID() { return colorID; };
		static PickingBufferObject opo;

	private:
		glm::uvec3 colorID = { 0, 0, 0 };
	};
}

