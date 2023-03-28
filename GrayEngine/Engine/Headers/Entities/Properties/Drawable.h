#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Entities/Entity.h"
#include "Engine/Headers/Virtual/Physics.h"

namespace GrEngine
{
	class Object
	{
	public:

		Object(Entity* owner)
		{
			ownerEntity = owner;
		};

		virtual ~Object()
		{
			if (physComponent != nullptr)
			{
				delete physComponent;
			}
		};

		virtual bool LoadMesh(const char* mesh_path, std::vector<std::string>* out_materials) = 0;

		virtual bool LoadModel(const char* model_path) = 0;

		virtual bool LoadModel(const char* mesh_path, std::vector<std::string> textures_vector) = 0;

		virtual void GeneratePlaneMesh(float width, int subdivisions) = 0;

		virtual void GenerateBoxMesh(float width, float height, float depth) = 0;

		virtual void Refresh() = 0;

		virtual void CalculateNormals() = 0;

		virtual glm::vec3 GetObjectPosition()
		{
			if (physComponent->HasValue())
			{
				auto pos = physComponent->GetPhysPosition();
				return pos;
			}
			else
			{
				return ownerEntity->GetObjectPosition();
			}
		};

		virtual glm::quat GetObjectOrientation()
		{
			if (physComponent->HasValue())
			{
				auto ori = physComponent->GetPhysOrientation();
				return ori;
			}
			else
			{
				return ownerEntity->GetObjectOrientation();
			}
		};

		virtual glm::mat4 GetObjectTransformation()
		{
			return glm::translate(glm::mat4(1.f), GetObjectPosition()) * glm::mat4_cast(GetObjectOrientation());
		};

		virtual glm::uvec3& GetObjectBounds()
		{
			return bound;
		};

		virtual void SetObjectBounds(glm::uvec3 new_bounds)
		{
			bound = new_bounds;
		};

		bool& IsVisible()
		{
			return visibility;
		};

		void SetVisisibility(bool value)
		{
			visibility = value;
		};

		const EntityType& GetEntityType()
		{
			return ownerEntity->GetEntityType();
		};

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
		};

		void recalculatePhysics(bool enabled)
		{
			physComponent->CalculatePhysics();
		};

		void DisableCollisions()
		{
			CollisionEnabled = false;
			ownerEntity->ParsePropertyValue("Mass", "0");

			if (physComponent != nullptr)
			{
				physComponent->DisablePhysics();
			}
		};

		Entity* GetOwnerEntity()
		{
			return ownerEntity;
		};

		std::vector<std::string> material_names;
		std::vector<std::string> texture_names;

	protected:

		Entity* ownerEntity = nullptr;
		Physics::PhysicsObject* physComponent = nullptr;
		virtual void updateCollisions() = 0;
		glm::uvec3 bound = { 0.f, 0.f, 0.f };
		bool visibility = true;
		bool CollisionEnabled = true;
	};
};

inline GrEngine::Object* GGetMesh(GrEngine::Entity* entity)
{
	EntityProperty* ent_property = entity->GetProperty("Drawable");
	if (ent_property != nullptr)
	{
		return static_cast<GrEngine::Object*>(ent_property->GetValueAdress());
	}
	else
	{
		return nullptr;
	}
};