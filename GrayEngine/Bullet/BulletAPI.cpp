#include "pch.h"
#include "BulletAPI.h"

namespace GrEngineBullet
{
	void BulletAPI::SimulateStep()
	{
		constexpr double physics_step = 1 / 60.0;
		if (simulate)
		{
			dynamicsWorld->stepSimulation(GrEngine::Globals::delta_time, 5, physics_step);
		}
	}

	void BulletAPI::updateObjects()
	{
		for (std::vector<GrEngine::PhysicsObject*>::iterator itt = objects.begin(); itt != objects.end(); ++itt)
		{
			BulletPhysObject* object = static_cast<BulletPhysObject*>(*itt);
			object->SetActivationState(simulate);
			object->CalculatePhysics();
			dynamicsWorld->refreshBroadphaseProxy(object->body);
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

		for (std::vector<GrEngine::MovementComponent*>::iterator itt = controllers.begin(); itt != controllers.end(); ++itt)
		{
			BulletMovementComponent* object = static_cast<BulletMovementComponent*>(*itt);
			object->UpdateController();
			object->ResetVelocity();
		}
		solver->reset();
	}

	void BulletAPI::TogglePhysicsState(bool state)
	{
		simulate = state;
		resetObjects();
	}

	GrEngine::PhysicsObject* BulletAPI::InitSimulationObject(GrEngine::Entity* owner)
	{
		auto phys = new BulletPhysObject(static_cast<GrEngine::Entity*>(owner), this, resourceManager, dynamicsWorld);
		if (phys->CalculatePhysics())
		{
			AddSimulationObject(phys);
		}

		return phys;
	}

	GrEngine::MovementComponent* BulletAPI::InitController(GrEngine::Entity* owner)
	{
		owner->AddNewProperty(PropertyType::PhysComponent);
		owner->AddNewProperty(PropertyType::BodyType)->ParsePropertyValue("1");
		controllers.push_back(new BulletMovementComponent(owner, dynamicsWorld));
		return controllers.back();
	}

	void BulletAPI::AddSimulationObject(GrEngine::PhysicsObject* object)
	{
		objects.push_back(object);
		object->SetActivationState(simulate);

		if (simulate)
		{
			BulletPhysObject* obj = static_cast<BulletPhysObject*>(object);
			dynamicsWorld->refreshBroadphaseProxy(obj->body);
		}
	}

	void BulletAPI::RemoveSimulationObject(GrEngine::PhysicsObject* object)
	{
		for (int position = 0; position < objects.size(); position++)
		{
			if (objects[position] == object)
			{
				if (static_cast<BulletPhysObject*>(object)->body != nullptr)
				{
					object->Dispose();
				}
				objects.erase(objects.begin() + position);
				return;
			}
		}
	}

	void BulletAPI::RemoveController(GrEngine::MovementComponent* object)
	{
		for (int position = 0; position < controllers.size(); position++)
		{
			if (controllers[position] == object)
			{
				object->Dispose();
				controllers.erase(controllers.begin() + position);
				return;
			}
		}
	}

	void BulletAPI::RemoveSimulationObject(UINT id)
	{
		for (int position = 0; position < objects.size(); position++)
		{
			if (objects[position]->GetID() == id)
			{
				BulletPhysObject* object = static_cast<BulletPhysObject*>(objects[position]);
				object->Dispose();
				delete object;
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
		TogglePhysicsState(false);
		int offset = 0;
		while (controllers.size() != offset)
		{
			std::vector<GrEngine::MovementComponent*>::iterator pos = controllers.begin();
			std::advance(pos, offset);
			BulletMovementComponent* controller = static_cast<BulletMovementComponent*>(*pos);

			if (!controller->IsOwnerStatic())
			{
				controller->Dispose();
				delete controller;
				controllers.erase(pos);
			}
			else
			{
				controller->Dispose();
				offset++;
			}
		}

		offset = 0;
		while (objects.size() != offset)
		{
			std::vector<GrEngine::PhysicsObject*>::iterator pos = objects.begin();
			std::advance(pos, offset);
			BulletPhysObject* object = static_cast<BulletPhysObject*>(*pos);

			if (!object->IsOwnerStatic())
			{
				object->Dispose();
				delete object;
				objects.erase(pos);
			}
			else
			{
				object->Dispose();
				offset++;
			}
		}

		resourceManager->Clean();
	}

	const GrEngine::RayCastResult BulletAPI::CastRayGetHit(glm::vec3 startPoint, glm::vec3 endPoint)
	{
		if (startPoint == endPoint) return GrEngine::RayCastResult{};

		dynamicsWorld->updateAabbs();
		dynamicsWorld->computeOverlappingPairs();

		btVector3 start = btVector3(startPoint.x, startPoint.y, startPoint.z);
		btVector3 end = btVector3(endPoint.x, endPoint.y, endPoint.z);
		btCollisionWorld::AllHitsRayResultCallback res(start, end);
		res.m_flags |= btTriangleRaycastCallback::kF_UseGjkConvexCastRaytest;
		//res.m_flags |= btTriangleRaycastCallback::kF_KeepUnflippedNormal;

		dynamicsWorld->rayTest(start, end, res);

		GrEngine::RayCastResult raycastResult{};
		raycastResult.hasHit = res.hasHit();

		if (res.hasHit())
		{
			btVector3 p = start.lerp(end, res.m_hitFractions[0]);
			btVector3 n = btVector3(res.m_hitNormalWorld[0].m_floats[0], res.m_hitNormalWorld[0].m_floats[1], res.m_hitNormalWorld[0].m_floats[2]);
			raycastResult.hasHit = true;
			raycastResult.hitPos = glm::vec3{ p.x(), p.y(), p.z() };
			raycastResult.hitNorm = glm::vec3{ n.x(), n.y(), n.z() };
			raycastResult.id = (UINT)res.m_collisionObjects[0]->getUserPointer();
		}
		return raycastResult;
	}

	const GrEngine::RayCastResult BulletAPI::CastRayToObject(glm::vec3 startPoint, glm::vec3 endPoint, UINT id)
	{
		if (startPoint == endPoint) return GrEngine::RayCastResult{};

		dynamicsWorld->updateAabbs();
		dynamicsWorld->computeOverlappingPairs();

		btVector3 start = btVector3(startPoint.x, startPoint.y, startPoint.z);
		btVector3 end = btVector3(endPoint.x, endPoint.y, endPoint.z);
		btCollisionWorld::AllHitsRayResultCallback res(start, end);
		res.m_flags |= btTriangleRaycastCallback::kF_UseGjkConvexCastRaytest;
		//res.m_flags |= btTriangleRaycastCallback::kF_KeepUnflippedNormal;

		dynamicsWorld->rayTest(start, end, res);

		GrEngine::RayCastResult raycastResult{};
		raycastResult.hasHit = res.hasHit();

		if (res.hasHit())
		{
			for (int i = 0; i < res.m_collisionObjects.size(); i++)
			{
				if ((UINT)res.m_collisionObjects[i]->getUserPointer() == id)
				{
					btVector3 p = start.lerp(end, res.m_hitFractions[i]);
					btVector3 n = btVector3(res.m_hitNormalWorld[i].m_floats[0], res.m_hitNormalWorld[i].m_floats[1], res.m_hitNormalWorld[i].m_floats[2]);
					raycastResult.hitPos = glm::vec3{ p.x(), p.y(), p.z() };
					raycastResult.hasHit = true;
					raycastResult.hitNorm = glm::vec3{ n.x(), n.y(), n.z() };
					raycastResult.id = (UINT)res.m_collisionObjects[i]->getUserPointer();
					break;
				}
			}
		}

		return raycastResult;
	}

	const std::vector<GrEngine::RayCastResult> BulletAPI::GetObjectContactPoints(GrEngine::PhysicsObject* object, float radius)
	{
		BulletPhysObject* btObject = static_cast<BulletPhysObject*>(object);
		std::vector<GrEngine::RayCastResult> out;
		CollisionTest callback(&out);
		dynamicsWorld->getCollisionWorld()->contactTest(btObject->body, callback);

		//BulletPhysObject* btObject = static_cast<BulletPhysObject*>(object);
		//std::vector<GrEngine::RayCastResult> out;
		//glm::vec3 orig1 = glm::vec3(btObject->body->getWorldTransform().getOrigin().x(), btObject->body->getWorldTransform().getOrigin().y(), btObject->body->getWorldTransform().getOrigin().z());

		//for (std::vector<GrEngine::PhysicsObject*>::iterator itt = objects.begin(); itt != objects.end(); ++itt)
		//{
		//	BulletPhysObject* objectSecond = static_cast<BulletPhysObject*>(*itt);
		//	glm::vec3 orig2 = glm::vec3(objectSecond->body->getWorldTransform().getOrigin().x(), objectSecond->body->getWorldTransform().getOrigin().y(), objectSecond->body->getWorldTransform().getOrigin().z());
		//	btVector3 min;
		//	btVector3 max;
		//	btTransform t;
		//	objectSecond->colShape->getAabb(t, min, max);
		//	if ((glm::distance(orig1, orig2) < radius && btObject != objectSecond ||
		//		(orig1.x < max.x() && orig1.x > min.x() && orig1.y < max.y() && orig1.y > min.y() && orig1.z < max.z() && orig1.z > min.z())) && btObject != objectSecond)
		//	{
		//		GrEngine::RayCastResult temp;
		//		CollisionTestSingle callback(temp);
		//		dynamicsWorld->getCollisionWorld()->contactPairTest(btObject->body, objectSecond->body, callback);
		//		out.push_back(temp);
		//	}
		//}

		//int numManifolds = dynamicsWorld->getDispatcher()->getNumManifolds();
		//for (int i = 0; i < numManifolds; i++)
		//{
		//	btPersistentManifold* contactManifold = dynamicsWorld->getDispatcher()->getManifoldByIndexInternal(i);
		//	const btCollisionObject* obA = static_cast<const btCollisionObject*>(contactManifold->getBody0());
		//	const btCollisionObject* obB = static_cast<const btCollisionObject*>(contactManifold->getBody1());
		//	if (obA == btObject->body)
		//	{
		//		int numContacts = contactManifold->getNumContacts();
		//		for (int j = 0; j < numContacts; j++)
		//		{
		//			btManifoldPoint& pt = contactManifold->getContactPoint(j);
		//			GrEngine::RayCastResult r{};
		//			r.hitPos = glm::vec3(pt.getPositionWorldOnB().x(), pt.getPositionWorldOnB().y(), pt.getPositionWorldOnB().z());
		//			r.hasHit = true;
		//			r.hitNorm = glm::vec3(pt.m_normalWorldOnB.x(), pt.m_normalWorldOnB.y(), pt.m_normalWorldOnB.z());
		//			r.distance = pt.getDistance();
		//			out.push_back(r);
		//		}
		//	}
		//}

		return out;
	}
};