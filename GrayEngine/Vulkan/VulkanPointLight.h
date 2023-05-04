#pragma once
#include "VulkanResourceManager.h"
#include "Entities/Properties/Light.h"

namespace GrEngine_Vulkan
{
	class VulkanPointLight : public GrEngine::LightObject
	{
		struct UniformBuffer
		{
			glm::vec4 spec;
			glm::vec4 color;
			glm::mat4 model;
		} lightPerspective;

	public:
		VulkanPointLight(GrEngine::Entity* owningEntity) : GrEngine::LightObject(owningEntity) {};

		void initLight(VkDevice device, VmaAllocator allocator);
		void destroyLight();
		UniformBuffer& getLightUBO();
		void UpdateLight() override;

	protected:
		VkDevice logicalDevice;
		VmaAllocator memAllocator;
	};
}
