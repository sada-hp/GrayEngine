#pragma once
#include "VulkanResourceManager.h"
#include "Entities/Properties/Light.h"

namespace GrEngine_Vulkan
{
	class VulkanOmniLight : public GrEngine::LightObject
	{
	public:
		struct OmniProjection
		{
			glm::vec4 spec;
			glm::vec4 color;
			glm::mat4 viewproj;
			glm::mat4 model;
		};

		VulkanOmniLight(GrEngine::Entity* owningEntity) : GrEngine::LightObject(owningEntity) {};

		void initLight(VkDevice device, VmaAllocator allocator);
		void destroyLight();
		std::array<OmniProjection, 6>& getLightUBO();
		void UpdateLight() override;

	protected:
		VkDevice logicalDevice;
		VmaAllocator memAllocator;
		std::array<OmniProjection, 6> lightPerspective;
	};
}

