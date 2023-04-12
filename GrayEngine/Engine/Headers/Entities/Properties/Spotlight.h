#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Entities/Entity.h"
#include "Engine/Headers/Virtual/Physics.h"

#define SHADOW_MAP_DIM 2048

enum LightType
{
	Spot = 1,
	Cascade
};

namespace GrEngine
{
	class SpotlightObject
	{
	public:

		SpotlightObject(Entity* owner)
		{
			ownerEntity = owner;
		};

		virtual ~SpotlightObject()
		{

		};

		virtual const LightType GetLightType()
		{
			return type;
		}

	protected:
		Entity* ownerEntity = nullptr;
		LightType type;
	};
};