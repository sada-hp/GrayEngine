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

		virtual void GenerateTerrain(int resolution, int width, int height, int depth, std::array<std::string, 6> images) = 0;
		virtual void calculateCollisions() = 0;
		virtual void UpdateFoliageMask(void* pixels) = 0;

		Physics::PhysicsObject* physComponent;
	};
}