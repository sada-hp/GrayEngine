#include "pch.h"
#include "BulletAPI.h"

namespace GrEngineBullet
{
	void BulletAPI::SimulateStep()
	{
		if (simulate)
		{
			dynamicsWorld->stepSimulation(GrEngine::Globals::delta_time);
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

	void BulletAPI::RemoveSimulationObject(GrEngine::Physics::PhysicsObject* object)
	{
		for (int position = 0; position < objects.size(); position++)
		{
			if (objects[position] == object)
			{
				objects.erase(objects.begin() + position);
				return;
			}
		}
	}

	void BulletAPI::RemovePhysicsObject(void* object)
	{
		dynamicsWorld->removeRigidBody(static_cast<btRigidBody*>(object));
	}

	void BulletAPI::CleanUp()
	{
		simulate = false;
		dynamicsWorld->getCollisionObjectArray().resize(0);
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

	const GrEngine::RayCastResult BulletAPI::CastRayGetHit(glm::vec3 startPoint, glm::vec3 endPoint)
	{
		for (std::vector<GrEngine::Physics::PhysicsObject*>::iterator itt = objects.begin(); itt != objects.end(); ++itt)
		{
			BulletPhysObject* object = static_cast<BulletPhysObject*>(*itt);
			if (object->CalculatePhysics())
			{
				dynamicsWorld->addRigidBody(object->body);
			}
		}

		dynamicsWorld->updateAabbs();
		dynamicsWorld->computeOverlappingPairs();

		btVector3 start = btVector3(startPoint.x, startPoint.y, startPoint.z);
		btVector3 end = btVector3(endPoint.x, endPoint.y, endPoint.z);
		btCollisionWorld::AllHitsRayResultCallback res(start, end);
		res.m_flags |= btTriangleRaycastCallback::kF_UseGjkConvexCastRaytest;
		res.m_flags |= btTriangleRaycastCallback::kF_KeepUnflippedNormal;

		dynamicsWorld->rayTest(start, end, res);

		GrEngine::RayCastResult raycastResult{};
		raycastResult.hasHit = res.hasHit();

		if (res.hasHit())
		{
			raycastResult.hasHit = true;
			btVector3 p = start.lerp(end, res.m_hitFractions[0]);
			raycastResult.hitPos = glm::vec3{ p.x(), p.y(), p.z() };
		}

		for (std::vector<GrEngine::Physics::PhysicsObject*>::iterator itt = objects.begin(); itt != objects.end(); ++itt)
		{
			BulletPhysObject* object = static_cast<BulletPhysObject*>(*itt);

			if (object->HasValue())
			{
				dynamicsWorld->removeRigidBody(object->body);
				object->Dispose();
			}
		}

		return raycastResult;
	}

	const GrEngine::RayCastResult BulletAPI::CastRayToObject(glm::vec3 startPoint, glm::vec3 endPoint, UINT id)
	{
		for (std::vector<GrEngine::Physics::PhysicsObject*>::iterator itt = objects.begin(); itt != objects.end(); ++itt)
		{
			BulletPhysObject* object = static_cast<BulletPhysObject*>(*itt);
			if (object->GetID() == id && object->CalculatePhysics())
			{
				dynamicsWorld->addRigidBody(object->body);
			}
		}

		dynamicsWorld->updateAabbs();
		dynamicsWorld->computeOverlappingPairs();

		btVector3 start = btVector3(startPoint.x, startPoint.y, startPoint.z);
		btVector3 end = btVector3(endPoint.x, endPoint.y, endPoint.z);
		btCollisionWorld::AllHitsRayResultCallback res(start, end);
		res.m_flags |= btTriangleRaycastCallback::kF_UseGjkConvexCastRaytest;
		res.m_flags |= btTriangleRaycastCallback::kF_KeepUnflippedNormal;

		dynamicsWorld->rayTest(start, end, res);

		GrEngine::RayCastResult raycastResult{};
		raycastResult.hasHit = res.hasHit();

		if (res.hasHit())
		{
			raycastResult.hasHit = true;
			btVector3 p = start.lerp(end, res.m_hitFractions[0]);
			raycastResult.hitPos = glm::vec3{ p.x(), p.y(), p.z() };
		}

		for (std::vector<GrEngine::Physics::PhysicsObject*>::iterator itt = objects.begin(); itt != objects.end(); ++itt)
		{
			BulletPhysObject* object = static_cast<BulletPhysObject*>(*itt);

			if (object->HasValue())
			{
				dynamicsWorld->removeRigidBody(object->body);
				object->Dispose();
			}
		}
		dynamicsWorld->getCollisionObjectArray().resize(0);

		return raycastResult;
	}
};