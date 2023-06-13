#pragma once
#include <btBulletDynamicsCommon.h>
#include "BulletCollision/CollisionDispatch/btCollisionWorld.h"
#include "BulletDynamics/Character/btKinematicCharacterController.h"
#include "Engine/Headers/Virtual/MovementComponent.h"
#include "BulletPhysicsObject.h"
#include "Engine/Headers/Entities/Entity.h"

namespace GrEngineBullet
{
	class BulletMovementComponent : public GrEngine::MovementComponent
	{
		float timer = 0.f;
		std::chrono::steady_clock::time_point time;

	public:
		BulletMovementComponent(GrEngine::Entity* owner, btDiscreteDynamicsWorld* world) : MovementComponent(owner)
		{
			pOwner = owner;
			physComp = (BulletPhysObject*)pOwner->GetProperty(PropertyType::PhysComponent)->GetValueAdress();

			if (physComp != nullptr)
			{
				controls = new btKinematicCharacterController(static_cast<btPairCachingGhostObject*>(physComp->body), static_cast<btConvexShape*>(physComp->getShape()), 0.35f, btVector3(0, 1, 0));
				controls->setUseGhostSweepTest(false);
				controls->setAngularDamping(1000.f);
				controls->setMaxSlope(glm::radians(89.f));
				controls->setStepHeight(0.25f);
				controls->setMaxPenetrationDepth(0.01f);
				controls->setJumpSpeed(10.f);
				dynamicWorld = world;
				dynamicWorld->addAction(controls);
			}
		}

		~BulletMovementComponent()
		{
			Dispose();
		}

		void UpdateController() override
		{
			Dispose();
			physComp = (BulletPhysObject*)pOwner->GetProperty(PropertyType::PhysComponent)->GetValueAdress();

			if (physComp != nullptr)
			{
				controls = new btKinematicCharacterController(static_cast<btPairCachingGhostObject*>(physComp->body), static_cast<btConvexShape*>(physComp->getShape()), 0.35f, btVector3(0, 1, 0));
				controls->setUseGhostSweepTest(false);
				controls->setAngularDamping(1000.f);
				controls->setMaxSlope(glm::radians(89.f));
				controls->setStepHeight(0.25f);
				controls->setMaxPenetrationDepth(0.01f);
				controls->setJumpSpeed(10.f);
				dynamicWorld->addAction(controls);
			}
		}

		void ResetVelocity() override
		{
			if (controls != nullptr)
			{
				controls->setLinearVelocity(btVector3(0, 0, 0));
				controls->setAngularVelocity(btVector3(0, 0, 0));
			}
		}

		void Dispose() override
		{
			if (controls != nullptr)
			{
				dynamicWorld->removeAction(controls);
				delete controls;
				controls = nullptr;
			}
		}

		void MoveObject(glm::vec3 vector) override
		{
			auto now = std::chrono::steady_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - time).count();
			if (duration / 1000.f > timer)
			{
				if (controls != nullptr)
				{
					controls->setWalkDirection(btVector3(vector.x, vector.y, vector.z) * physComp->getVelocityMultiplier());
				}
			}
		}

		void SlideObjectForDuration(glm::vec3 vector, float dur) override
		{
			if (controls != nullptr)
			{
				controls->setWalkDirection(btVector3(vector.x, vector.y, vector.z) * physComp->getVelocityMultiplier());
				timer = dur;
				time = std::chrono::steady_clock::now();
			}
		}

		bool IsGrounded() override
		{
			return controls->onGround();
		}

		void Jump() override
		{
			if (controls->canJump())
			{
				controls->jump();
			}
		}

		btKinematicCharacterController* getController()
		{
			return controls;
		}

		inline bool IsOwnerStatic()
		{
			return pOwner->IsStatic();
		}

	private:
		btKinematicCharacterController* controls;
		BulletPhysObject* physComp;
		btDiscreteDynamicsWorld* dynamicWorld;
	};
}