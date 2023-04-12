#pragma once
#include "VulkanSpotlight.h"
#include "Engine/Headers/Virtual/Renderer.h"

#define SHADOW_MAP_CASCADE_COUNT 6

namespace GrEngine_Vulkan
{
	class VulkanCascade : public GrEngine::SpotlightObject
	{
	public:
		struct Cascade 
		{
			float splitDepth;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 model;
		};

		VulkanCascade(GrEngine::Entity* owningEntity) : GrEngine::SpotlightObject(owningEntity) {};

		void initLight(VkDevice device, VmaAllocator allocator);
		void destroyLight();
		std::array<Cascade, SHADOW_MAP_CASCADE_COUNT>& getCascadeUBO();

		std::array<Cascade, SHADOW_MAP_CASCADE_COUNT> lightPerspective;

	protected:
		VkDevice logicalDevice;
		VmaAllocator memAllocator;
	private:
		float cascadeSplitLambda = 0.96f;
		float cascadeSplits[SHADOW_MAP_CASCADE_COUNT];
		Cascade cascades[SHADOW_MAP_CASCADE_COUNT];
	};
};

