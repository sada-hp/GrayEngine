#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Entities/Entity.h"

namespace GrEngine
{
	class Entity;

	class PhysicsObject
	{
	public:
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
		virtual void GenerateBoxCollision(float width, float height, float length) = 0;
		virtual void ResetMotion() = 0;
		virtual void MoveObject(glm::vec3 vector) = 0;
		virtual void SetActivationState(bool state)
		{
			active = state;
		}

		virtual void SetKinematic(int value) = 0;


		bool HasValue()
		{
			return initialized && active;
		}

		void QueueUpdate()
		{
			was_updated = true;
		}

		bool IsUpdateNeeded()
		{
			return was_updated;
		}

		virtual UINT GetID() = 0;

	protected:
		bool was_updated = false;
		bool initialized = false;
		bool active = false;
		Entity* pOwner;
	};
};