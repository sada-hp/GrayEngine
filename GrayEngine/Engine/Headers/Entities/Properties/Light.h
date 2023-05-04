#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Entities/Entity.h"
#include "Engine/Headers/Virtual/Physics.h"

#define SHADOW_MAP_DIM 2048

enum LightType
{
	Spot = 1,
	Cascade,
	Point,
	Omni
};

namespace GrEngine
{
	class LightObject
	{
	public:

		LightObject(Entity* owner)
		{
			ownerEntity = owner;
		};

		virtual ~LightObject()
		{

		};

		virtual const LightType GetLightType()
		{
			return type;
		}

		virtual void UpdateLight() = 0;

	protected:
		Entity* ownerEntity = nullptr;
		LightType type;
		float max_distance = -1.f;
		float brightness = 1.f;
	};
};