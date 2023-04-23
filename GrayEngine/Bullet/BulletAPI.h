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
#include "Virtual/PhysicsObject.h"
#include "Engine/Headers/Core/Globals.h"
#include "Engine/Headers/Core/Logger.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

template<> struct std::hash<btVector3> {
	size_t operator()(btVector3 const& vertex) const {
		return ((std::hash<btScalar>()(vertex.x())) ^ (std::hash<btScalar>()(vertex.y())) ^ (std::hash<btScalar>()(vertex.z())) >> 1);
	}
};

namespace GrEngineBullet
{
	class BulletPhysObject : public GrEngine::PhysicsObject
	{
		float timer = 0.f;
		std::chrono::steady_clock::time_point time;

	public:
		BulletPhysObject(GrEngine::Entity* owner, BulletResourceManager* manager, btDiscreteDynamicsWorld* world) : GrEngine::PhysicsObject(owner)
		{
			resources = manager;
			dynamicWorld = world;
			UpdateCollisionType((CollisionTypeEnum)pOwner->GetPropertyValue(PropertyType::CollisionType, 0));
			time = std::chrono::steady_clock::now();
		}

		~BulletPhysObject()
		{
			Dispose();

			if (colShape != nullptr)
			{
				resources->RemoveCollisionMeshResource(shape_name.c_str());
			}

			colShape = nullptr;
		}

		btCollisionShape* colShape;
		btCollisionObject* body;
		btDefaultMotionState* myMotionState;

		bool CalculatePhysics() override
		{
			return initPhysics();
		}

		void DisablePhysics() override
		{
			CollisionEnabled = false;
			SetKinematic(3);
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

		void MoveObject(glm::vec3 vector) override
		{
			auto now = std::chrono::steady_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - time).count();
			if (duration/1000.f > timer)
			{
				if (flags == 0)
				{
					btTransform trans;
					static_cast<btRigidBody*>(body)->getMotionState()->getWorldTransform(trans);
					trans.setOrigin(trans.getOrigin() + btVector3(vector.x, vector.y, vector.z) * GrEngine::Globals::delta_time);
					static_cast<btRigidBody*>(body)->getMotionState()->setWorldTransform(trans);
				}
				else if (flags == btCollisionObject::CF_KINEMATIC_OBJECT && controls != nullptr)
				{
					controls->setWalkDirection(btVector3(vector.x, vector.y, vector.z));
				}
			}
		}

		void SlideObjectForDuration(glm::vec3 vector, float dur) override
		{
			if (flags == btCollisionObject::CF_KINEMATIC_OBJECT && controls != nullptr)
			{
				controls->setVelocityForTimeInterval(btVector3(vector.x, vector.y, vector.z), dur);
				timer = dur;
				time = std::chrono::steady_clock::now();
			}
		}

		void Dispose() override
		{
			if (initialized)
			{
				if (body != nullptr)
				{
					if (body->getCollisionFlags() == 0)
					{
						dynamicWorld->removeRigidBody((btRigidBody*)body);
					}
					else if (body->getCollisionFlags() == btCollisionObject::CF_KINEMATIC_OBJECT || body->getCollisionFlags() == btCollisionObject::CF_CHARACTER_OBJECT)
					{
						dynamicWorld->removeCollisionObject(body);
						dynamicWorld->removeAction(controls);
					}
					else
					{
						dynamicWorld->removeCollisionObject(body);
					}

					delete body;
					body = nullptr;
				}
				if (myMotionState != nullptr)
				{
					delete myMotionState;
					myMotionState = nullptr;
				}
				if (controls != nullptr)
				{
					delete controls;
					controls = nullptr;
				}

				//if (colShape != nullptr)
				//{
				//	resources->RemoveCollisionMeshResource(shape_name.c_str());
				//	colShape = nullptr;
				//}
			}

			initialized = false;
		}

		inline bool IsOwnerStatic()
		{
			return pOwner->IsStatic();
		}

		void UpdateCollisionShape(void* shape) override
		{
			if (colShape != nullptr && colShape != shape)
			{
				resources->RemoveCollisionMeshResource(shape_name.c_str());
			}

			colShape = static_cast<btCollisionShape*>(shape);
			CalculatePhysics();
		}

		UINT GetID() override
		{
			return pOwner->GetEntityID();
		}

		void GenerateBoxCollision(float width, float height, float length) override
		{
			auto resource = resources->GetCollisionMeshResource(("box" + GrEngine::Globals::FloatToString(width, 5) + GrEngine::Globals::FloatToString(height, 5) + GrEngine::Globals::FloatToString(length, 5)).c_str());
			if (resource != nullptr)
			{
				CollisionMesh* cmesh = resource->AddLink();
				UpdateCollisionShape(cmesh->shape);
				shape_name = "box" + GrEngine::Globals::FloatToString(width, 5) + GrEngine::Globals::FloatToString(height, 5) + GrEngine::Globals::FloatToString(length, 5);
			}
			else
			{
				CollisionMesh* cmesh = new CollisionMesh();
				cmesh->shape = new btBoxShape(btVector3(width, height, length));
				auto collision_mesh = resources->AddCollisionMeshResource(("box" + GrEngine::Globals::FloatToString(width, 5) + GrEngine::Globals::FloatToString(height, 5) + GrEngine::Globals::FloatToString(length, 5)).c_str(), cmesh);
				collision_mesh->AddLink();
				UpdateCollisionShape(cmesh->shape);
				shape_name = "box" + GrEngine::Globals::FloatToString(width, 5) + GrEngine::Globals::FloatToString(height, 5) + GrEngine::Globals::FloatToString(length, 5);
			}
		}

		void GenerateCapsuleCollision(float radius, float height)
		{
			auto resource = resources->GetCollisionMeshResource(("capsule" + GrEngine::Globals::FloatToString(radius, 5) + GrEngine::Globals::FloatToString(height, 5)).c_str());
			if (resource != nullptr)
			{
				CollisionMesh* cmesh = resource->AddLink();
				UpdateCollisionShape(cmesh->shape);
				shape_name = "capsule" + GrEngine::Globals::FloatToString(radius, 5) + GrEngine::Globals::FloatToString(height, 5);
			}
			else
			{
				CollisionMesh* cmesh = new CollisionMesh();
				cmesh->shape = new btCapsuleShape(radius, height);
				auto collision_mesh = resources->AddCollisionMeshResource(("capsule" + GrEngine::Globals::FloatToString(radius, 5) + GrEngine::Globals::FloatToString(height, 5)).c_str(), cmesh);
				collision_mesh->AddLink();
				UpdateCollisionShape(cmesh->shape);
				shape_name = "capsule" + GrEngine::Globals::FloatToString(radius, 5) + GrEngine::Globals::FloatToString(height, 5);
			}
		}

		bool LoadCollisionMesh(const char* mesh_path) override
		{
			if (mesh_path == NULL || mesh_path == "")
			{
				GenerateBoxCollision(1, 1, 1);
				return false;
			}

			auto resource = resources->GetCollisionMeshResource(mesh_path);
			std::string solution = GrEngine::Globals::getExecutablePath();
			std::string collision_path = mesh_path;

			if (resource == nullptr)
			{
				btTriangleMesh* colMesh = new btTriangleMesh();

				std::vector<btVector3> hullVert{};
				std::unordered_map<btVector3, uint32_t> uniqueVertices{};
				Assimp::Importer importer;

				const aiScene* collision;

				if (collision_path.size() >= solution.size() && collision_path.substr(0, solution.size()) == solution)
				{
					collision = importer.ReadFile(mesh_path, 0);
				}
				else
				{
					collision = importer.ReadFile(solution + mesh_path, 0);
				}

				if (collision == NULL)
				{
					Logger::Out("Could not load the mesh %c%s%c!", OutputColor::Red, OutputType::Error, '"', mesh_path, '"');
					return false;
				}

				std::vector<uint32_t> indices;

				for (int mesh_ind = 0; mesh_ind < collision->mNumMeshes; mesh_ind++)
				{
					auto num_vert = collision->mMeshes[mesh_ind]->mNumVertices;
					auto cur_mesh = collision->mMeshes[mesh_ind];
					auto name3 = collision->mMeshes[mesh_ind]->mName;
					auto uv_ind = mesh_ind;

					for (int vert_ind = 0; vert_ind < num_vert; vert_ind++)
					{
						auto coord = collision->mMeshes[mesh_ind]->mTextureCoords[0];

						btVector3 vertex{};
						vertex = { cur_mesh->mVertices[vert_ind].x, cur_mesh->mVertices[vert_ind].y, cur_mesh->mVertices[vert_ind].z };

						if (uniqueVertices.count(vertex) == 0)
						{
							uniqueVertices[vertex] = static_cast<uint32_t>(hullVert.size());
							hullVert.push_back(vertex);
							colMesh->findOrAddVertex(vertex, false);
						}

						int index = uniqueVertices[vertex];
						indices.push_back(index);
					}
				}

				for (std::vector<uint32_t>::iterator itt = indices.begin(); itt != indices.end(); ++itt)
				{
					colMesh->addTriangleIndices(*(++itt), *(++itt), *itt);
				}

				CollisionMesh* cmesh = new CollisionMesh();
				btConvexShape* tmpshape = new btConvexTriangleMeshShape(colMesh);
				tmpshape->setMargin(0.001f);
				btShapeHull* hull = new btShapeHull(tmpshape);
				hull->buildHull(0);
				cmesh->path = mesh_path;
				cmesh->shape = new btConvexHullShape((const btScalar*)hull->getVertexPointer(), hull->numVertices(), sizeof(btVector3));
				auto collision_mesh = resources->AddCollisionMeshResource(mesh_path, cmesh);
				objCollision = collision_mesh->AddLink();
				delete hull;
				delete tmpshape;
				delete colMesh;

				UpdateCollisionShape(objCollision->shape);
			}
			else if (!resource->Compare(objCollision))
			{
				objCollision = resource->AddLink();
				UpdateCollisionShape(objCollision->shape);
			}
			else
			{
				UpdateCollisionShape(objCollision->shape);
			}

			shape_name = mesh_path;
			pOwner->collision_path = mesh_path;
			return true;
		}

		void UpdateCollisionType(CollisionTypeEnum value) override
		{
			switch (value)
			{
			case CollisionTypeEnum::Box:
				GenerateBoxCollision(0.25f, 0.25f, 0.25f);
				break;
			case CollisionTypeEnum::Sphere:
				break;
			case CollisionTypeEnum::ConvexHullMesh:
				LoadCollisionMesh(pOwner->collision_path.c_str());
				break;
			case CollisionTypeEnum::Mesh:
				LoadCollisionMesh(pOwner->collision_path.c_str());
				break;
			case CollisionTypeEnum::Capsule:
				GenerateCapsuleCollision(1, 2);
				break;
			}
		}

		void SetKinematic(int value)
		{
			if (value == 0)
			{
				flags = btCollisionObject::CF_DYNAMIC_OBJECT;
			}
			else if (value == 1)
			{
				flags = btCollisionObject::CF_KINEMATIC_OBJECT;
			}
			else if (value == 2)
			{
				flags = btCollisionObject::CF_STATIC_OBJECT;
			}
			else
			{
				flags = btCollisionObject::CF_NO_CONTACT_RESPONSE;
			}

			if (body != nullptr)
			{
				CalculatePhysics();

				if ((flags & btCollisionObject::CF_CHARACTER_OBJECT) != 0)
				{
					body->setActivationState(DISABLE_DEACTIVATION);
				}
				else
				{
					body->setActivationState(ACTIVE_TAG);
					body->setDeactivationTime(1.f);
				}
			}
		}

		const int getFlags()
		{
			return flags;
		}

		void ResetMotion()
		{
			if (flags == 0)
			{
				static_cast<btRigidBody*>(body)->setLinearVelocity(btVector3(0, 0, 0));
				static_cast<btRigidBody*>(body)->setAngularVelocity(btVector3(0, 0, 0));
			}
			else if (flags == btCollisionObject::CF_KINEMATIC_OBJECT || flags == btCollisionObject::CF_CHARACTER_OBJECT)
			{
				controls->setLinearVelocity(btVector3(0, 0, 0));
				controls->setAngularVelocity(btVector3(0, 0, 0));
			}

			body->activate();
		}

		btKinematicCharacterController* controls;

	private:
		bool initPhysics()
		{
			if (myMotionState != nullptr)
			{
				delete myMotionState;
				myMotionState = nullptr;
			}

			if (body != nullptr && body->getCollisionFlags() != flags)
			{
				Dispose();
			}

			initialized = false;

			float obj_mass = pOwner->GetPropertyValue(PropertyType::Mass, 0.f);
			btTransform startTransform;
			glm::mat4 transformation = pOwner->GetObjectTransformation();
			const float* pSource = (const float*)glm::value_ptr(transformation);
			startTransform.setFromOpenGLMatrix(pSource);
			btVector3 localInertia{ 0,0,0 };

			if (colShape != nullptr)
			{
				if (colShape->getShapeType() == 21 && was_updated)
				{
					static_cast<btBvhTriangleMeshShape*>(colShape)->buildOptimizedBvh();
					was_updated = false;
				}

				if (obj_mass != 0.f && (flags & btCollisionObject::CF_STATIC_OBJECT) == 0)
					colShape->calculateLocalInertia(obj_mass, localInertia);
				else
					obj_mass = 0.f;

				glm::vec3 scale = pOwner->GetPropertyValue(PropertyType::Scale, glm::vec3(1.0f));
				colShape->setLocalScaling(btVector3(scale.x + 0.00001f, scale.y + 0.00001f, scale.z + 0.00001f));
			}


			if (body == nullptr)
			{
				if (flags == btCollisionObject::CF_DYNAMIC_OBJECT)
				{
					myMotionState = new btDefaultMotionState(startTransform);
					btRigidBody::btRigidBodyConstructionInfo rbInfo(obj_mass, myMotionState, colShape, localInertia);
					body = new btRigidBody(rbInfo);
					body->setActivationState(ACTIVE_TAG);
					body->setDeactivationTime(1.f);
					dynamicWorld->addRigidBody((btRigidBody*)body);
				}
				else if (flags == btCollisionObject::CF_NO_CONTACT_RESPONSE)
				{
					body = new btGhostObject();
					dynamicWorld->addCollisionObject((btRigidBody*)body);
				}
				else if (flags == btCollisionObject::CF_KINEMATIC_OBJECT)
				{
					static_cast<btCapsuleShape*>(colShape)->setMargin(0.1f);
					static_cast<btCapsuleShape*>(colShape)->setSafeMargin(0.5f);
					body = new btPairCachingGhostObject();
					static_cast<btPairCachingGhostObject*>(body)->setCollisionShape(colShape);
					controls = new btKinematicCharacterController(static_cast<btPairCachingGhostObject*>(body), static_cast<btConvexShape*>(colShape), 0.35f, btVector3(0, 1, 0));
					controls->setUseGhostSweepTest(false);
					controls->setAngularDamping(100.f);
					controls->setMaxSlope(glm::radians(89.f));
					controls->setMaxPenetrationDepth(0.01f);
					static_cast<btPairCachingGhostObject*>(body)->setWorldTransform(startTransform);
					dynamicWorld->addCollisionObject(body, btBroadphaseProxy::CharacterFilter, btBroadphaseProxy::StaticFilter | btBroadphaseProxy::DefaultFilter);
					dynamicWorld->addAction(controls);
				}
				else
				{
					myMotionState = new btDefaultMotionState(startTransform);
					btRigidBody::btRigidBodyConstructionInfo rbInfo(obj_mass, myMotionState, colShape, localInertia);
					body = new btRigidBody(rbInfo);
					dynamicWorld->addCollisionObject(body, flags);
				}
				body->setCollisionFlags(flags);
			}
			else
			{
				body->setCollisionShape(colShape);
				if (flags == 0)
				{
					myMotionState = new btDefaultMotionState(startTransform);
					static_cast<btRigidBody*>(body)->setCollisionShape(colShape);
					static_cast<btRigidBody*>(body)->setMassProps(obj_mass, localInertia);
					static_cast<btRigidBody*>(body)->setMotionState(myMotionState);
				}
				else if (flags == btCollisionObject::CF_KINEMATIC_OBJECT)
				{
					//static_cast<btPairCachingGhostObject*>(body)->setActivationState(DISABLE_DEACTIVATION);
					static_cast<btPairCachingGhostObject*>(body)->setCollisionShape(colShape);
					static_cast<btPairCachingGhostObject*>(body)->setWorldTransform(startTransform);
				}
				else if (flags == btCollisionObject::CF_NO_CONTACT_RESPONSE)
				{
					static_cast<btGhostObject*>(body)->setCollisionShape(colShape);
				}
				else
				{
					myMotionState = new btDefaultMotionState(startTransform);
					static_cast<btRigidBody*>(body)->setCollisionShape(colShape);
					static_cast<btRigidBody*>(body)->setMotionState(myMotionState);
				}
			}

			body->setUserPointer((void*)pOwner->GetEntityID());

			initialized = true;

			return true;
		}

		int flags = 0;
		BulletResourceManager* resources;
		CollisionMesh* objCollision;
		btDiscreteDynamicsWorld* dynamicWorld;
		std::string shape_name;
		std::string mesh_name;
	};

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
		void AddSimulationObject(GrEngine::PhysicsObject* object) override;
		void RemoveSimulationObject(GrEngine::PhysicsObject* object) override;
		void RemoveSimulationObject(UINT id) override;
		void RemovePhysicsObject(void* object) override;
		void TogglePhysicsState(bool state) override;
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
};