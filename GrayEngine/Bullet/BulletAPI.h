#pragma once
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

		void AddSimulationObject(void* object) override;
		void AddPhysicsObject(void* object) override;
		void RemoveObject(void* object) override;
		void RemovePhysicsObject(void* object) override;
		void TogglePhysicsState(bool state) override;

		void CleanUp() override;
	};
};

