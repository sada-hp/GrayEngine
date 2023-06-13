#pragma once
#include <btBulletDynamicsCommon.h>
#include "BulletCollision/CollisionDispatch/btCollisionWorld.h"
#include "BulletDynamics/Character/btKinematicCharacterController.h"
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
		BulletPhysObject(GrEngine::Entity* owner, GrEngine::Physics* context, BulletResourceManager* manager, btDiscreteDynamicsWorld* world) : GrEngine::PhysicsObject(owner, context)
		{
			resources = manager;
			dynamicWorld = world;
			flags = btCollisionObject::CF_DYNAMIC_OBJECT;
			time = std::chrono::steady_clock::now();

			UpdateCollisionType(owner->GetPropertyValue(PropertyType::CollisionType, 0));
			SetKinematic(owner->GetPropertyValue(PropertyType::BodyType, 0));
		}

		~BulletPhysObject()
		{
			Dispose();

			if (objCollision != nullptr)
			{
				resources->RemoveCollisionMeshResource(objCollision);
				objCollision = nullptr;
			}
		}

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
			}

			initialized = false;
		}

		inline bool IsOwnerStatic()
		{
			return pOwner->IsStatic();
		}

		UINT GetID() override
		{
			return pOwner->GetEntityID();
		}

		void AddCollisionResource(const char* name, void* data) override
		{
			auto resource = resources->GetCollisionMeshResource(name, colType);

			if (resource == nullptr)
			{
				CollisionMesh* cmesh = new CollisionMesh();
				cmesh->shape = (btCollisionShape*)data;
				cmesh->path = name;
				objCollision = resources->AddCollisionMeshResource(name, cmesh)->AddLink();
			}
		}

		void GenerateBoxCollision(float width, float height, float length) override
		{
			glm::vec3 scale = pOwner->GetPropertyValue(PropertyType::Scale, glm::vec3(1.0f));
			auto resource = resources->GetCollisionMeshResource(("box" + GrEngine::Globals::FloatToString(width * scale.x, 5) + GrEngine::Globals::FloatToString(height * scale.y, 5) + GrEngine::Globals::FloatToString(length * scale.z, 5)).c_str(), colType);
			if (resource != nullptr)
			{
				objCollision = resource->AddLink();
				shape_name = "box" + GrEngine::Globals::FloatToString(width * scale.x, 5) + GrEngine::Globals::FloatToString(height * scale.y, 5) + GrEngine::Globals::FloatToString(length * scale.z, 5);
			}
			else
			{
				CollisionMesh* cmesh = new CollisionMesh();
				cmesh->shape = new btBoxShape(btVector3(width, height, length));
				auto collision_mesh = resources->AddCollisionMeshResource(("box" + GrEngine::Globals::FloatToString(width * scale.x, 5) + GrEngine::Globals::FloatToString(height * scale.y, 5) + GrEngine::Globals::FloatToString(length * scale.z, 5)).c_str(), cmesh);
				objCollision = collision_mesh->AddLink();
				shape_name = collision_mesh->name;
			}
		}

		void GenerateCapsuleCollision(float radius, float height)
		{
			auto resource = resources->GetCollisionMeshResource(("capsule" + GrEngine::Globals::FloatToString(radius, 5) + GrEngine::Globals::FloatToString(height, 5)).c_str(), colType);
			if (resource != nullptr)
			{
				objCollision = resource->AddLink();
				shape_name = "capsule" + GrEngine::Globals::FloatToString(radius, 5) + GrEngine::Globals::FloatToString(height, 5);
			}
			else
			{
				CollisionMesh* cmesh = new CollisionMesh();
				cmesh->shape = new btCapsuleShape(radius, height);
				auto collision_mesh = resources->AddCollisionMeshResource(("capsule" + GrEngine::Globals::FloatToString(radius, 5) + GrEngine::Globals::FloatToString(height, 5)).c_str(), cmesh);
				objCollision = collision_mesh->AddLink();
				shape_name = "capsule" + GrEngine::Globals::FloatToString(radius, 5) + GrEngine::Globals::FloatToString(height, 5);
			}
		}

		bool LoadCollisionMesh(const char* mesh_path, bool use_hull = true) override
		{
			if (mesh_path == NULL || mesh_path == "")
			{
				GenerateBoxCollision(1, 1, 1);
				return false;
			}

			glm::vec3 scale = pOwner->GetPropertyValue(PropertyType::Scale, glm::vec3(1.0f));
			std::string res_postfix = "";
			if (scale != glm::vec3(1.f))
			{
				res_postfix = GrEngine::Globals::FloatToString(scale.x, 5) + GrEngine::Globals::FloatToString(scale.y, 5) + GrEngine::Globals::FloatToString(scale.z, 5);
			}
			auto resource = resources->GetCollisionMeshResource((std::string(mesh_path) + res_postfix).c_str(), colType);
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
					Logger::Out("Could not load the mesh %c%s%c!",OutputType::Error, '"', mesh_path, '"');
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

						if (indices.size() % 3 == 0 && indices.size() > 0)
						{
							colMesh->addTriangleIndices(indices[indices.size() - 1], indices[indices.size() - 2], indices[indices.size() - 3]);
						}
					}
				}

				CollisionMesh* cmesh = new CollisionMesh();
				if (use_hull)
				{
					btConvexTriangleMeshShape* tmpshape = new btConvexTriangleMeshShape(colMesh);
					tmpshape->setMargin(0.001f);
					btShapeHull* hull = new btShapeHull(tmpshape);
					hull->buildHull(0);
					cmesh->path = mesh_path;
					cmesh->shape = new btConvexHullShape((const btScalar*)hull->getVertexPointer(), hull->numVertices(), sizeof(btVector3));
					auto collision_mesh = resources->AddCollisionMeshResource(("Hull_" + std::string(mesh_path) + res_postfix).c_str(), cmesh);
					objCollision = collision_mesh->AddLink();
					delete hull;
					delete tmpshape;
					delete colMesh;
				}
				else
				{
					cmesh->path = mesh_path;
					cmesh->shape = new btBvhTriangleMeshShape(colMesh, true);
					auto collision_mesh = resources->AddCollisionMeshResource(("Mesh_" + std::string(mesh_path) + res_postfix).c_str(), cmesh);
					objCollision = collision_mesh->AddLink();
				}
			}
			else
			{
				objCollision = resource->AddLink();
			}

			shape_name = use_hull ? "Hull_" + std::string(mesh_path) : "Mesh_" + std::string(mesh_path);
			return true;
		}

		void UpdateCollisionType(int value) override
		{
			std::string coll_path = "";
			std::string model_path = "";

			if (objCollision != nullptr)
			{
				resources->RemoveCollisionMeshResource(objCollision);
				objCollision = nullptr;
			}

			colType = (CollisionType)value;

			switch (value)
			{
			case CollisionType::Box:
				GenerateBoxCollision(0.25f, 0.25f, 0.25f);
				break;
			case CollisionType::Sphere:
				break;
			case CollisionType::ConvexHullMesh:
				model_path = pOwner->GetPropertyValue(PropertyType::ModelPath, coll_path);
				GrEngine::Globals::readGMF(model_path, nullptr, &coll_path, nullptr, nullptr);
				LoadCollisionMesh(coll_path.c_str(), true);
				break;
			case CollisionType::Mesh:
				model_path = pOwner->GetPropertyValue(PropertyType::ModelPath, coll_path);
				GrEngine::Globals::readGMF(model_path, nullptr, &coll_path, nullptr, nullptr);
				LoadCollisionMesh(coll_path.c_str(), false);
				break;
			case CollisionType::Capsule:
				GenerateCapsuleCollision(1, 2);
				break;
			}

			Dispose();
			CalculatePhysics();
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

			Dispose();
			CalculatePhysics();
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

			body->activate();
		}
	
		btCollisionShape* getShape()
		{
			return objCollision->shape;
		}

		btVector3 getVelocityMultiplier()
		{
			return { vAxes.x, vAxes.y, vAxes.z};
		}

	private:
		bool initPhysics()
		{
			if (myMotionState != nullptr)
			{
				delete myMotionState;
				myMotionState = nullptr;
			}

			initialized = false;

			float obj_mass = pOwner->GetPropertyValue(PropertyType::Mass, 0.f);
			btTransform startTransform;
			glm::mat4 transformation = pOwner->GetObjectTransformation();
			const float* pSource = (const float*)glm::value_ptr(transformation);
			startTransform.setFromOpenGLMatrix(pSource);
			btVector3 localInertia{ 0,0,0 };

			if (objCollision != nullptr)
			{
				glm::vec3 scale = pOwner->GetPropertyValue(PropertyType::Scale, glm::vec3(1.0f));
				objCollision->shape->setLocalScaling(btVector3(scale.x, scale.y, scale.z));

				if (obj_mass != 0.f && (flags & btCollisionObject::CF_STATIC_OBJECT) == 0)
					objCollision->shape->calculateLocalInertia(obj_mass, localInertia);
				else
					obj_mass = 0.f;
			}
			else
			{
				UpdateCollisionType(colType);
			}

			if (body == nullptr)
			{
				if (flags == btCollisionObject::CF_DYNAMIC_OBJECT)
				{
					myMotionState = new btDefaultMotionState(startTransform);
					btRigidBody::btRigidBodyConstructionInfo rbInfo(obj_mass, myMotionState, objCollision->shape, localInertia);
					body = new btRigidBody(rbInfo);
					body->setActivationState(ACTIVE_TAG);
					body->setDeactivationTime(1.f);
					static_cast<btRigidBody*>(body)->setMassProps(obj_mass, localInertia);
					static_cast<btRigidBody*>(body)->setLinearFactor(getVelocityMultiplier());
					dynamicWorld->addRigidBody((btRigidBody*)body);
				}
				else if (flags == btCollisionObject::CF_NO_CONTACT_RESPONSE)
				{
					body = new btGhostObject();
					dynamicWorld->addCollisionObject((btRigidBody*)body);
				}
				else if (flags == btCollisionObject::CF_KINEMATIC_OBJECT)
				{
					static_cast<btCapsuleShape*>(objCollision->shape)->setMargin(0.1f);
					static_cast<btCapsuleShape*>(objCollision->shape)->setSafeMargin(0.5f);
					body = new btPairCachingGhostObject();
					static_cast<btPairCachingGhostObject*>(body)->setCollisionShape(objCollision->shape);
					static_cast<btPairCachingGhostObject*>(body)->setWorldTransform(startTransform);
					dynamicWorld->addCollisionObject(body, btBroadphaseProxy::CharacterFilter, btBroadphaseProxy::StaticFilter | btBroadphaseProxy::DefaultFilter);
				}
				else
				{
					myMotionState = new btDefaultMotionState(startTransform);
					btRigidBody::btRigidBodyConstructionInfo rbInfo(obj_mass, myMotionState, objCollision->shape, localInertia);
					body = new btRigidBody(rbInfo);
					dynamicWorld->addCollisionObject(body,  btBroadphaseProxy::StaticFilter, btBroadphaseProxy::CharacterFilter | btBroadphaseProxy::DefaultFilter);
				}
				body->setCollisionFlags(flags);
			}
			else
			{
				body->setCollisionShape(objCollision->shape);
				if (flags == 0)
				{
					myMotionState = new btDefaultMotionState(startTransform);
					static_cast<btRigidBody*>(body)->setMassProps(obj_mass, localInertia);
					static_cast<btRigidBody*>(body)->setMotionState(myMotionState);
					body->setActivationState(ACTIVE_TAG);
					body->setDeactivationTime(1.f);
				}
				else if (flags == btCollisionObject::CF_KINEMATIC_OBJECT)
				{
					static_cast<btPairCachingGhostObject*>(body)->setActivationState(DISABLE_DEACTIVATION);
					static_cast<btPairCachingGhostObject*>(body)->setWorldTransform(startTransform);
				}
				else if (flags != btCollisionObject::CF_NO_CONTACT_RESPONSE)
				{
					myMotionState = new btDefaultMotionState(startTransform);
					static_cast<btRigidBody*>(body)->setMotionState(myMotionState);
				}

				body->setCollisionFlags(flags);
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
		CollisionType colType = (CollisionType)0;
	};
}