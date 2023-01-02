#pragma once
#include <pch.h>
#include "Virtual/Entity.h"
#include "Virtual/Physics.h"

namespace GrEngine
{
	struct Texture
	{
		const char* texture_path;
		bool initialized = false;
	};

	struct Vertex
	{
		glm::vec4 pos;
		glm::vec4 color;
		glm::vec2 uv;
		uint32_t uv_index;
		uint32_t uses_texture;
		glm::uvec3 inID;

		Vertex(glm::vec4 position, glm::vec2 uv_coordinates, glm::uvec3 color_attach = {0, 0, 0}, uint32_t material_index = 0, BOOL use_texture = 0)
		{
			pos = position;
			uv = uv_coordinates;
			uv_index = material_index;
			uses_texture = use_texture;
			inID = color_attach;
		}

		bool operator==(const Vertex& other) const
		{
			return pos == other.pos && uv == other.uv;
		}
	};

	struct Mesh
	{
		const char* mesh_path = "";
	};

	class DllExport DrawableObject : public Entity
	{
	public:
		DrawableObject()
		{
			recalculatePhysics();
		};

		virtual ~DrawableObject()
		{

		};

		virtual bool LoadMesh(const char* mesh_path, bool useTexturing, std::vector<std::string>* out_materials) = 0;


		glm::vec3& GetObjectBounds()
		{
			return bound;
		}

		void SetObjectBounds(glm::vec3 new_bounds)
		{
			bound = new_bounds;
		}

		bool& IsVisible()
		{
			return visibility;
		}

		void SetVisisibility(bool value)
		{
			visibility = value;
		}

		void AssignColorMask(glm::vec4 mask)
		{
			color_mask = mask;
		}

		void AssignColorMask(float r, float g, float b, float a)
		{
			color_mask = { r, g, b, a };
		}

		glm::vec4 GetColorMask()
		{
			return color_mask;
		}

		void SetMass(float new_mass)
		{
			mass = new_mass;
			recalculatePhysics();
		}

		glm::vec3& GetObjectPosition() override
		{
			if (Physics::GetContext()->GetSimulationState() && mass != 0.f)
			{
				auto phys_pos = body->getWorldTransform().getOrigin();
				object_position = glm::vec3(phys_pos.x(), phys_pos.y(), phys_pos.z());

				if (phys_pos.y() < -600)
				{
					mass = 0.f;
					Logger::Out("Object ID:%d fell out of level and was frozen at coordinates %f %f %f!", OutputColor::Red, OutputType::Error, id, phys_pos.x(), phys_pos.y(), phys_pos.z());
				}
				return object_position;
			}
			else
			{
				btTransform worldTransform;
				worldTransform.setIdentity();
				worldTransform.setOrigin(btVector3(object_origin.x, object_origin.y, object_origin.z));
				worldTransform.setRotation(btQuaternion(btVector3(1, 0, 0), M_PI));
				body->setWorldTransform(worldTransform);
				body->setCenterOfMassTransform(worldTransform);
				return object_origin;
			}
		}

		glm::quat& GetObjectOrientation() override
		{
			auto phys_pos = body->getWorldTransform().getRotation();
			auto ori = glm::quat(phys_pos.x(), phys_pos.y(), phys_pos.z(), phys_pos.w());
			return ori;
		}

		void PositionObjectAt(const glm::vec3& vector) override
		{
			object_origin = vector;
			object_position = vector;
			recalculatePhysics();
		}

		void PositionObjectAt(const float& x, const float& y, const float& z) override
		{
			object_origin = glm::vec3(x, y, z);
			object_position = glm::vec3(x, y, z);
			recalculatePhysics();
		}

		void SetObjectScale(glm::vec3 new_scale) override
		{
			scale = new_scale;
			recalculatePhysics();
		}

		void SetObjectScale(float scalex, float scaley, float scalez) override
		{
			colShape->setLocalScaling(btVector3(scalex, scaley, scalez));
			scale = { scalex, scaley, scalez };
			recalculatePhysics();
		}

	protected:
		glm::uvec3 colorID = { 0, 0, 0 };

		void recalculatePhysics()
		{
			if (body != nullptr)
			{
				Physics::GetContext()->RemoveObject(static_cast<void*>(body));
				delete body;
				delete myMotionState;
			}

			btTransform startTransform;
			startTransform.setIdentity();
			startTransform.setOrigin(btVector3(object_origin.x, object_origin.y, object_origin.z));
			startTransform.setRotation(btQuaternion(btVector3(1, 0, 0), M_PI));
			btVector3 localInertia{ 0,0,0 };

			if (mass != 0.f)
				colShape->calculateLocalInertia(mass, localInertia);
			colShape->setLocalScaling(btVector3(scale.x, -1 * scale.y, scale.z));

			myMotionState = new btDefaultMotionState(startTransform);
			btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);
			rbInfo.m_angularDamping = .2f;
			rbInfo.m_linearDamping = .2f;
			body = new btRigidBody(rbInfo);

			Physics::GetContext()->AddSimulationObject(static_cast<void*>(body));
		}

		virtual void updateCollisions() = 0;
		glm::vec3 bound = { 0.f, 0.f, 0.f };
		bool visibility = true;
		glm::vec4 color_mask;

		//btCollisionShape* colShape = new btSphereShape(0.25f);
		btCollisionShape* colShape = new btBoxShape(btVector3(0.25f, 0.25f, 0.25f));
		btScalar mass{ 0.f };

		btRigidBody* body;
		btDefaultMotionState* myMotionState;
		btTriangleMesh* colMesh;
	};
}
