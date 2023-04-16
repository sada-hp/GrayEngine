#pragma once
#include <btBulletDynamicsCommon.h>
#include "BulletCollision/NarrowPhaseCollision/btRaycastCallback.h"
#include "Entities/Entity.h"
#include "Virtual/Physics.h"
#include "Virtual/PhysicsObject.h"
#include "Engine/Headers/Core/Globals.h"
#include "Engine/Headers/Core/Logger.h"

namespace GrEngineBullet
{
	class BulletPhysObject : public GrEngine::PhysicsObject
	{
	public:
		BulletPhysObject(GrEngine::Entity* owner) : GrEngine::PhysicsObject(owner)
		{
			colShape = new btBoxShape(btVector3(0.25f, 0.25f, 0.25f));
		}

		~BulletPhysObject()
		{
			Dispose();
		}

		btCollisionShape* colShape;
		btRigidBody* body;
		btCollisionObject collision;
		btDefaultMotionState* myMotionState;

		bool CalculatePhysics() override
		{
			return initPhysics();
		}

		void DisablePhysics() override
		{
			CollisionEnabled = false;
			//Dispose();
			SetKinematic(3);
		}

		glm::vec3 GetPhysPosition() override
		{
			auto phys_pos = body->getWorldTransform().getOrigin();
			return glm::vec3(phys_pos.x(), phys_pos.y(), phys_pos.z());
		}

		glm::quat GetPhysOrientation() override
		{
			auto phys_pos = body->getWorldTransform().getRotation();
			return glm::quat(phys_pos.w(), phys_pos.x(), phys_pos.y(), phys_pos.z());
		}

		void MoveObject(glm::vec3 vector) override
		{
			btTransform newTrans = body->getWorldTransform();
			newTrans.getOrigin() += (btVector3(vector.x, vector.y, vector.z));
			body->setWorldTransform(newTrans);
		}

		void Dispose() override
		{
			if (initialized)
			{
				delete body;
				body = nullptr;
				delete myMotionState;
				myMotionState = nullptr;
			}

			initialized = false;
		}

		void UpdateCollisionShape(void* shape) override
		{
			if (colShape != nullptr)
			{
				delete colShape;
				colShape = nullptr;
			}

			colShape = static_cast<btCollisionShape*>(shape);
			CalculatePhysics();
		}

		UINT GetID() override
		{
			return pOwner->GetEntityID();
		}

		void GenerateBoxCollision(float width, float height, float length) override
		{
			UpdateCollisionShape(new btBoxShape(btVector3(width, height, length)));
		}

		void SetKinematic(int value)
		{
			if (value == 0)
			{
				flags = 0;
			}
			else if (value == 1)
			{
				flags = btCollisionObject::CF_KINEMATIC_OBJECT;
			}
			else if (value == 2)
			{
				flags = btCollisionObject::CF_STATIC_OBJECT;
			}
			else
			{
				flags = btCollisionObject::CF_NO_CONTACT_RESPONSE;
			}

			if (body != nullptr)
			{
				body->setCollisionFlags(flags);

				if ((body->getCollisionFlags() & btCollisionObject::CF_KINEMATIC_OBJECT) != 0)
				{
					body->setActivationState(DISABLE_DEACTIVATION);
				}
				else if ((body->getCollisionFlags() & btCollisionObject::CF_STATIC_OBJECT) != 0)
				{
					body->setActivationState(0);
				}
				else
				{
					body->setActivationState(ACTIVE_TAG);
					body->setDeactivationTime(1.f);
				}
			}

			CalculatePhysics();
		}

		void ResetMotion()
		{
			body->setLinearVelocity(btVector3(0, 0, 0));
			body->setAngularVelocity(btVector3(0, 0, 0));
			body->activate();
		}

	private:
		bool initPhysics()
		{
			/*if (!CollisionEnabled)
			return false;*/

			if (myMotionState != nullptr)
			{
				delete myMotionState;
				myMotionState = nullptr;
			}

			initialized = false;

			float obj_mass = pOwner->GetPropertyValue(PropertyType::Mass, 0.f);
			btTransform startTransform;
			glm::mat4 transformation = pOwner->GetObjectTransformation();
			const float* pSource = (const float*)glm::value_ptr(transformation);
			startTransform.setFromOpenGLMatrix(pSource);
			btVector3 localInertia{ 0,0,0 };

			if (colShape->getShapeType() == 21 && was_updated)
			{
				static_cast<btBvhTriangleMeshShape*>(colShape)->buildOptimizedBvh();
				was_updated = false;
			}

			if (obj_mass != 0.f && (body->getCollisionFlags() & btCollisionObject::CF_STATIC_OBJECT) != 0)
				colShape->calculateLocalInertia(obj_mass, localInertia);
			else
				obj_mass = 0.f;

			glm::vec3 scale = pOwner->GetPropertyValue(PropertyType::Scale, glm::vec3(1.0f));
			colShape->setLocalScaling(btVector3(scale.x + 0.00001f, scale.y + 0.00001f, scale.z + 0.00001f));

			myMotionState = new btDefaultMotionState(startTransform);

			if (body == nullptr)
			{
				btRigidBody::btRigidBodyConstructionInfo rbInfo(obj_mass, myMotionState, colShape, localInertia);
				body = new btRigidBody(rbInfo);
				body->setCollisionFlags(flags);
				if ((body->getCollisionFlags() & btCollisionObject::CF_KINEMATIC_OBJECT) != 0)
				{
					body->setActivationState(DISABLE_DEACTIVATION);
				}
				else
				{
					body->setActivationState(ACTIVE_TAG);
					body->setDeactivationTime(1.f);
				}
			}
			else
			{
				body->setMassProps(obj_mass, localInertia);
				body->setCollisionShape(colShape);
				body->setMotionState(myMotionState);
			}

			body->setUserPointer((void*)pOwner->GetEntityID());

			initialized = true;

			return true;
		}

		int flags = 0;
	};

	class BulletAPI : public GrEngine::Physics
	{
		btDefaultCollisionConfiguration* collisionConfiguration = new btDefaultCollisionConfiguration();
		btCollisionDispatcher* dispatcher = new btCollisionDispatcher(collisionConfiguration);
		btBroadphaseInterface* broadphase = new btDbvtBroadphase();
		btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;
		btDiscreteDynamicsWorld* dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);

	public:
		BulletAPI()
		{
			dynamicsWorld->setGravity(btVector3(0, -9.8, 0));
		}

		~BulletAPI()
		{
			simulate = false;
			delete dynamicsWorld;
			delete solver;
			delete broadphase;
			delete dispatcher;
			delete collisionConfiguration;
		}

		void SimulateStep() override;

		GrEngine::PhysicsObject* InitSimulationObject(GrEngine::Entity* owner) override;
		void AddSimulationObject(GrEngine::PhysicsObject* object) override;
		void RemoveSimulationObject(GrEngine::PhysicsObject* object) override;
		void RemovePhysicsObject(void* object) override;
		void TogglePhysicsState(bool state) override;
		const GrEngine::RayCastResult CastRayGetHit(glm::vec3 startPoint, glm::vec3 endPoint) override;
		const GrEngine::RayCastResult CastRayToObject(glm::vec3 startPoint, glm::vec3 endPoint, UINT id) override;

		void CleanUp() override;
	private:
		void updateObjects();
		void resetObjects();
	};
};