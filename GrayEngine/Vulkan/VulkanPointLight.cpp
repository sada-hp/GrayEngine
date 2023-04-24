#include "pch.h"
#include "VulkanPointLight.h"

namespace GrEngine_Vulkan
{
	void VulkanPointLight::initLight(VkDevice device, VmaAllocator allocator)
	{
		logicalDevice = device;
		memAllocator = allocator;
		type = LightType::Point;
	}

	void VulkanPointLight::destroyLight()
	{

	}

	VulkanPointLight::UniformBuffer& VulkanPointLight::getLightUBO()
	{
		lightPerspective.model = ownerEntity->GetObjectTransformation();
		lightPerspective.color = ownerEntity->GetPropertyValue(PropertyType::Color, glm::vec4(1.f));
		lightPerspective.spec.x = LightType::Point;
		lightPerspective.spec.y = 15.f;
		//lightPerspective.proj *= -1;

		return lightPerspective;
	}
}