#include "pch.h"
#include "VulkanSpotlight.h"

namespace GrEngine_Vulkan
{
	void VulkanSpotlight::initLight(VkDevice device, VmaAllocator allocator)
	{
		logicalDevice = device;
		memAllocator = allocator;
		type = LightType::Spot;
	}

	void VulkanSpotlight::destroyLight()
	{

	}

	VulkanSpotlight::ShadowProjection& VulkanSpotlight::getLightUBO()
	{
		lightPerspective.model = ownerEntity->GetObjectTransformation();
		glm::mat4 view = glm::lookAt(ownerEntity->GetObjectPosition(), (glm::vec3(lightPerspective.model[2][0], lightPerspective.model[2][1], lightPerspective.model[2][2]) * 10.f + ownerEntity->GetObjectPosition()), glm::vec3(0, 1, 0));
		glm::mat4 proj = glm::perspective(glm::radians(80.f), 1.f, 0.01f, 15.f);
		float f = (15.f * 2.0) / (SHADOW_MAP_DIM);
		view[3][0] = glm::round(view[3][0] / f) * f;
		view[3][1] = glm::round(view[3][1] / f) * f;
		view[3][2] = glm::round(view[3][2] / f) * f;
		lightPerspective.viewproj = proj * view;
		lightPerspective.color = ownerEntity->GetPropertyValue(PropertyType::Color, glm::vec4(1.f));
		lightPerspective.spec.x = LightType::Spot;
		lightPerspective.spec.y = 80.f;
		lightPerspective.spec.z = 15.f;
		//lightPerspective.proj *= -1;

		return lightPerspective;
	}
}