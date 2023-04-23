#pragma once
#include <btBulletDynamicsCommon.h>
#include "BulletCollision/CollisionDispatch/btCollisionWorld.h"
#include "BulletDynamics/Character/btKinematicCharacterController.h"
#include "BulletCollision/NarrowPhaseCollision/btRaycastCallback.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"
#include "BulletCollision/CollisionShapes/btShapeHull.h"
#include "Engine/Headers/Virtual/ResourceManager.h"

namespace GrEngineBullet
{
	struct CollisionMesh
	{
		std::string path;
		btCollisionShape* shape;
	};

	class BulletResourceManager : public GrEngine::ResourceManager
	{
	public:
		BulletResourceManager(btDiscreteDynamicsWorld* world)
		{
			dynamicsWorld = world;
		}

		~BulletResourceManager()
		{
			colResources.clear();
		};

		Resource<CollisionMesh*>* AddCollisionMeshResource(const char* name, CollisionMesh* pointer);

		Resource<CollisionMesh*>* GetCollisionMeshResource(const char* name);

		void RemoveCollisionMeshResource(const char* name);
		void Clean();

	private:
		std::vector<Resource<CollisionMesh*>*> colResources;
		btDiscreteDynamicsWorld* dynamicsWorld;
	};
}
