#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "PhysicsObject.h"
#include "MovementComponent.h"

namespace GrEngine
{
	struct RayCastResult
	{
		bool hasHit = false;
		glm::vec3 hitPos;
		glm::vec3 hitNorm;
		float distance;
		UINT id;
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
		virtual MovementComponent* InitController(Entity* owner) = 0;
		virtual void AddSimulationObject(PhysicsObject* object) = 0;
		virtual void RemoveSimulationObject(PhysicsObject* object) = 0;
		virtual void RemoveController(MovementComponent* object) = 0;
		virtual void RemoveSimulationObject(UINT id) = 0;
		virtual void RemovePhysicsObject(void* object) = 0;
		virtual void SetGravity(glm::vec3 gravity_vector) = 0;
		virtual void CleanUp() = 0;
		virtual void TogglePhysicsState(bool state) = 0;
		virtual const RayCastResult CastRayGetHit(glm::vec3 startPoint, glm::vec3 endPoint) = 0;
		virtual const RayCastResult CastRayToObject(glm::vec3 startPoint, glm::vec3 endPoint, UINT id) = 0;
		virtual const std::vector<GrEngine::RayCastResult> GetObjectContactPoints(PhysicsObject* object, float radius) = 0;

		inline bool GetSimulationState() { return simulate; };
	protected:
		bool simulate = false;
		std::vector<PhysicsObject*> objects;
		std::vector<MovementComponent*> controllers;
	};
};
