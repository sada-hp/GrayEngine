#include "pch.h"
#include "VulkanCascade.h"
#include "VulkanRenderer.h"

namespace GrEngine_Vulkan
{
	void VulkanCascade::initLight(VkDevice device, VmaAllocator allocator)
	{
		logicalDevice = device;
		memAllocator = allocator;
		type = LightType::Cascade;
		max_distance = GrEngine::Renderer::FarPlane;
		UpdateLight();
	}

	void VulkanCascade::destroyLight()
	{

	}

	void VulkanCascade::UpdateLight()
	{
		max_distance = ownerEntity->GetPropertyValue(PropertyType::MaximumDistance, GrEngine::Renderer::FarPlane);
		max_distance = max_distance < 0 ? GrEngine::Renderer::FarPlane : max_distance;
		brightness = ownerEntity->GetPropertyValue(PropertyType::Brightness, 1.f);
		float nearClip = GrEngine::Renderer::NearPlane;
		float farClip = max_distance;
		float clipRange = farClip - nearClip;

		float minZ = nearClip;
		float maxZ = nearClip + clipRange;

		float range = maxZ - minZ;
		float ratio = maxZ / minZ;

		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
		{
			float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
			float log = minZ * std::pow(ratio, p);
			float uniform = minZ + range * p;
			float d = cascadeSplitLambda * (log - uniform) + uniform;
			cascadeSplits[i] = (d - nearClip) / clipRange;
		}
	}

	std::array<VulkanCascade::Cascade, SHADOW_MAP_CASCADE_COUNT>& VulkanCascade::getCascadeUBO()
	{
		float nearClip = GrEngine::Renderer::NearPlane;
		float farClip = GrEngine::Renderer::FarPlane;
		float clipRange = farClip - nearClip;
		float lastSplitDist = 0.0;
		glm::vec4 color = ownerEntity->GetPropertyValue(PropertyType::Color, glm::vec4(1.f));

		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
			float splitDist = cascadeSplits[i];

			glm::vec3 frustumCorners[8] = 
			{
				glm::vec3(-1.0f,  1.0f, 0.0f),
				glm::vec3(1.0f,  1.0f, 0.0f),
				glm::vec3(1.0f, -1.0f, 0.0f),
				glm::vec3(-1.0f, -1.0f, 0.0f),
				glm::vec3(-1.0f,  1.0f,  1.0f),
				glm::vec3(1.0f,  1.0f,  1.0f),
				glm::vec3(1.0f, -1.0f,  1.0f),
				glm::vec3(-1.0f, -1.0f,  1.0f),
			};

			auto cproj = static_cast<VulkanRenderer*>(GrEngine::Engine::GetContext()->GetRenderer())->vpUBO.proj;
			//cproj[1][1] *= -1;
			auto cview = static_cast<VulkanRenderer*>(GrEngine::Engine::GetContext()->GetRenderer())->vpUBO.view;
			glm::mat4 invCam = glm::inverse(cproj * cview);
			for (uint32_t i = 0; i < 8; i++) 
			{
				glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
				frustumCorners[i] = invCorner / invCorner.w;
			}

			for (uint32_t i = 0; i < 4; i++) 
			{
				glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
				frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
				frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
			}

			glm::vec3 frustumCenter = glm::vec3(0.0f);
			for (uint32_t i = 0; i < 8; i++) 
			{
				frustumCenter += frustumCorners[i];
			}
			frustumCenter /= 8.0f;

			float radius = 0.0f;
			for (uint32_t i = 0; i < 8; i++) 
			{
				float distance = glm::length(frustumCorners[i] - frustumCenter);
				radius = glm::max(radius, distance);
			}
			radius = glm::ceil(radius / 8.f) * 8.f;


			glm::vec3 maxExtents = glm::vec3(radius);
			glm::vec3 minExtents = -maxExtents;

			lightPerspective[i].model = ownerEntity->GetObjectTransformation();
			//glm::vec3 lightDir = (glm::vec3(lightPerspective[i].model[2][0], lightPerspective[i].model[2][1], lightPerspective[i].model[2][2]));
			glm::vec3 lightDir = normalize(-ownerEntity->GetObjectPosition());
			lightPerspective[i].view = glm::lookAt(frustumCenter - lightDir * maxExtents.z, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
			lightPerspective[i].proj = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, minExtents.z * 2, maxExtents.z * 2);
			//lightPerspective[i].proj[1][1] *= -1;
			lightPerspective[i].splitDepth = (nearClip + splitDist * clipRange);


			//Fixes shadow shimmering
			float f = (radius * 8.0) / (SHADOW_MAP_DIM);
			lightPerspective[i].view[3][0] = glm::ceil(lightPerspective[i].view[3][0] / f) * f;
			lightPerspective[i].view[3][1] = glm::ceil(lightPerspective[i].view[3][1] / f) * f;
			lightPerspective[i].view[3][2] = glm::ceil(lightPerspective[i].view[3][2] / f) * f;

			lightPerspective[i].color = color;

			lastSplitDist = cascadeSplits[i];
		}

		return lightPerspective;
	}
}