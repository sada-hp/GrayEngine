#include "pch.h"
#include "BulletAPI.h"

namespace GrEngineBullet
{
	void BulletAPI::SimulateStep()
	{
		if (simulate)
		{
			dynamicsWorld->stepSimulation(GrEngine::Globals::delta_time, 1, GrEngine::Globals::delta_time);
		}
	}

	void BulletAPI::updateObjects()
	{
		for (std::vector<GrEngine::PhysicsObject*>::iterator itt = objects.begin(); itt != objects.end(); ++itt)
		{
			BulletPhysObject* object = static_cast<BulletPhysObject*>(*itt);
			object->SetActivationState(simulate);
			object->CalculatePhysics();
		}
	}

	void BulletAPI::resetObjects()
	{
		for (std::vector<GrEngine::PhysicsObject*>::iterator itt = objects.begin(); itt != objects.end(); ++itt)
		{
			BulletPhysObject* object = static_cast<BulletPhysObject*>(*itt);
			object->SetActivationState(simulate);
			object->CalculatePhysics();
			object->ResetMotion();
		}
	}

	void BulletAPI::TogglePhysicsState(bool state)
	{
		simulate = state;
		resetObjects();
	}

	GrEngine::PhysicsObject* BulletAPI::InitSimulationObject(GrEngine::Entity* owner)
	{
		auto phys = new BulletPhysObject(static_cast<GrEngine::Entity*>(owner));
		objects.push_back(phys);
		if (phys->CalculatePhysics())
		{
			dynamicsWorld->addRigidBody(phys->body);
		}

		return phys;
	}

	void BulletAPI::AddSimulationObject(GrEngine::PhysicsObject* object)
	{
		objects.push_back(object);
	}

	void BulletAPI::RemoveSimulationObject(GrEngine::PhysicsObject* object)
	{
		for (int position = 0; position < objects.size(); position++)
		{
			if (objects[position] == object)
			{
				if (object->HasValue())
				{
					dynamicsWorld->removeRigidBody(static_cast<BulletPhysObject*>(object)->body);
					object->Dispose();
				}
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
		for (std::vector<GrEngine::PhysicsObject*>::iterator itt = objects.begin(); itt != objects.end(); ++itt)
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
		updateObjects();
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
		return raycastResult;
	}

	const GrEngine::RayCastResult BulletAPI::CastRayToObject(glm::vec3 startPoint, glm::vec3 endPoint, UINT id)
	{
		updateObjects();
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
			for (int i = 0; i < res.m_hitFractions.size(); i++)
			{
				if ((UINT)res.m_collisionObjects[i]->getUserPointer() == id)
				{
					btVector3 p = start.lerp(end, res.m_hitFractions[i]);
					raycastResult.hitPos = glm::vec3{ p.x(), p.y(), p.z() };
					raycastResult.hasHit = true;
					break;
				}
			}
		}

		return raycastResult;
	}
};