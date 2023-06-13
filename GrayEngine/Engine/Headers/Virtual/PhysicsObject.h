#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Entities/Entity.h"

namespace GrEngine
{
	class Physics;
	class Entity;

	class PhysicsObject
	{
	public:
		PhysicsObject(Entity* owner, Physics* context)
		{
			pOwner = owner;
			pContext = context;
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
		virtual void GenerateBoxCollision(float width, float height, float length) = 0;
		virtual void GenerateCapsuleCollision(float radius, float height) = 0;
		virtual bool LoadCollisionMesh(const char* mesh_path, bool use_hull = true) = 0;
		virtual void ResetMotion() = 0;
		virtual void AddCollisionResource(const char* name, void* data) = 0;
		virtual void SetActivationState(bool state)
		{
			active = state;
		}
		virtual void VelocityMultiplier(glm::vec3 axes)
		{
			vAxes = axes;
		}
		virtual void SetKinematic(int value) = 0;
		virtual void UpdateCollisionType(int value) = 0;

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
		Physics* GetSimulationContext()
		{
			return pContext;
		}

	protected:
		bool was_updated = false;
		bool initialized = false;
		bool active = false;
		Entity* pOwner;
		Physics* pContext;
		glm::vec3 vAxes = {1, 1, 1};
	};
};