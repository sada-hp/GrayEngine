#pragma once
#include <btBulletDynamicsCommon.h>
#include "BulletCollision/CollisionDispatch/btCollisionWorld.h"
#include "BulletDynamics/Character/btKinematicCharacterController.h"
#include "BulletCollision/NarrowPhaseCollision/btRaycastCallback.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"
#include "BulletCollision/CollisionShapes/btShapeHull.h"
#include "BulletResourceManager.h"
#include "Entities/Entity.h"
#include "Virtual/Physics.h"
#include "BulletPhysicsObject.h"
#include "BulletMovementComponent.h"
#include "Engine/Headers/Core/Globals.h"
#include "Engine/Headers/Core/Logger.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace GrEngineBullet
{
	class BulletAPI : public GrEngine::Physics
	{
		btDefaultCollisionConfiguration* collisionConfiguration = new btDefaultCollisionConfiguration();
		btCollisionDispatcher* dispatcher = new btCollisionDispatcher(collisionConfiguration);
		btBroadphaseInterface* broadphase = new btDbvtBroadphase();
		btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;
		btDiscreteDynamicsWorld* dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
		BulletResourceManager* resourceManager = new BulletResourceManager(dynamicsWorld);

	public:
		BulletAPI()
		{
			//collisionConfiguration->setConvexConvexMultipointIterations(10, 10);
			collisionConfiguration->setPlaneConvexMultipointIterations();
			collisionConfiguration->setConvexConvexMultipointIterations();
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
			delete resourceManager;
		}

		void SimulateStep() override;

		GrEngine::PhysicsObject* InitSimulationObject(GrEngine::Entity* owner) override;
		GrEngine::MovementComponent* InitController(GrEngine::Entity* owner) override;
		void AddSimulationObject(GrEngine::PhysicsObject* object) override;
		void RemoveSimulationObject(GrEngine::PhysicsObject* object) override;
		void RemoveController(GrEngine::MovementComponent* object) override;
		void RemoveSimulationObject(UINT id) override;
		void RemovePhysicsObject(void* object) override;
		void TogglePhysicsState(bool state) override;
		void SetGravity(glm::vec3 gravity_vector) override
		{
			dynamicsWorld->setGravity({ gravity_vector.x, gravity_vector.y, gravity_vector.z });
		}
		const GrEngine::RayCastResult CastRayGetHit(glm::vec3 startPoint, glm::vec3 endPoint) override;
		const GrEngine::RayCastResult CastRayToObject(glm::vec3 startPoint, glm::vec3 endPoint, UINT id) override;
		const std::vector<GrEngine::RayCastResult> GetObjectContactPoints(GrEngine::PhysicsObject* object, float radius) override;

		void CleanUp() override;
	private:
		void updateObjects();
		void resetObjects();
	};

	struct CollisionTest : public btCollisionWorld::ContactResultCallback
	{
		std::vector<GrEngine::RayCastResult>* out{};

		CollisionTest(std::vector<GrEngine::RayCastResult>* output)
		{
			out = output;
		}

		btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1) override
		{
			GrEngine::RayCastResult r{};
			r.hitPos = glm::vec3(cp.getPositionWorldOnB().x(), cp.getPositionWorldOnB().y(), cp.getPositionWorldOnB().z());
			r.hasHit = true;
			r.hitNorm = glm::vec3(cp.m_normalWorldOnB.x(), cp.m_normalWorldOnB.y(), cp.m_normalWorldOnB.z());
			r.distance = cp.getDistance();
			out->push_back(r);
			return 1;
		}
	};

	struct CollisionTestSingle : public btCollisionWorld::ContactResultCallback
	{
		GrEngine::RayCastResult& outSingle;

		CollisionTestSingle(GrEngine::RayCastResult& output) : outSingle(output)
		{
			outSingle = output;
		}

		btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1) override
		{
			GrEngine::RayCastResult r{};
			r.hitPos = glm::vec3(cp.getPositionWorldOnB().x(), cp.getPositionWorldOnB().y(), cp.getPositionWorldOnB().z());
			r.hasHit = true;
			r.hitNorm = glm::vec3(cp.m_normalWorldOnB.x(), cp.m_normalWorldOnB.y(), cp.m_normalWorldOnB.z());
			r.distance = cp.getDistance();
			r.id = (UINT)colObj1Wrap->getCollisionObject()->getUserPointer();
			outSingle = r;
			return 1;
		}
	};
};