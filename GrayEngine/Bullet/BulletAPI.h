#pragma once
#include <btBulletDynamicsCommon.h>
#include "Virtual/Physics.h"
#include "Engine/Headers/Core/Globals.h"
#include "Engine/Headers/Core/Logger.h"

namespace GrEngineBullet
{
	class BulletAPI : public GrEngine::Physics
	{
		btDefaultCollisionConfiguration* collisionConfiguration = new btDefaultCollisionConfiguration();
		btCollisionDispatcher* dispatcher = new btCollisionDispatcher(collisionConfiguration);
		btBroadphaseInterface* broadphase = new btDbvtBroadphase();
		btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;
		btDiscreteDynamicsWorld* dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);

	public:
		struct BulletPhysObject : public GrEngine::Physics::PhysicsObject
		{
		public:
			BulletPhysObject(GrEngine::Entity* owner) : GrEngine::Physics::PhysicsObject(owner)
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
				Dispose();
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
			}

		private:
			bool initPhysics()
			{
				if (!CollisionEnabled)
					return false;

				float obj_mass = pOwner->GetPropertyValue<float>("Mass", 0.f);
				btTransform startTransform;
				glm::mat4 transformation = pOwner->GetObjectTransformation();
				const float* pSource = (const float*)glm::value_ptr(transformation);
				startTransform.setFromOpenGLMatrix(pSource);
				btVector3 localInertia{ 0,0,0 };

				if (obj_mass != 0.f)
					colShape->calculateLocalInertia(obj_mass, localInertia);

				glm::vec3 scale = pOwner->GetPropertyValue("Scale", glm::vec3(1.0f));
				colShape->setLocalScaling(btVector3(scale.x + 0.0001f, scale.y + 0.0001f, scale.z + 0.0001f));

				myMotionState = new btDefaultMotionState(startTransform);
				btRigidBody::btRigidBodyConstructionInfo rbInfo(obj_mass, myMotionState, colShape, localInertia);
				//rbInfo.m_angularDamping = .2f;
				//rbInfo.m_linearDamping = .2f;
				body = new btRigidBody(rbInfo);
				initialized = true;
				
				return true;
			}
		};

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

		void AddSimulationObject(GrEngine::Physics::PhysicsObject* object) override;
		void RemoveSimulationObject(GrEngine::Physics::PhysicsObject* object) override;
		void RemovePhysicsObject(void* object) override;
		void TogglePhysicsState(bool state) override;

		void CleanUp() override;
	};
};

