#include "pch.h"
#include "BulletAPI.h"

namespace GrEngineBullet
{
	void BulletAPI::SimulateStep()
	{
		if (simulate)
		{
			dynamicsWorld->stepSimulation(GrEngine::Globals::delta_time, 10);
			int numObjects = dynamicsWorld->getNumCollisionObjects();

			for (int i = 0; i < numObjects; i++)
			{
				btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
				btRigidBody* body = btRigidBody::upcast(obj);
				btTransform trans;
				if (body && body->getMotionState())
				{
					body->getMotionState()->getWorldTransform(trans);
				}
				else
				{
					trans = obj->getWorldTransform();
				}
			}
		}
	}

	void BulletAPI::AddSimulationObject(void* object)
	{
		dynamicsWorld->addRigidBody(static_cast<btRigidBody*>(object));
	}

	void BulletAPI::RemoveObject(void* object)
	{
		dynamicsWorld->removeRigidBody(static_cast<btRigidBody*>(object));
	}
};