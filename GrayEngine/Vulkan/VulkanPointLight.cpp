#include "pch.h"
#include "VulkanPointLight.h"

namespace GrEngine_Vulkan
{
	void VulkanPointLight::initLight(VkDevice device, VmaAllocator allocator)
	{
		logicalDevice = device;
		memAllocator = allocator;
		type = LightType::Point;
		UpdateLight();
	}

	void VulkanPointLight::destroyLight()
	{

	}

	void VulkanPointLight::UpdateLight()
	{
		max_distance = ownerEntity->GetPropertyValue(PropertyType::MaximumDistance, 15.f);
		max_distance = max_distance < 0 ? 15.f : max_distance;
		brightness = ownerEntity->GetPropertyValue(PropertyType::Brightness, 1.f);
	}

	VulkanPointLight::UniformBuffer& VulkanPointLight::getLightUBO()
	{
		lightPerspective.model = ownerEntity->GetObjectTransformation();
		lightPerspective.color = ownerEntity->GetPropertyValue(PropertyType::Color, glm::vec4(1.f));
		lightPerspective.spec.x = LightType::Point;
		lightPerspective.spec.y = max_distance;
		lightPerspective.spec.z = brightness;
		//lightPerspective.proj *= -1;

		return lightPerspective;
	}
}