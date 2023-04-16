#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "PhysicsObject.h"

namespace GrEngine
{
	struct RayCastResult
	{
		bool hasHit = false;
		glm::vec3 hitPos;
	};

	class Physics
	{
	public:
	
		Physics()
		{
			
		}

		~Physics() {};

		virtual void SimulateStep() = 0;
		virtual PhysicsObject* InitSimulationObject(Entity* owner) = 0;
		virtual void AddSimulationObject(PhysicsObject* object) = 0;
		virtual void RemoveSimulationObject(PhysicsObject* object) = 0;
		virtual void RemovePhysicsObject(void* object) = 0;
		virtual void CleanUp() = 0;
		virtual void TogglePhysicsState(bool state) = 0;
		virtual const RayCastResult CastRayGetHit(glm::vec3 startPoint, glm::vec3 endPoint) = 0;
		virtual const RayCastResult CastRayToObject(glm::vec3 startPoint, glm::vec3 endPoint, UINT id) = 0;

		inline bool GetSimulationState() { return simulate; };
	protected:
		bool simulate = false;
		std::vector<PhysicsObject*> objects;
	};
};
