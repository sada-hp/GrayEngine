#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vk_mem_alloc.h>
#include "Engine/Headers/Virtual/Renderer.h"
#include "Engine/Headers/Entities/Skybox.h"
#include "VulkanDrawable.h"

namespace GrEngine_Vulkan
{
	class VulkanSkybox : public VulkanDrawable, public GrEngine::Skybox
	{
	public:
		void UpdateTextures(std::array<std::string, 6> sky) override;
		bool recordCommandBuffer(VkDevice device, VkCommandBuffer commandBuffer, VkExtent2D extent, UINT32 mode) override;
		void initObject(VkDevice device, VmaAllocator allocator, GrEngine::Renderer* owner) override;
		bool pushConstants(VkDevice devicce, VkCommandBuffer cmd, VkExtent2D extent, UINT32 mode) override;
	private:
		bool filled = false;
	};
};

