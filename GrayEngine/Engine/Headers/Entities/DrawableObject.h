#pragma once
#include <pch.h>
#include "Entity.h"

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
			Type = "Object";
			recalculatePhysics();
		};

		DrawableObject(UINT id) : Entity(id)
		{
			Type = "Object";
			recalculatePhysics();
		};

		virtual ~DrawableObject()
		{
			if (body != nullptr)
			{
				Physics::GetContext()->RemoveObject(static_cast<void*>(body));
				delete body;
				body = nullptr;
				delete myMotionState;
				myMotionState = nullptr;
			}
		};

		virtual bool LoadMesh(const char* mesh_path, bool useTexturing, std::vector<std::string>* out_materials) = 0;
		virtual bool LoadModel(const char* model_path) = 0;
		virtual bool LoadModel(const char* mesh_path, std::vector<std::string> textures_vector) = 0;

		virtual glm::vec3 GetObjectPosition() override
		{
			if (Physics::GetContext()->GetSimulationState() && body != nullptr && body->getMass() > 0.f)
			{
				auto phys_pos = body->getWorldTransform().getOrigin();
				auto pos = glm::vec3(phys_pos.x(), phys_pos.y(), phys_pos.z());;
				return glm::vec3(phys_pos.x(), phys_pos.y(), phys_pos.z());
			}
			else
			{
				return *object_origin;
			}
		};

		virtual glm::quat GetObjectOrientation() override
		{
			if (Physics::GetContext()->GetSimulationState() && body != nullptr && body->getMass() > 0.f)
			{
				auto phys_pos = body->getWorldTransform().getRotation();
				auto ori = glm::quat(phys_pos.x(), phys_pos.y(), phys_pos.z(), phys_pos.w());
				return ori;
			}
			else
			{
				return *obj_orientation;
			}
		};

		void ParsePropertyValue(const char* property_name, const char* property_value) override
		{
			for (std::vector<EntityProperty*>::iterator itt = properties.begin(); itt != properties.end(); ++itt)
			{
				if ((*itt)->property_name == std::string(property_name))
				{
					(*itt)->ParsePropertyValue(property_value);
					recalculatePhysics();
				}
			}
		}


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

		std::unordered_map<std::string, std::string> GetMaterials()
		{
			std::unordered_map<std::string, std::string> res;

			for (int i = 0; i < material_names.size(); i++)
			{
				if (i < texture_names.size())
					res.insert_or_assign(material_names[i], texture_names[i]);
				else
					res.insert_or_assign(material_names[i], "nil");
			}

			return res;
		}

		void recalculatePhysics()
		{
			if (!CollisionEnabled) return;

			float obj_mass = GetPropertyValue<float>("Mass", 0.f);

			if (body != nullptr)
			{
				Physics::GetContext()->RemoveObject(body);
				delete body;
				body = nullptr;
				delete myMotionState;
				myMotionState = nullptr;
			}

			btTransform startTransform;
			auto trans = glm::translate(glm::mat4(1.f), GetObjectPosition()) * glm::mat4_cast(GetObjectOrientation());
			const float* pSource = (const float*)glm::value_ptr(trans);
			startTransform.setFromOpenGLMatrix(pSource);
			btVector3 localInertia{ 0,0,0 };

			if (obj_mass != 0.f)
				colShape->calculateLocalInertia(obj_mass, localInertia);

			glm::vec3 scale = GetPropertyValue("Scale", glm::vec3(1.f));
			colShape->setLocalScaling(btVector3(scale.x + 0.1, scale.y + 0.1, scale.z + 0.1));

			myMotionState = new btDefaultMotionState(startTransform);
			btRigidBody::btRigidBodyConstructionInfo rbInfo(obj_mass, myMotionState, colShape, localInertia);
			rbInfo.m_angularDamping = .2f;
			rbInfo.m_linearDamping = .2f;
			body = new btRigidBody(rbInfo);

			Physics::GetContext()->AddSimulationObject(body);
		};

		void DisableCollisions()
		{
			CollisionEnabled = false;

			if (body != nullptr)
			{
				Physics::GetContext()->RemoveObject(body);
				delete body;
				body = nullptr;
				delete myMotionState;
				myMotionState = nullptr;
			}
		}

		std::vector<std::string> material_names;
		std::vector<std::string> texture_names;
	protected:
		virtual void updateCollisions() = 0;
		glm::vec3 bound = { 0.f, 0.f, 0.f };
		bool visibility = true;

		btCollisionShape* colShape = new btBoxShape(btVector3(0.25f, 0.25f, 0.25f));
		btRigidBody* body;
		btDefaultMotionState* myMotionState;
		btTriangleMesh* colMesh;
		bool CollisionEnabled = true;
	};
}
