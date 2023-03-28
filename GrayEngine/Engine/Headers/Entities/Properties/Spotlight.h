#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Entities/Entity.h"
#include "Engine/Headers/Virtual/Physics.h"

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

	protected:
		Entity* ownerEntity = nullptr;
	};
};