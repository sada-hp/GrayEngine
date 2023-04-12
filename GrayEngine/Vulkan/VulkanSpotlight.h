#pragma once
#include "VulkanResourceManager.h"
#include "Entities/Properties/Spotlight.h"
#include "VulkanAPI.h"

namespace GrEngine_Vulkan
{
	class VulkanSpotlight : public GrEngine::SpotlightObject
	{
	public:
		struct ShadowProjection
		{
			glm::vec4 spec;
			glm::vec4 color;
			glm::mat4 viewproj;
			glm::mat4 model;
		} lightPerspective;


		VulkanSpotlight(GrEngine::Entity* owningEntity) : GrEngine::SpotlightObject(owningEntity) {};

		void initLight(VkDevice device, VmaAllocator allocator);
		void destroyLight();
		ShadowProjection& getLightUBO();

	protected:
		VkDevice logicalDevice;
		VmaAllocator memAllocator;
	};
};

