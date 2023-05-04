#pragma once
#include "VulkanResourceManager.h"
#include "Entities/Properties/Light.h"

#define SHADOW_MAP_CASCADE_COUNT 4

namespace GrEngine_Vulkan
{
	class VulkanCascade : public GrEngine::LightObject
	{
	public:
		struct Cascade 
		{
			float splitDepth;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 model;
			glm::vec4 color;
		};

		VulkanCascade(GrEngine::Entity* owningEntity) : GrEngine::LightObject(owningEntity) {};

		void initLight(VkDevice device, VmaAllocator allocator);
		void destroyLight();
		std::array<Cascade, SHADOW_MAP_CASCADE_COUNT>& getCascadeUBO();
		void UpdateLight() override;


	protected:
		VkDevice logicalDevice;
		VmaAllocator memAllocator;
	private:
		std::array<Cascade, SHADOW_MAP_CASCADE_COUNT> lightPerspective;
		float cascadeSplitLambda = 0.96f;
		float cascadeSplits[SHADOW_MAP_CASCADE_COUNT];
		Cascade cascades[SHADOW_MAP_CASCADE_COUNT];
	};
};

