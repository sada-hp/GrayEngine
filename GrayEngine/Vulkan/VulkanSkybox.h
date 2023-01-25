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
		VulkanSkybox()
		{

		}

		VulkanSkybox(UINT id) : GrEngine::Skybox(id)
		{

		}

		void UpdateTextures(std::array<std::string, 6> sky) override;
		bool recordCommandBuffer(VkCommandBuffer commandBuffer, VkExtent2D extent, UINT32 mode) override;
		void initObject(VkDevice device, VmaAllocator allocator, GrEngine::Renderer* owner) override;
		bool pushConstants(VkCommandBuffer cmd, VkExtent2D extent, UINT32 mode) override;
	private:
		bool filled = false;
	};
};
