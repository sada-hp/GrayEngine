#include "pch.h"
#include "BulletAPI.h"
#include "Entities/DrawableObject.h"

namespace GrEngineBullet
{
	void BulletAPI::SimulateStep()
	{
		if (simulate)
		{
			dynamicsWorld->stepSimulation(GrEngine::Globals::delta_time, 10);
		}
	}

	void BulletAPI::TogglePhysicsState(bool state)
	{
		dynamicsWorld->getCollisionObjectArray().resize(0);

		if (state == false)
		{
			for (std::vector<GrEngine::Physics::PhysicsObject*>::iterator itt = objects.begin(); itt != objects.end(); ++itt)
			{
				BulletPhysObject* object = static_cast<BulletPhysObject*>(*itt);

				if (object->HasValue())
				{
					dynamicsWorld->removeRigidBody(object->body);
					object->Dispose();
				}
			}
		}
		else
		{
			for (std::vector<GrEngine::Physics::PhysicsObject*>::iterator itt = objects.begin(); itt != objects.end(); ++itt)
			{
				BulletPhysObject* object = static_cast<BulletPhysObject*>(*itt);
				if (object->CalculatePhysics())
				{
					dynamicsWorld->addRigidBody(object->body);
				}
			}
		}

		simulate = state;
	}

	void BulletAPI::AddSimulationObject(GrEngine::Physics::PhysicsObject* object)
	{
		objects.push_back(object);
	}

	void BulletAPI::RemoveObject(void* object)
	{
		dynamicsWorld->removeCollisionObject(static_cast<btCollisionObject*>(object));
	}

	void BulletAPI::RemovePhysicsObject(void* object)
	{
		dynamicsWorld->removeRigidBody(static_cast<btRigidBody*>(object));
	}

	void BulletAPI::CleanUp()
	{
		simulate = false;
		dynamicsWorld->getCollisionObjectArray().clear();
		for (std::vector<GrEngine::Physics::PhysicsObject*>::iterator itt = objects.begin(); itt != objects.end(); ++itt)
		{
			BulletPhysObject* object = static_cast<BulletPhysObject*>(*itt);

			if (object->HasValue())
			{
				dynamicsWorld->removeRigidBody(object->body);
				object->Dispose();
			}
		}
		objects.resize(0);
	}
};