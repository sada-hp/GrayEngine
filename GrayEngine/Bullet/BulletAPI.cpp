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
		simulate = state;

		for (std::vector<void*>::iterator itt = objects.begin(); itt != objects.end(); ++itt)
		{
			GrEngine::DrawableObject* object = static_cast<GrEngine::DrawableObject*>(*itt);
			object->recalculatePhysics(simulate);
		}

		if (simulate == false)
			dynamicsWorld->getCollisionObjectArray().resize(0);
	}

	void BulletAPI::AddSimulationObject(void* object)
	{
		objects.push_back(object);
	}

	void BulletAPI::AddPhysicsObject(void* object)
	{
		dynamicsWorld->addRigidBody(static_cast<btRigidBody*>(object));
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
		objects.resize(0);
	}
};