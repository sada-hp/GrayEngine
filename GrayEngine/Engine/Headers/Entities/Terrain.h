#pragma once
#include <pch.h>
#include "Entity.h"
#include "Engine/Headers/Engine.h"
#include "Engine/Headers/Virtual/Physics.h"

namespace GrEngine
{
	class Terrain : public Entity
	{
	public:
		Terrain()
		{
			Type = "Terrain";
		}

		Terrain(UINT ID) : Entity(ID)
		{
			Type = "Terrain";
		}

		virtual ~Terrain()
		{
			if (physComponent != nullptr)
			{
				physComponent->Dispose();
				delete physComponent;
			}
		}

		virtual void GenerateTerrain(uint16_t resolution) = 0;
		virtual void calculateCollisions() = 0;

		Physics::PhysicsObject* physComponent;
	};
}