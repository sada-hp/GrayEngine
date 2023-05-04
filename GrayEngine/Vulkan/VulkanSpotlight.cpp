#include "pch.h"
#include "VulkanSpotlight.h"

namespace GrEngine_Vulkan
{
	void VulkanSpotlight::initLight(VkDevice device, VmaAllocator allocator)
	{
		logicalDevice = device;
		memAllocator = allocator;
		type = LightType::Spot;
		UpdateLight();
	}

	void VulkanSpotlight::destroyLight()
	{

	}

	void VulkanSpotlight::UpdateLight()
	{
		max_distance = ownerEntity->GetPropertyValue(PropertyType::MaximumDistance, 15.f);
		max_distance = max_distance < 0 ? 15.f : max_distance;
		brightness = ownerEntity->GetPropertyValue(PropertyType::Brightness, 1.f);
	}

	VulkanSpotlight::ShadowProjection& VulkanSpotlight::getLightUBO()
	{
		lightPerspective.model = ownerEntity->GetObjectTransformation();
		glm::mat4 view = glm::lookAt(ownerEntity->GetObjectPosition(), (glm::vec3(lightPerspective.model[2][0], lightPerspective.model[2][1], lightPerspective.model[2][2]) * 10.f + ownerEntity->GetObjectPosition()), glm::vec3(0, 1, 0));
		glm::mat4 proj = glm::perspective(glm::radians(80.f), 1.f, 0.01f, max_distance);
		float f = (15.f * 2.0) / (SHADOW_MAP_DIM);
		view[3][0] = glm::round(view[3][0] / f) * f;
		view[3][1] = glm::round(view[3][1] / f) * f;
		view[3][2] = glm::round(view[3][2] / f) * f;
		lightPerspective.viewproj = proj * view;
		lightPerspective.color = ownerEntity->GetPropertyValue(PropertyType::Color, glm::vec4(1.f));
		lightPerspective.spec.x = type;
		lightPerspective.spec.y = max_distance;
		lightPerspective.spec.z = brightness;
		lightPerspective.spec.w = 80.f;
		//lightPerspective.proj *= -1;

		return lightPerspective;
	}
}