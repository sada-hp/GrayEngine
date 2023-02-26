#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Entities/Entity.h"

namespace GrEngine
{
	class Physics
	{
	public:
		struct PhysicsObject
		{
			PhysicsObject(Entity* owner)
			{
				pOwner = owner;
			}

			virtual ~PhysicsObject()
			{

			}

			bool CollisionEnabled = true;

			virtual bool CalculatePhysics() = 0;
			virtual void DisablePhysics() = 0;
			virtual glm::vec3 GetPhysPosition() = 0;
			virtual glm::quat GetPhysOrientation() = 0;
			virtual void Dispose() = 0;
			virtual void UpdateCollisionShape(void* shape) = 0;

			bool HasValue()
			{
				return initialized;
			}

			void QueueUpdate()
			{
				was_updated = true;
			}

		protected:
			bool was_updated = false;
			bool initialized = false;
			bool locked;
			Entity* pOwner;
		};

		Physics()
		{
			
		}

		~Physics() {};

		virtual void SimulateStep() = 0;
		virtual void AddSimulationObject(PhysicsObject* object) = 0;
		virtual void RemoveSimulationObject(PhysicsObject* object) = 0;
		virtual void RemovePhysicsObject(void* object) = 0;
		virtual void CleanUp() = 0;
		virtual void TogglePhysicsState(bool state) = 0;

		inline bool GetSimulationState() { return simulate; };
	protected:
		bool simulate = false;
		std::vector<PhysicsObject*> objects;
	};
};
