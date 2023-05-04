#pragma once
#include "VulkanResourceManager.h"
#include "Entities/Properties/Light.h"
#include "VulkanAPI.h"

namespace GrEngine_Vulkan
{
	class VulkanSpotlight : public GrEngine::LightObject
	{
	public:
		struct ShadowProjection
		{
			glm::vec4 spec;
			glm::vec4 color;
			glm::mat4 viewproj;
			glm::mat4 model;
		} lightPerspective;


		VulkanSpotlight(GrEngine::Entity* owningEntity) : GrEngine::LightObject(owningEntity) {};

		void initLight(VkDevice device, VmaAllocator allocator);
		void destroyLight();
		ShadowProjection& getLightUBO();
		void UpdateLight() override;

	protected:
		VkDevice logicalDevice;
		VmaAllocator memAllocator;
	};
};

